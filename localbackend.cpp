#include "localbackend.h"
#include "cal.h"
#include "calendaritem.h"
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QCryptographicHash>

LocalBackend::LocalBackend(const QString &rootPath, QObject *parent)
    : SyncBackend(parent), m_rootPath(rootPath)
{
    qDebug() << "LocalBackend: Initialized with rootPath" << m_rootPath;
}

QList<CalendarMetadata> LocalBackend::loadCalendars(const QString &collectionId)
{
    qDebug() << "LocalBackend: Loading calendars for collection" << collectionId;
    QList<CalendarMetadata> calendars;
    QDir rootDir(m_rootPath);
    if (!rootDir.exists()) {
        qDebug() << "LocalBackend: Root directory does not exist:" << m_rootPath;
        return calendars;
    }

    for (const QString &dirName : rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        CalendarMetadata meta;
        meta.id = collectionId + "_" + dirName.toLower().replace(" ", "_");
        meta.name = dirName;
        calendars.append(meta);
        qDebug() << "LocalBackend: Added calendar" << meta.id << meta.name;
    }
    qDebug() << "LocalBackend: Loaded" << calendars.size() << "calendars for collection" << collectionId;
    return calendars; // No signals here—let startSync handle it
}

QList<QSharedPointer<CalendarItem>> LocalBackend::loadItems(Cal *cal)
{
    qDebug() << "LocalBackend: Loading items for calendar ID" << cal->id() << "name" << cal->name();
    QList<QSharedPointer<CalendarItem>> items;
    QString calId = cal->id();
    if (calId.isEmpty()) {
        qWarning() << "LocalBackend: Empty calId for calendar" << cal->name();
        return items;
    }

    QString calDirPath = QDir(m_rootPath).filePath(cal->name());
    QDir calDir(calDirPath);
    if (!calDir.exists()) {
        qDebug() << "LocalBackend: No directory found for" << cal->name() << "at" << calDirPath;
        return items;
    }

    QStringList icsFiles = calDir.entryList({"*.ics"}, QDir::Files);
    qDebug() << "LocalBackend: Found" << icsFiles.size() << "ICS files in" << calDirPath;
    KCalendarCore::ICalFormat format;
    for (const QString &fileName : icsFiles) {
        QString filePath = calDir.filePath(fileName);
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "LocalBackend: Failed to open" << filePath << ":" << file.errorString();
            continue;
        }

        QString icalData = QString::fromUtf8(file.readAll());
        file.close();
        if (icalData.isEmpty()) {
            qWarning() << "LocalBackend: Empty ICS data in" << filePath;
            continue;
        }

        KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
        if (!format.fromString(tempCalendar, icalData)) {
            qWarning() << "LocalBackend: Failed to parse ICS data from" << filePath;
            continue;
        }

        KCalendarCore::Incidence::List incidences = tempCalendar->incidences();
        if (incidences.isEmpty()) {
            qWarning() << "LocalBackend: No incidences found in" << filePath;
            continue;
        }

        KCalendarCore::Incidence::Ptr incidence = incidences.first();
        QString itemUid = incidence->uid();
        if (itemUid.isEmpty()) {
            qDebug() << "LocalBackend: Empty UID in" << filePath << "- generating fallback";
            itemUid = QString::number(qHash(filePath));
            if (itemUid == "0" || itemUid.isEmpty()) {
                itemUid = QString("fallback_%1").arg(QDateTime::currentMSecsSinceEpoch());
            }
        }
        QString itemId = calId + "_" + itemUid;

        QSharedPointer<CalendarItem> item;
        if (incidence->type() == KCalendarCore::IncidenceBase::TypeEvent) {
            item = QSharedPointer<CalendarItem>(new Event(calId, itemUid, nullptr));
        } else if (incidence->type() == KCalendarCore::IncidenceBase::TypeTodo) {
            item = QSharedPointer<CalendarItem>(new Todo(calId, itemUid, nullptr));
        } else {
            qWarning() << "LocalBackend: Unsupported incidence type" << incidence->type() << "in" << filePath;
            continue;
        }

        item->setIncidence(incidence);
        QFileInfo fileInfo(filePath);
        item->setLastModified(fileInfo.lastModified());
        item->setVersionIdentifier(""); // No ETag for local files
        items.append(item);
        m_idToPath[itemId] = filePath;
    }

    qDebug() << "LocalBackend: Loaded" << items.size() << "items for calendar" << cal->name();
    return items; // No signals here—let startSync handle it
}

void LocalBackend::startSync(const QString &collectionId)
{
    qDebug() << "LocalBackend: Starting sync for collection" << collectionId;

    QList<CalendarMetadata> calendars = loadCalendars(collectionId);
    for (const CalendarMetadata &meta : calendars) {
        emit calendarDiscovered(collectionId, meta);
        Cal tempCal(meta.id, meta.name, nullptr); // Stack-allocated
        QList<QSharedPointer<CalendarItem>> items = loadItems(&tempCal);
        qDebug() << "LocalBackend: Loaded" << items.size() << "items for" << meta.id;
        for (const QSharedPointer<CalendarItem> &item : items) {
            QString verId = fetchItemVersionIdentifier(tempCal.id(), item->id());
            emit itemLoaded(&tempCal, item, verId);
        }
        emit calendarLoaded(&tempCal);
    }

    emit syncCompleted(collectionId);
}

