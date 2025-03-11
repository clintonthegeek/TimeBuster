#include "calendaritem.h"
#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>
#include <QDateTime>

CalendarItem::CalendarItem(const QString &id, QObject *parent)
    : QObject(parent), m_id(id)
{
}

void CalendarItem::setIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    m_incidence = incidence;
}

Event::Event(const QString &id, QObject *parent)
    : CalendarItem(id, parent)
{
}

QVariant Event::data(int role) const
{
    if (!m_incidence) return QVariant();
    auto event = qSharedPointerCast<KCalendarCore::Event>(m_incidence);
    switch (role) {
    case Qt::DisplayRole:
        return event->summary();
    case Qt::UserRole:
        return event->dtStart();
    case Qt::UserRole + 1:
        return event->dtEnd();
    default:
        return QVariant();
    }
}

Todo::Todo(const QString &id, QObject *parent)
    : CalendarItem(id, parent)
{
}

QVariant Todo::data(int role) const
{
    if (!m_incidence) return QVariant();
    auto todo = qSharedPointerCast<KCalendarCore::Todo>(m_incidence);
    switch (role) {
    case Qt::DisplayRole:
        return todo->summary();
    case Qt::UserRole:
        return todo->dtStart(false); // Current occurrence
    case Qt::UserRole + 1:
        return todo->dtDue(false);
    case Qt::UserRole + 2:
        return todo->percentComplete();
    default:
        return QVariant();
    }
}

CalendarItem* Event::clone(QObject *parent) const
{
    Event *newEvent = new Event(m_id, parent);
    if (m_incidence) {
        newEvent->setIncidence(KCalendarCore::Incidence::Ptr(m_incidence->clone())); // Wrap in QSharedPointer
    }
    return newEvent;
}

CalendarItem* Todo::clone(QObject *parent) const
{
    Todo *newTodo = new Todo(m_id, parent);
    if (m_incidence) {
        newTodo->setIncidence(KCalendarCore::Incidence::Ptr(m_incidence->clone())); // Wrap in QSharedPointer
    }
    return newTodo;
}
