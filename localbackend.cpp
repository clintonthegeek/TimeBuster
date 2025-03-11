#include "localbackend.h"
#include "cal.h" // For Cal in storeCalendars

LocalBackend::LocalBackend(const QString &rootPath, QObject *parent)
    : SyncBackend(parent), m_rootPath(rootPath)
{
}

QList<CalendarMetadata> LocalBackend::fetchCalendars(const QString &collectionId)
{
    // Stubbed - pretend we scanned m_rootPath
    QList<CalendarMetadata> calendars;
    calendars.append(CalendarMetadata{ "cal1", "Local Calendar 1" }); // Explicit construction
    m_idToPath["cal1"] = m_rootPath + "/cal1"; // Fake path
    emit dataFetched();
    return calendars;
}

QList<CalendarItem*> LocalBackend::fetchItems(const QString &calId)
{
    // Stubbed - fake items for the calendar
    QList<CalendarItem*> items;
    if (calId == "cal1") {
        items.append(new Event("event1", this));
        items.append(new Todo("todo1", this));
    }
    emit dataFetched();
    return items;
}

void LocalBackend::storeCalendars(const QString &collectionId, const QList<Cal*> &calendars)
{
    // Stub - pretend we saved to m_rootPath
    for (Cal *cal : calendars) {
        m_idToPath[cal->id()] = m_rootPath + "/" + cal->id();
    }
}

void LocalBackend::storeItems(const QString &calId, const QList<CalendarItem*> &items)
{
    // Stub - no-op for now
    Q_UNUSED(calId);
    Q_UNUSED(items);
}