void LocalBackend::storeCalendars(const QString &collectionId, const QList<Cal*> &calendars)
{
    qDebug() << "LocalBackend: Storing calendars for collection" << collectionId << "with" << calendars.size() << "calendars";
    QDir rootDir(m_rootPath);
    if (!rootDir.exists() && !rootDir.mkpath(".")) {
        qWarning() << "LocalBackend: Failed to create root directory:" << m_rootPath;
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
            qWarning() << "LocalBackend: Failed to create directory:" << calDirPath;
            emit errorOccurred("Failed to create calendar directory: " + calDirPath);
            continue;
        }
        qDebug() << "LocalBackend: Ensured directory for calendar" << cal->id() << "at" << calDirPath;
    }
    // No dataLoaded—caller should handle completion
}

void LocalBackend::storeItems(Cal *cal, const QList<QSharedPointer<CalendarItem>> &items)
{
    qDebug() << "LocalBackend: Storing" << items.size() << "items for" << cal->name();
    QString calId = cal->id();
    if (calId.isEmpty()) {
        qWarning() << "LocalBackend: Empty calId in storeItems for" << cal->name();
        emit errorOccurred("Empty calId in storeItems");
        return;
    }

    QString calDirPath = QDir(m_rootPath).filePath(cal->name());
    QDir calDir(calDirPath);
    if (!calDir.exists() && !calDir.mkpath(".")) {
        qWarning() << "LocalBackend: Failed to create directory" << calDirPath;
        emit errorOccurred("Failed to create calendar directory: " + calDirPath);
        return;
    }

    KCalendarCore::ICalFormat format;
    for (const QSharedPointer<CalendarItem> &item : items) {
        if (!item || !item->incidence()) {
            qDebug() << "LocalBackend: Skipping invalid item in storeItems";
            continue;
        }

        QString itemUid = item->id().split("_").last();
        QString fileName = QString("%1.ics").arg(itemUid);
        QString filePath = calDir.filePath(fileName);
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "LocalBackend: Failed to open" << filePath << ":" << file.errorString();
            emit errorOccurred("Failed to write item: " + file.errorString());
            continue;
        }

        KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
        tempCalendar->addIncidence(item->incidence());
        QString icalData = format.toString(tempCalendar);
        if (file.write(icalData.toUtf8()) == -1) {
            qWarning() << "LocalBackend: Write failed for" << filePath << ":" << file.errorString();
            emit errorOccurred("Write failed: " + file.errorString());
        } else {
            qDebug() << "LocalBackend: Saved" << item->type() << item->id() << "to" << filePath;
            m_idToPath[item->id()] = filePath;
        }
        file.close();
    }
    // No dataLoaded—caller should handle completion
}

void LocalBackend::updateItem(const QString &calId, const QString &itemId, const QString &icalData)
{
    qDebug() << "LocalBackend: Updating item" << itemId << "for calendar" << calId;
    QString fullId = calId + "_" + itemId;
    QString filePath = m_idToPath.value(fullId);
    if (filePath.isEmpty()) {
        qWarning() << "LocalBackend: No file path found for item" << fullId;
        emit errorOccurred("No file path found for item: " + fullId);
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "LocalBackend: Failed to open" << filePath << ":" << file.errorString();
        emit errorOccurred("Failed to write item: " + file.errorString());
        return;
    }

    if (file.write(icalData.toUtf8()) == -1) {
        qWarning() << "LocalBackend: Write failed for" << filePath << ":" << file.errorString();
        emit errorOccurred("Write failed: " + file.errorString());
    } else {
        qDebug() << "LocalBackend: Updated item" << fullId << "at" << filePath;
    }
    file.close();
    // No dataLoaded—caller should handle completion
}

QString LocalBackend::fetchItemVersionIdentifier(const QString &calId, const QString &itemId)
{
    Q_UNUSED(calId);
    QString filePath = m_idToPath.value(itemId);
    if (filePath.isEmpty()) {
        qWarning() << "LocalBackend: No file path found for item" << itemId;
        return QString();
    }
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "LocalBackend: Failed to open file" << filePath;
        return QString();
    }
    QByteArray data = file.readAll();
    file.close();
    // Use MD5 for a lightweight hash; collisions are extremely unlikely in our use case.
    QByteArray md5 = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    QString verId = QString::fromUtf8(md5.toHex());
    qDebug() << "LocalBackend: Computed version identifier for item" << itemId << ":" << verId;
    return verId;
}

void LocalBackend::removeItem(const QString &calId, const QString &itemId)
{
    Q_UNUSED(calId);
    QString filePath = m_idToPath.value(itemId);
    if (filePath.isEmpty()) {
        qWarning() << "LocalBackend: No file path found for item" << itemId;
        return;
    }
    QFile file(filePath);
    if (file.exists()) {
        if (!file.remove()) {
            qWarning() << "LocalBackend: Failed to remove file" << filePath << ":" << file.errorString();
        } else {
            m_idToPath.remove(itemId);
            qDebug() << "LocalBackend: Successfully removed item" << itemId;
        }
    } else {
        qWarning() << "LocalBackend: File" << filePath << "does not exist for item" << itemId;
    }
}
