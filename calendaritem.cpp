#include "calendaritem.h"

CalendarItem::CalendarItem(const QString &uid, QObject *parent)
    : QObject(parent), m_uid(uid)
{
}

Event::Event(const QString &uid, QObject *parent)
    : CalendarItem(uid, parent), m_summary("Stub Event")
{
}

Todo::Todo(const QString &uid, QObject *parent)
    : CalendarItem(uid, parent), m_summary("Stub Todo")
{
}
