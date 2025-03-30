#ifndef DELTAENTRY_H
#define DELTAENTRY_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

struct DeltaEntry {
    QString actionId;      // Unique UUID per edit
    QString sessionId;     // Unique per app run
    QDateTime timestamp;   // ISO 8601 timestamp
    bool crashFlag;        // Reserved, unused for now
    QString userIntent;    // e.g., "modify summary"
    QString itemId;        // CalendarItem UUID
    QString icalData;      // Base64-encoded .ics snippet

    // Serialize to JSON
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["actionId"] = actionId;
        obj["sessionId"] = sessionId;
        obj["timestamp"] = timestamp.toString(Qt::ISODate);
        obj["crashFlag"] = crashFlag;
        obj["userIntent"] = userIntent;
        obj["itemId"] = itemId;
        obj["icalData"] = icalData;
        return obj;
    }

    // Deserialize from JSON
    static DeltaEntry fromJson(const QJsonObject& obj) {
        DeltaEntry entry;
        entry.actionId = obj["actionId"].toString();
        entry.sessionId = obj["sessionId"].toString();
        entry.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        entry.crashFlag = obj["crashFlag"].toBool();
        entry.userIntent = obj["userIntent"].toString();
        entry.itemId = obj["itemId"].toString();
        entry.icalData = obj["icalData"].toString();
        return entry;
    }
};

#endif // DELTAENTRY_H
