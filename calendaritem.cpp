#include "calendaritem.h"
#include <QDateTime>
#include <QDebug>
#include <QString>

CalendarItem::CalendarItem(const QString &calId, const QString &itemId, QObject *parent)
    : QObject(parent), m_calId(calId), m_itemId(itemId), m_lastModified(QDateTime::currentDateTime())
{
    //qDebug() << "CalendarItem: Created with calId" << calId << "itemId" << itemId;
}

void CalendarItem::setIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    m_incidence = incidence;
    if (incidence) {
        m_lastModified = QDateTime::currentDateTime();
        //qDebug() << "CalendarItem: Set incidence for" << m_itemId;
    }
}

Event::Event(const QString &calId, const QString &itemId, QObject *parent)
    : CalendarItem(calId, itemId, parent)
{
}

QString Event::type() const
{
    return "Event";
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

QString Event::toICal() const
{
    if (!m_incidence) return QString();
    KCalendarCore::ICalFormat format;
    KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
    tempCalendar->addIncidence(m_incidence);
    return format.toString(tempCalendar);
}

CalendarItem* Event::clone(QObject *parent) const
{
    if (!m_incidence) return nullptr;
    Event *clone = new Event(m_calId, m_itemId.split("_").last(), parent);
    clone->setIncidence(KCalendarCore::Incidence::Ptr(m_incidence->clone()));
    clone->setLastModified(m_lastModified);
    clone->setEtag(m_etag);
    clone->setDirty(m_dirty);
    return clone;
}

Todo::Todo(const QString &calId, const QString &itemUid, QObject *parent)
    : CalendarItem(calId, itemUid, parent)
{
}

QString Todo::type() const
{
    return "Todo";
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

QString Todo::toICal() const
{
    if (!m_incidence) return QString();
    KCalendarCore::ICalFormat format;
    KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
    tempCalendar->addIncidence(m_incidence);
    return format.toString(tempCalendar);
}

CalendarItem* Todo::clone(QObject *parent) const
{
    if (!m_incidence) return nullptr;
    Todo *clone = new Todo(m_calId, m_itemId.split("_").last(), parent);
    clone->setIncidence(KCalendarCore::Incidence::Ptr(m_incidence->clone()));
    clone->setLastModified(m_lastModified);
    clone->setEtag(m_etag);
    clone->setDirty(m_dirty);
    return clone;
}
