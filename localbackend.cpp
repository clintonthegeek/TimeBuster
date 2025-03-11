#include "localbackend.h"
#include "cal.h"
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar> // Include for MemoryCalendar
#include <QDir>
#include <QFile>
#include <QUrl>

LocalBackend::LocalBackend(const QString &rootPath, QObject *parent)
    : SyncBackend(parent), m_rootPath(rootPath)
{
}


QList<CalendarMetadata> LocalBackend::fetchCalendars(const QString &collectionId)
{
    qDebug() << "LocalBackend: Fetching calendars for collection" << collectionId;
    QList<CalendarMetadata> calendars;

    QDir rootDir(m_rootPath);
    if (!rootDir.exists()) {
        qDebug() << "LocalBackend: Root path" << m_rootPath << "does not exist";
        emit dataFetched();
        return calendars;
    }

    // Scan for subdirectories (e.g., "personal", "loganlist")
    QStringList calDirs = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &calName : calDirs) {
        // Use directory name as both ID and name for now - CalDAV ID might override later
        QString calId = calName; // Temporary ID until synced with CalDAV
        calendars.append(CalendarMetadata{ calId, calName });
        m_idToPath[calId] = rootDir.filePath(calName);
        qDebug() << "LocalBackend: Found calendar" << calName << "at" << m_idToPath[calId];
    }

    emit dataFetched();
    return calendars;
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
        QString fileName = QUrl(item->id()).fileName();
        if (fileName.isEmpty()) {
            fileName = QString("%1.ics").arg(qHash(item->id()));
        }
        QString filePath = calDir.filePath(fileName);
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "LocalBackend: Failed to open" << filePath << ":" << file.errorString();
            emit errorOccurred("Failed to write item: " + file.errorString());
            continue;
        }

        // Create a temporary MemoryCalendar to wrap the incidence
        KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone("UTC")));
        KCalendarCore::Incidence::Ptr incidence = item->incidence();
        tempCalendar->addIncidence(incidence);

        // Serialize the full calendar to get a proper VCALENDAR structure
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


void LocalBackend::storeCalendars(const QString &collectionId, const QList<Cal*> &calendars)
{
    qDebug() << "LocalBackend: storeCalendars stub for" << collectionId;
    for (Cal *cal : calendars) {
        m_idToPath[cal->id()] = m_rootPath + "/" + cal->name();
    }
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

        // Log raw data for debugging
        qDebug() << "LocalBackend: Raw ICS data from" << filePath << ":\n" << icalData.left(200) + (icalData.length() > 200 ? "..." : "");

        auto incidence = format.fromString(icalData);
        if (!incidence) {
            qDebug() << "LocalBackend: Failed to parse ICS data from" << filePath << "- Check format or encoding";
            continue;
        }

        QString itemId = m_idToPath.key(filePath, filePath); // Fallback to file path if not mapped
        CalendarItem *item = nullptr;
        switch (incidence->type()) {
        case KCalendarCore::IncidenceBase::TypeEvent:
            item = new Event(itemId, this);
            break;
        case KCalendarCore::IncidenceBase::TypeTodo:
            item = new Todo(itemId, this);
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
