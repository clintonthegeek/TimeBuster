#include "sessionmanager.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "cal.h"
#include "calendaritemfactory.h"
#include "localbackend.h" // Add this to declare LocalBackend
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

SessionManager::SessionManager(const QString &kalbFilePath, QObject *parent)
    : QObject(parent), m_kalbFilePath(kalbFilePath)
{
    setKalbFilePath(kalbFilePath); // Initialize delta path
}

SessionManager::~SessionManager()
{
    saveCache();
}

void SessionManager::setKalbFilePath(const QString &kalbFilePath)
{
    m_kalbFilePath = kalbFilePath;
    m_deltaFilePath = QFileInfo(m_kalbFilePath).absolutePath() + "/.kalb.delta";
    qDebug() << "SessionManager: Set delta path to" << m_deltaFilePath;
}

void SessionManager::stageChange(const Change &change)
{
    m_changes.append(change);
    saveCache(); // Save immediately for now
}

/*
void SessionManager::saveCache()
{
    if (m_changes.isEmpty()) return;

    QFile file(m_kalbPath + ".delta");
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "SessionManager: Failed to save cache" << file.errorString();
        return;
    }

    QJsonArray jsonArray;
    for (const Change &change : m_changes) {
        QJsonObject jsonObj;
        jsonObj["calId"] = change.calId;
        jsonObj["itemId"] = change.itemId;
        jsonObj["operation"] = change.operation;
        jsonObj["data"] = change.data.toString();
        jsonObj["timestamp"] = change.timestamp.toString(Qt::ISODate);
        jsonArray.append(jsonObj);
    }

    QJsonDocument doc(jsonArray);
    file.write(doc.toJson());
    file.close();
}*/
void SessionManager::saveCache()
{
    QFile file(m_deltaFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const Change &change : m_changes) {
            out << "calId:" << change.calId << "\n"
                << "itemId:" << change.itemId << "\n"
                << "operation:" << change.operation << "\n"
                << "data:" << change.data << "\n"
                << "timestamp:" << change.timestamp.toString() << "\n"
                << "---\n";
        }
        file.close();
    } else {
        qDebug() << "SessionManager: Failed to save cache to" << m_deltaFilePath;
    }
}

/*
void SessionManager::loadCache()
{
    QFile file(m_kalbPath + ".delta");
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return;

    QJsonArray jsonArray = doc.array();
    for (const QJsonValue &value : jsonArray) {
        QJsonObject obj = value.toObject();
        Change change;
        change.calId = obj["calId"].toString();
        change.itemId = obj["itemId"].toString();
        change.operation = obj["operation"].toString();
        change.data = obj["data"].toString();
        change.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        m_changes.append(change);
    }
}*/

void SessionManager::loadCache()
{
    m_changes.clear();
    QFile file(m_deltaFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        Change change;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line == "---") {
                if (!change.calId.isEmpty()) {
                    m_changes.append(change);
                    change = Change(); // Reset for next change
                }
                continue;
            }
            if (line.startsWith("calId:")) change.calId = line.mid(6);
            else if (line.startsWith("itemId:")) change.itemId = line.mid(7);
            else if (line.startsWith("operation:")) change.operation = line.mid(10);
            else if (line.startsWith("data:")) change.data = line.mid(5);
            else if (line.startsWith("timestamp:")) change.timestamp = QDateTime::fromString(line.mid(10));
        }
        if (!change.calId.isEmpty()) m_changes.append(change); // Append the last change
        file.close();
        qDebug() << "SessionManager: Loaded" << m_changes.size() << "changes";
    } else {
        qDebug() << "SessionManager: No cache file found at" << m_deltaFilePath;
    }
}

/*
void SessionManager::applyToBackend(SyncBackend *backend)
{
    for (const Change &change : m_changes) {
        if (change.operation == "ADD" || change.operation == "UPDATE") {
            // Parse the serialized data into an Incidence
            KCalendarCore::ICalFormat format;
            QString dataStr = change.data.toString();
            KCalendarCore::Incidence::Ptr incidence = format.fromString(dataStr);
            if (!incidence) {
                qDebug() << "SessionManager: Failed to parse incidence from data for item" << change.itemId;
                continue;
            }

            // Create a temporary Cal object with nullptr parent
            Cal *cal = new Cal(change.calId, change.calId.split('/').last(), nullptr);
            CalendarItem *item = CalendarItemFactory::createItem(change.itemId, incidence, cal);
            if (!item) {
                qDebug() << "SessionManager: Failed to create CalendarItem for" << change.itemId;
                delete cal;
                continue;
            }

            // Add item to cal and store via backend
            cal->addItem(item);
            backend->storeCalendars(change.calId, {cal});
            backend->storeItems(cal, {item});

            // Clean up temporary Cal object
            delete cal;
        } else if (change.operation == "DELETE") {
            // TODO: Handle DELETE operation (e.g., remove item from backend)
            qDebug() << "SessionManager: DELETE operation not yet implemented for" << change.itemId;
        }
    }
    clearCache();
}*/

void SessionManager::applyToBackend(SyncBackend *backend)
{
    Q_UNUSED(backend); // Not used for storage
    if (!m_collectionManager) {
        qDebug() << "SessionManager: No CollectionManager available";
        return;
    }

    for (const Change &change : m_changes) {
        if (change.operation == "UPDATE") {
            bool calFound = false;
            for (Collection *col : m_collectionManager->getCollections()) {
                Cal *cal = col->calendar(change.calId);
                if (!cal) continue;
                calFound = true;
                for (int i = 0; i < cal->items().size(); ++i) {
                    CalendarItem *item = cal->items().at(i);
                    if (item->id() == change.itemId) {
                        KCalendarCore::ICalFormat format;
                        KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone("UTC")));
                        if (format.fromString(tempCalendar, change.data)) {
                            KCalendarCore::Incidence::List incidences = tempCalendar->incidences();
                            if (!incidences.isEmpty()) {
                                item->setIncidence(incidences.first());
                                emit cal->dataChanged(cal->index(i, 0), cal->index(i, cal->columnCount() - 1));
                                qDebug() << "SessionManager: Updated in-memory item" << change.itemId;
                            }
                        }
                        break;
                    }
                }
                break;
            }
            if (!calFound) {
                qDebug() << "SessionManager: Calendar" << change.calId << "not found";
            }
        }
    }
    emit changesApplied();
}

void SessionManager::commitChanges(SyncBackend *backend)
{
    if (LocalBackend *local = qobject_cast<LocalBackend*>(backend)) {
        for (const Change &change : m_changes) {
            if (change.operation == "UPDATE") {
                local->updateItem(change.calId, change.itemId, change.data);
                qDebug() << "SessionManager: Committed change for item" << change.itemId;
            }
        }
        clearCache();
    }
}

void SessionManager::clearCache()
{
    QFile::remove(m_deltaFilePath); // Use m_deltaFilePath instead of m_kalbPath + ".delta"
    m_changes.clear();
}
