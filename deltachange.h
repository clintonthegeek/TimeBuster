#ifndef DELTACHANGE_H
#define DELTACHANGE_H

#include <QString>
#include <QSharedPointer>
#include "calendaritem.h"

class DeltaChange
{
public:
    enum Type {
        Add,
        Modify,
        Remove
    };

    DeltaChange() : changeType(Add) {}
    DeltaChange(Type type, const QSharedPointer<CalendarItem> &item)
        : changeType(type), item(item) {}

    Type change() const { return changeType; }
    QSharedPointer<CalendarItem> getItem() const { return item; }

private:
    Type changeType;
    QSharedPointer<CalendarItem> item;
};

#endif // DELTACHANGE_H
