#include "calendaritemfactory.h"
#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>

CalendarItem* CalendarItemFactory::createItem(const QString &id, const KCalendarCore::Incidence::Ptr &incidence, QObject *parent)
{
    if (!incidence) return nullptr;

    // Assume id is the calId (e.g., "col0_next_actions")
    QString calId = id;
    // Derive itemUid from incidence
    QString itemUid = incidence->uid().isEmpty() ? QString::number(qHash(id)) : incidence->uid();

    CalendarItem *item = nullptr;
    switch (incidence->type()) {
    case KCalendarCore::IncidenceBase::TypeEvent:
        item = new Event(calId, itemUid, parent);
        break;
    case KCalendarCore::IncidenceBase::TypeTodo:
        item = new Todo(calId, itemUid, parent);
        break;
    default:
        qDebug() << "CalendarItemFactory: Unsupported incidence type" << incidence->type();
        return nullptr;
    }
    item->setIncidence(incidence);
    return item;
}

void CalendarItemFactory::destroyItem(CalendarItem *item)
{
    delete item;
}
