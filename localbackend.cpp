#include "localbackend.h"
#include "cal.h"
#include "databasemanager.h"
#include "calendaritemfactory.h"
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

LocalBackend::LocalBackend(const QString &rootPath, QObject *parent)
    : SyncBackend(parent), m_rootPath(rootPath)
{
    qDebug() << "LocalBackend: Initialized with rootPath" << m_rootPath;
}

QList<CalendarMetadata> LocalBackend::fetchCalendars(const QString &collectionId)
{
    qDebug() << "LocalBackend: Fetching calendars for collection" << collectionId;
    QList<CalendarMetadata> calendars;

    // Initialize the database
    QSqlDatabase db;
    if (!DatabaseManager::initializeDatabase(collectionId, m_rootPath, db)) {
        qDebug() << "LocalBackend: Database initialization failed for" << collectionId;
        emit dataFetched();
        return calendars;
    }

    // Fetch calendars from the database
    QSqlQuery query(db);
    query.exec("SELECT calId, calName, path FROM calendars");
    if (query.lastError().isValid()) {
        qDebug() << "LocalBackend: Failed to query calendars:" << query.lastError().text();
    }

    int count = 0;
    while (query.next()) {
        QString calId = query.value("calId").toString();
        QString calName = query.value("calName").toString();
        QString calPath = query.value("path").toString();
        calendars.append(CalendarMetadata{ calId, calName });
        m_idToPath[calId] = calPath;
        qDebug() << "LocalBackend: Loaded calendar" << calName << "at" << calPath;
        count++;
    }
    qDebug() << "LocalBackend: Loaded" << count << "calendars from database";

    db.close();
    emit dataFetched();
    return calendars;
}

void LocalBackend::storeCalendars(const QString &collectionId, const QList<Cal*> &calendars)
{
    qDebug() << "LocalBackend: Storing calendars for collection" << collectionId << "with" << calendars.size() << "calendars";

    // Initialize the database
    QSqlDatabase db;
    if (!DatabaseManager::initializeDatabase(collectionId, m_rootPath, db)) {
        qDebug() << "LocalBackend: Database initialization failed for" << collectionId;
        return;
    }

    // Begin transaction for atomicity
    db.transaction();

    // Clear existing entries (we'll rewrite the full set)
    QSqlQuery query(db);
    query.exec("DELETE FROM calendars");
    if (query.lastError().isValid()) {
        qDebug() << "LocalBackend: Failed to clear calendars table:" << query.lastError().text();
        emit errorOccurred("Failed to clear calendars table: " + query.lastError().text());
        db.rollback();
        db.close();
        return;
    }

    // Insert new calendar metadata
    QSqlQuery insertQuery(db);
    insertQuery.prepare("INSERT INTO calendars (calId, calName, path) VALUES (?, ?, ?)");
    for (const Cal *cal : calendars) {
        if (!cal) {
            qDebug() << "LocalBackend: Skipping null calendar in storeCalendars";
            continue;
        }
        QString calPath = QDir(m_rootPath).filePath(cal->name());
        insertQuery.addBindValue(cal->id());
        insertQuery.addBindValue(cal->name());
        insertQuery.addBindValue(calPath);
        if (!insertQuery.exec()) {
            qDebug() << "LocalBackend: Failed to store calendar" << cal->name() << ":" << insertQuery.lastError().text();
            emit errorOccurred("Failed to store calendar: " + insertQuery.lastError().text());
            db.rollback();
            db.close();
            return;
        }
        m_idToPath[cal->id()] = calPath;
        qDebug() << "LocalBackend: Stored calendar" << cal->name() << "at" << calPath;
    }

    // Commit transaction
    if (!db.commit()) {
        qDebug() << "LocalBackend: Failed to commit transaction:" << db.lastError().text();
        emit errorOccurred("Failed to commit transaction: " + db.lastError().text());
        db.rollback();
        db.close();
        return;
    }

    qDebug() << "LocalBackend: Successfully stored" << calendars.size() << "calendars for collection" << collectionId;
    db.close();
    emit dataFetched();
}

