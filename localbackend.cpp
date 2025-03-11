#include "localbackend.h"
#include "cal.h"
#include <KCalendarCore/ICalFormat>
#include <QDir>
#include <QFile>
#include <QUrl>

LocalBackend::LocalBackend(const QString &rootPath, QObject *parent)
    : SyncBackend(parent), m_rootPath(rootPath)
{
}


QList<CalendarMetadata> LocalBackend::fetchCalendars(const QString &collectionId)
{
    // Stubbed - still fake for now
    QList<CalendarMetadata> calendars;
    calendars.append(CalendarMetadata{ "cal1", "Local Calendar 1" });
    m_idToPath["cal1"] = m_rootPath + "/cal1";
    emit dataFetched();
    return calendars;
}

void LocalBackend::storeItems(Cal *cal, const QList<CalendarItem*> &items)
{
    qDebug() << "LocalBackend: Storing" << items.size() << "items for" << cal->name();
    QString calDirPath = QDir(m_rootPath).filePath(cal->name()); // e.g., /tmp/personal
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

        QString icalData = format.toString(item->incidence());
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
        auto incidence = format.fromString(icalData);
        if (!incidence) {
            qDebug() << "LocalBackend: Failed to parse ICS data from" << filePath;
            continue;
        }

        QString itemId = m_idToPath.key(filePath, filePath);
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
