#include "localbackend.h"
#include "cal.h"
#include "calendaritem.h"
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>
#include <QDir>
#include <QFile>
#include <QDebug>

LocalBackend::LocalBackend(const QString &rootPath, QObject *parent)
    : SyncBackend(parent), m_rootPath(rootPath)
{
    qDebug() << "LocalBackend: Initialized with rootPath" << m_rootPath;
}

QList<CalendarMetadata> LocalBackend::fetchCalendars(const QString &collectionId)
{
    qDebug() << "LocalBackend: Fetching calendars for collection" << collectionId;
    QList<CalendarMetadata> calendars;
    QDir rootDir(m_rootPath);
    if (!rootDir.exists()) {
        qDebug() << "LocalBackend: Root directory does not exist:" << m_rootPath;
        emit dataFetched();
        return calendars;
    }

    for (const QString &dirName : rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        CalendarMetadata meta;
        meta.id = collectionId + "_" + dirName.toLower().replace(" ", "_");
        meta.name = dirName;
        calendars.append(meta);
        qDebug() << "LocalBackend: Added calendar" << meta.id << meta.name;
    }
    emit calendarsFetched(collectionId, calendars); // Emit signal
    emit dataFetched();
    return calendars;
}

void LocalBackend::storeCalendars(const QString &collectionId, const QList<Cal*> &calendars)
{
    qDebug() << "LocalBackend: Storing calendars for collection" << collectionId << "with" << calendars.size() << "calendars";
    QDir rootDir(m_rootPath);
    if (!rootDir.exists() && !rootDir.mkpath(".")) {
        qDebug() << "LocalBackend: Failed to create root directory:" << m_rootPath;
        emit errorOccurred("Failed to create root directory: " + m_rootPath);
        return;
    }

    for (const Cal *cal : calendars) {
        if (!cal) {
            qDebug() << "LocalBackend: Skipping null calendar in storeCalendars";
            continue;
        }
        QString calDirPath = QDir(m_rootPath).filePath(cal->name());
        QDir calDir(calDirPath);
        if (!calDir.exists() && !calDir.mkpath(".")) {
            qDebug() << "LocalBackend: Failed to create directory:" << calDirPath;
            emit errorOccurred("Failed to create calendar directory: " + calDirPath);
            continue;
        }
        qDebug() << "LocalBackend: Ensured directory for calendar" << cal->id() << "at" << calDirPath;
    }
    emit dataFetched();
}

QList<CalendarItem*> LocalBackend::fetchItems(Cal *cal)
{
    qDebug() << "LocalBackend: Fetching items for calendar ID" << cal->id() << "name" << cal->name();
    QList<CalendarItem*> items;
    QString calId = cal->id();
    QString calDirPath = QDir(m_rootPath).filePath(cal->name());
    QDir calDir(calDirPath);
    if (!calDir.exists()) {
        qDebug() << "LocalBackend: No directory found for" << cal->name() << "at" << calDirPath;
        emit dataFetched();
        return items;
    }

    QStringList icsFiles = calDir.entryList({"*.ics"}, QDir::Files);
    qDebug() << "LocalBackend: Found" << icsFiles.size() << "ICS files in" << calDirPath;
    KCalendarCore::ICalFormat format;
    for (const QString &fileName : icsFiles) {
        QString filePath = calDir.filePath(fileName);
        qDebug() << "LocalBackend: Processing file" << filePath;
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "LocalBackend: Failed to open" << filePath << ":" << file.errorString();
            continue;
        }

        QString icalData = QString::fromUtf8(file.readAll());
        file.close();
        qDebug() << "LocalBackend: Read" << icalData.size() << "bytes from" << filePath;
        qDebug() << "LocalBackend: ICS data:\n" << icalData.left(200) + (icalData.length() > 200 ? "..." : "");

        KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
        if (!format.fromString(tempCalendar, icalData)) {
            qDebug() << "LocalBackend: Failed to parse ICS data from" << filePath;
            continue;
        }

        KCalendarCore::Incidence::List incidences = tempCalendar->incidences();
        qDebug() << "LocalBackend: Found" << incidences.size() << "incidences in" << filePath;
        if (incidences.isEmpty()) {
            qDebug() << "LocalBackend: No incidences found in" << filePath;
            continue;
        }

        KCalendarCore::Incidence::Ptr incidence = incidences.first();
        qDebug() << "LocalBackend: Incidence type:" << incidence->type() << "UID:" << incidence->uid();
        QString itemUid = incidence->uid().isEmpty() ? QString::number(qHash(filePath)) : incidence->uid();
        QString itemId = calId + "_" + itemUid;
        qDebug() << "LocalBackend: Creating item with ID" << itemId;
        CalendarItem *item = nullptr;
        if (incidence->type() == KCalendarCore::IncidenceBase::TypeEvent) {
            item = new Event(calId, itemUid, cal);
        } else if (incidence->type() == KCalendarCore::IncidenceBase::TypeTodo) {
            item = new Todo(calId, itemUid, cal);
        } else {
            qDebug() << "LocalBackend: Unsupported incidence type" << incidence->type();
            continue;
        }

        if (item) {
            item->setIncidence(incidence);
            items.append(item);
            m_idToPath[itemId] = filePath;
            qDebug() << "LocalBackend: Loaded" << item->type() << itemId << "from" << filePath;
        } else {
            qDebug() << "LocalBackend: Failed to create item for" << filePath;
        }
    }
    qDebug() << "LocalBackend: Fetched" << items.size() << "items for" << cal->name();
    emit itemsFetched(cal, items);
    emit dataFetched();
    return items;
}

