#ifndef DELTAENTRY_H
#define DELTAENTRY_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

struct DeltaEntry {
    QString actionId;
    QString sessionId;
    QDateTime timestamp;
    bool crashFlag;
    QString userIntent;
    QString itemId;
    QString calId; // New field
    QString icalData;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["actionId"] = actionId;
        obj["sessionId"] = sessionId;
        obj["timestamp"] = timestamp.toString(Qt::ISODateWithMs);
        obj["crashFlag"] = crashFlag;
        obj["userIntent"] = userIntent;
        obj["itemId"] = itemId;
        obj["calId"] = calId; // Add to JSON
        obj["icalData"] = icalData;
        return obj;
    }

    static DeltaEntry fromJson(const QJsonObject &obj) {
        DeltaEntry entry;
        entry.actionId = obj["actionId"].toString();
        entry.sessionId = obj["sessionId"].toString();
        entry.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODateWithMs);
        entry.crashFlag = obj["crashFlag"].toBool();
        entry.userIntent = obj["userIntent"].toString();
        entry.itemId = obj["itemId"].toString();
        entry.calId = obj["calId"].toString(); // Load from JSON
        entry.icalData = obj["icalData"].toString();
        return entry;
    }
};

// In deltaentry.h, after DeltaEntry class definition
QDataStream &operator<<(QDataStream &out, const DeltaEntry &entry);
QDataStream &operator>>(QDataStream &in, DeltaEntry &entry);

#endif // DELTAENTRY_H
