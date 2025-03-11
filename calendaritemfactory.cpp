#include "calendaritemfactory.h"
#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>

CalendarItem* CalendarItemFactory::createItem(const QString &id, const KCalendarCore::Incidence::Ptr &incidence, QObject *parent)
{
    if (!incidence) return nullptr;

    CalendarItem *item = nullptr;
    switch (incidence->type()) {
    case KCalendarCore::IncidenceBase::TypeEvent:
        item = new Event(id, parent);
        break;
    case KCalendarCore::IncidenceBase::TypeTodo:
        item = new Todo(id, parent);
        break;
    default:
        qDebug() << "CalendarItemFactory: Unsupported incidence type";
        return nullptr;
    }
    item->setIncidence(incidence);
    return item;
}

void CalendarItemFactory::destroyItem(CalendarItem *item)
{
    delete item;
}
