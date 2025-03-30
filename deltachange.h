#ifndef DELTACHANGE_H
#define DELTACHANGE_H

#include <QString>
#include <QSharedPointer>
#include "calendaritem.h"
#include <QDataStream>

class Cal; // Forward declaration

class DeltaChange {
public:
    enum Type { Add, Modify, Remove };
    DeltaChange() : changeType(Add), itemId("") {}
    DeltaChange(Type type, const QSharedPointer<CalendarItem> &item)
        : changeType(type), itemId(item ? item->id() : ""), item(item) {}

    Type change() const { return changeType; }
    QSharedPointer<CalendarItem> getItem() const { return item; }
    QString getItemId() const { return itemId; }
    void resolveItem(Cal* cal); // New: Links item from Cal

private:
    Type changeType;
    QString itemId;
    QSharedPointer<CalendarItem> item;

    friend QDataStream &operator<<(QDataStream &out, const DeltaChange &delta);
    friend QDataStream &operator>>(QDataStream &in, DeltaChange &delta);
};

inline QDataStream &operator<<(QDataStream &out, const DeltaChange &delta) {
    out << static_cast<int>(delta.changeType) << delta.itemId;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, DeltaChange &delta) {
    int type;
    in >> type >> delta.itemId;
    delta.changeType = static_cast<DeltaChange::Type>(type);
    delta.item = nullptr; // Reset, resolved later
    return in;
}

#endif // DELTACHANGE_H
