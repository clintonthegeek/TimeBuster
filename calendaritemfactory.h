#ifndef CALENDARITEMFACTORY_H
#define CALENDARITEMFACTORY_H

#include <QObject>
#include "calendaritem.h"

class CalendarItemFactory : public QObject
{
    Q_OBJECT
public:
    static CalendarItem* createItem(const QString &id, const KCalendarCore::Incidence::Ptr &incidence, QObject *parent = nullptr);
    static void destroyItem(CalendarItem *item); // Optional cleanup method
};

#endif // CALENDARITEMFACTORY_H
