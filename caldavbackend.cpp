#include "caldavbackend.h"
#include "cal.h" // For Cal in storeCalendars

CalDAVBackend::CalDAVBackend(const QString &serverUrl, const QString &username, const QString &password, QObject *parent)
    : SyncBackend(parent), m_serverUrl(serverUrl), m_username(username), m_password(password)
{
}

QList<CalendarMetadata> CalDAVBackend::fetchCalendars(const QString &collectionId)
{
    // Stubbed - pretend we fetched from m_serverUrl
    QList<CalendarMetadata> calendars;
    calendars.append(CalendarMetadata{ "cal1", "Remote Calendar 1" });
    m_idToUrl["cal1"] = m_serverUrl + "/cal1"; // Fake URL
    emit dataFetched();
    return calendars;
}

QList<CalendarItem*> CalDAVBackend::fetchItems(const QString &calId)
{
    // Stubbed - fake items for the calendar
    QList<CalendarItem*> items;
    if (calId == "cal1") {
        items.append(new Event("event2", this));
        items.append(new Todo("todo2", this));
    }
    emit dataFetched();
    return items;
}

void CalDAVBackend::storeCalendars(const QString &collectionId, const QList<Cal*> &calendars)
{
    // Stub - pretend we pushed to m_serverUrl
    for (Cal *cal : calendars) {
        m_idToUrl[cal->id()] = m_serverUrl + "/" + cal->id();
    }
}

void CalDAVBackend::storeItems(const QString &calId, const QList<CalendarItem*> &items)
{
    // Stub - no-op for now
    Q_UNUSED(calId);
    Q_UNUSED(items);
}
