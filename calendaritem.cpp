#include "calendaritem.h"
#include <QDateTime>
#include <QDebug>
#include <QString>

CalendarItem::CalendarItem(const QString &calId, const QString &itemId, QObject *parent)
    : QObject(parent), m_calId(calId), m_itemId(itemId), m_lastModified(QDateTime::currentDateTime())
{
}

void CalendarItem::setIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    m_incidence = incidence;
    if (incidence) {
        m_lastModified = QDateTime::currentDateTime();
    }
}

// --- Event ---
Event::Event(const QString &calId, const QString &itemId, QObject *parent)
    : CalendarItem(calId, itemId, parent)
{
}

QString Event::type() const { return "Event"; }

QVariant Event::data(int role) const
{
    if (!m_incidence) return QVariant();
    if (role == Qt::DisplayRole) return m_incidence->summary();
    if (role == Qt::UserRole) return m_incidence.staticCast<KCalendarCore::Event>()->dtStart().toString();
    if (role == Qt::UserRole + 1) return m_incidence.staticCast<KCalendarCore::Event>()->dtEnd().toString();
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
    // Replace setEtag with setVersionIdentifier
    clone->setVersionIdentifier(m_etag);
    clone->setDirty(m_dirty);
    return clone;
}


QDateTime Event::dtStart() const
{
    return m_incidence ? m_incidence.staticCast<KCalendarCore::Event>()->dtStart() : QDateTime();
}

void Event::setDtStart(const QDateTime &dtStart)
{
    if (m_incidence) {
        m_incidence.staticCast<KCalendarCore::Event>()->setDtStart(dtStart);
        setDirty(true);
    }
}

QDateTime Event::dtEndOrDue() const
{
    return m_incidence ? m_incidence.staticCast<KCalendarCore::Event>()->dtEnd() : QDateTime();
}

void Event::setDtEndOrDue(const QDateTime &dtEndOrDue)
{
    if (m_incidence) {
        m_incidence.staticCast<KCalendarCore::Event>()->setDtEnd(dtEndOrDue);
        setDirty(true);
    }
}

QStringList Event::categories() const
{
    return m_incidence ? m_incidence->categories() : QStringList();
}

void Event::setCategories(const QStringList &categories)
{
    if (m_incidence) {
        m_incidence->setCategories(categories);
        setDirty(true);
    }
}

QString Event::description() const
{
    return m_incidence ? m_incidence->description() : QString();
}

void Event::setDescription(const QString &description)
{
    if (m_incidence) {
        m_incidence->setDescription(description);
        setDirty(true);
    }
}

bool Event::allDay() const
{
    return m_incidence ? m_incidence.staticCast<KCalendarCore::Event>()->allDay() : false;
}

void Event::setAllDay(bool allDay)
{
    if (m_incidence) {
        m_incidence.staticCast<KCalendarCore::Event>()->setAllDay(allDay);
        setDirty(true);
    }
}

// --- Todo ---
Todo::Todo(const QString &calId, const QString &itemUid, QObject *parent)
    : CalendarItem(calId, itemUid, parent)
{
}

QString Todo::type() const { return "Todo"; }

QVariant Todo::data(int role) const
{
    if (!m_incidence) return QVariant();
    if (role == Qt::DisplayRole) return m_incidence->summary();
    if (role == Qt::UserRole) return m_incidence.staticCast<KCalendarCore::Todo>()->dtStart().toString();
    if (role == Qt::UserRole + 1) return m_incidence.staticCast<KCalendarCore::Todo>()->dtDue().toString();
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
    // Replace setEtag with setVersionIdentifier
    clone->setVersionIdentifier(m_etag);
    clone->setDirty(m_dirty);
    return clone;
}


QDateTime Todo::dtStart() const
{
    return m_incidence && m_incidence.staticCast<KCalendarCore::Todo>()->hasStartDate() ?
               m_incidence.staticCast<KCalendarCore::Todo>()->dtStart() : QDateTime();
}

void Todo::setDtStart(const QDateTime &dtStart)
{
    if (m_incidence) {
        m_incidence.staticCast<KCalendarCore::Todo>()->setDtStart(dtStart);
        setDirty(true);
    }
}

QDateTime Todo::dtEndOrDue() const
{
    return m_incidence && m_incidence.staticCast<KCalendarCore::Todo>()->hasDueDate() ?
               m_incidence.staticCast<KCalendarCore::Todo>()->dtDue() : QDateTime();
}

void Todo::setDtEndOrDue(const QDateTime &dtEndOrDue)
{
    if (m_incidence) {
        m_incidence.staticCast<KCalendarCore::Todo>()->setDtDue(dtEndOrDue);
        setDirty(true);
    }
}

QStringList Todo::categories() const
{
    return m_incidence ? m_incidence->categories() : QStringList();
}

void Todo::setCategories(const QStringList &categories)
{
    if (m_incidence) {
        m_incidence->setCategories(categories);
        setDirty(true);
    }
}

QString Todo::description() const
{
    return m_incidence ? m_incidence->description() : QString();
}

void Todo::setDescription(const QString &description)
{
    if (m_incidence) {
        m_incidence->setDescription(description);
        setDirty(true);
    }
}

bool Todo::allDay() const
{
    return m_incidence ? m_incidence.staticCast<KCalendarCore::Todo>()->allDay() : false;
}

void Todo::setAllDay(bool allDay)
{
    if (m_incidence) {
        m_incidence.staticCast<KCalendarCore::Todo>()->setAllDay(allDay);
        setDirty(true);
    }
}
