#include "sessionmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "cal.h"
#include "calendaritemfactory.h"
#include <KCalendarCore/ICalFormat>

SessionManager::SessionManager(const QString &kalbPath, QObject *parent)
    : QObject(parent), m_kalbPath(kalbPath)
{
    loadCache();
}

SessionManager::~SessionManager()
{
    saveCache();
}

void SessionManager::stageChange(const Change &change)
{
    m_changes.append(change);
}

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
}

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
}

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
}

void SessionManager::clearCache()
{
    QFile::remove(m_kalbPath + ".delta");
    m_changes.clear();
}