void LocalBackend::storeItems(Cal *cal, const QList<CalendarItem*> &items)
{
    qDebug() << "LocalBackend: Storing" << items.size() << "items for" << cal->name();
    QString calDirPath = QDir(m_rootPath).filePath(cal->name());
    QDir calDir(calDirPath);
    if (!calDir.exists()) {
        if (!calDir.mkpath(".")) {
            qDebug() << "LocalBackend: Failed to create directory" << calDirPath;
            emit errorOccurred("Failed to create calendar directory: " + calDirPath);
            return;
        }
    }

    KCalendarCore::ICalFormat format;
    for (const CalendarItem *item : items) {
        CalendarItem *clonedItem = CalendarItemFactory::createItem(item->id(), item->incidence(), nullptr);
        if (clonedItem) {
            qDebug() << "LocalBackend: Failed to clone item" << item->id();
            continue;
        }

        QString fileName = QUrl(item->id()).fileName();
        if (fileName.isEmpty()) {
            fileName = QString("%1.ics").arg(qHash(item->id()));
        }
        QString filePath = calDir.filePath(fileName);
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "LocalBackend: Failed to open" << filePath << ":" << file.errorString();
            emit errorOccurred("Failed to write item: " + file.errorString());
            delete clonedItem;
            continue;
        }

        KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone("UTC")));
        KCalendarCore::Incidence::Ptr incidence = clonedItem->incidence();
        tempCalendar->addIncidence(incidence);
        QString icalData = format.toString(tempCalendar);
        if (file.write(icalData.toUtf8()) == -1) {
            qDebug() << "LocalBackend: Write failed for" << filePath << ":" << file.errorString();
            emit errorOccurred("Write failed: " + file.errorString());
        } else {
            qDebug() << "LocalBackend: Saved" << clonedItem->type() << clonedItem->id() << "to" << filePath;
            m_idToPath[clonedItem->id()] = filePath;
        }
        file.close();
        delete clonedItem; // Clean up the clone after saving
    }
    emit dataFetched();
}

QList<CalendarItem*> LocalBackend::fetchItems(Cal *cal)
{
    qDebug() << "LocalBackend: Fetching items for" << cal->name();
    QList<CalendarItem*> items;

    QString calDirPath = QDir(m_rootPath).filePath(cal->name());
    QDir calDir(calDirPath);
    if (!calDir.exists()) {
        qDebug() << "LocalBackend: No directory found for" << cal->name() << "at" << calDirPath;
        emit dataFetched();
        return items;
    }

    KCalendarCore::ICalFormat format;
    QStringList icsFiles = calDir.entryList({"*.ics"}, QDir::Files);
    for (const QString &fileName : icsFiles) {
        QString filePath = calDir.filePath(fileName);
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "LocalBackend: Failed to open" << filePath << ":" << file.errorString();
            continue;
        }

        QString icalData = QString::fromUtf8(file.readAll());
        file.close();

        qDebug() << "LocalBackend: Raw ICS data from" << filePath << ":\n" << icalData.left(200) + (icalData.length() > 200 ? "..." : "");

        KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone("UTC")));
        if (!format.fromString(tempCalendar, icalData)) {
            qDebug() << "LocalBackend: Failed to parse ICS data from" << filePath << "- Check format or encoding";
            continue;
        }

        KCalendarCore::Incidence::List incidences = tempCalendar->incidences();
        if (incidences.isEmpty()) {
            qDebug() << "LocalBackend: No incidences found in" << filePath;
            continue;
        }

        KCalendarCore::Incidence::Ptr incidence = incidences.first();
        QString itemId = m_idToPath.key(filePath, filePath);
        CalendarItem *item = nullptr;
        switch (incidence->type()) {
        case KCalendarCore::IncidenceBase::TypeEvent:
            item = new Event(itemId, cal); // Parent to Cal, not LocalBackend
            break;
        case KCalendarCore::IncidenceBase::TypeTodo:
            item = new Todo(itemId, cal);  // Parent to Cal, not LocalBackend
            break;
        default:
            qDebug() << "LocalBackend: Unsupported incidence type in" << filePath;
            continue;
        }
        item->setIncidence(incidence);
        items.append(item);
        qDebug() << "LocalBackend: Loaded" << item->type() << itemId << "from" << filePath;
        m_idToPath[itemId] = filePath;
    }

    emit dataFetched();
    return items;
}
