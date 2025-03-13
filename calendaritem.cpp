#include "calendaritem.h"
#include <QDateTime>

CalendarItem::CalendarItem(const QString &calId, const QString &itemUid, QObject *parent)
    : QObject(parent), m_id(calId + "_" + itemUid), m_incidence(nullptr)
{
}

void CalendarItem::setIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    m_incidence = incidence;
}

Event::Event(const QString &calId, const QString &itemUid, QObject *parent)
    : CalendarItem(calId, itemUid, parent)
{
}

QVariant Event::data(int role) const
{
    if (role == Qt::DisplayRole) {
        return m_incidence ? m_incidence->summary() : QVariant();
    } else if (role == Qt::UserRole) {
        return m_incidence ? m_incidence.staticCast<KCalendarCore::Event>()->dtStart().toString() : QVariant();
    } else if (role == Qt::UserRole + 1) {
        return m_incidence ? m_incidence.staticCast<KCalendarCore::Event>()->dtEnd().toString() : QVariant();
    }
    return QVariant();
}

CalendarItem* Event::clone(QObject *parent) const
{
    if (!m_incidence) return nullptr;
    Event *clone = new Event(id().split("_").first(), id().split("_").last(), parent);
    clone->setIncidence(KCalendarCore::Incidence::Ptr(m_incidence->clone())); // Wrap in QSharedPointer
    return clone;
}

Todo::Todo(const QString &calId, const QString &itemUid, QObject *parent)
    : CalendarItem(calId, itemUid, parent)
{
}

QVariant Todo::data(int role) const
{
    if (role == Qt::DisplayRole) {
        return m_incidence ? m_incidence->summary() : QVariant();
    } else if (role == Qt::UserRole) {
        return m_incidence ? m_incidence.staticCast<KCalendarCore::Todo>()->dtStart().toString() : QVariant();
    } else if (role == Qt::UserRole + 1) {
        return m_incidence ? m_incidence.staticCast<KCalendarCore::Todo>()->dtDue().toString() : QVariant();
    }
    return QVariant();
}

CalendarItem* Todo::clone(QObject *parent) const
{
    if (!m_incidence) return nullptr;
    Todo *clone = new Todo(id().split("_").first(), id().split("_").last(), parent);
    clone->setIncidence(KCalendarCore::Incidence::Ptr(m_incidence->clone())); // Wrap in QSharedPointer
    return clone;
}