void LocalBackend::storeItems(Cal *cal, const QList<CalendarItem*> &items)
{
    qDebug() << "LocalBackend: Storing" << items.size() << "items for" << cal->name();
    QString calId = cal->id();
    QString calDirPath = QDir(m_rootPath).filePath(cal->name());
    QDir calDir(calDirPath);
    if (!calDir.exists() && !calDir.mkpath(".")) {
        qDebug() << "LocalBackend: Failed to create directory" << calDirPath;
        emit errorOccurred("Failed to create calendar directory: " + calDirPath);
        return;
    }

    KCalendarCore::ICalFormat format;
    for (const CalendarItem *item : items) {
        if (!item || !item->incidence()) {
            qDebug() << "LocalBackend: Skipping invalid item in storeItems";
            continue;
        }

        QString itemUid = item->id().split("_").last();
        QString fileName = QString("%1.ics").arg(itemUid);
        QString filePath = calDir.filePath(fileName);
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "LocalBackend: Failed to open" << filePath << ":" << file.errorString();
            emit errorOccurred("Failed to write item: " + file.errorString());
            continue;
        }

        KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
        tempCalendar->addIncidence(item->incidence());
        QString icalData = format.toString(tempCalendar);
        if (file.write(icalData.toUtf8()) == -1) {
            qDebug() << "LocalBackend: Write failed for" << filePath << ":" << file.errorString();
            emit errorOccurred("Write failed: " + file.errorString());
        } else {
            qDebug() << "LocalBackend: Saved" << item->type() << item->id() << "to" << filePath;
            m_idToPath[item->id()] = filePath;
        }
        file.close();
    }
    emit dataFetched();
}

void LocalBackend::updateItem(const QString &calId, const QString &itemId, const QString &icalData)
{
    qDebug() << "LocalBackend: Updating item" << itemId << "for calendar" << calId;
    QString fullId = calId + "_" + itemId;
    QString filePath = m_idToPath.value(fullId);
    if (filePath.isEmpty()) {
        qDebug() << "LocalBackend: No file path found for item" << fullId;
        emit errorOccurred("No file path found for item: " + fullId);
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "LocalBackend: Failed to open" << filePath << ":" << file.errorString();
        emit errorOccurred("Failed to write item: " + file.errorString());
        return;
    }

    if (file.write(icalData.toUtf8()) == -1) {
        qDebug() << "LocalBackend: Write failed for" << filePath << ":" << file.errorString();
        emit errorOccurred("Write failed: " + file.errorString());
    } else {
        qDebug() << "LocalBackend: Updated item" << fullId << "at" << filePath;
    }
    file.close();
    emit dataFetched();
}
