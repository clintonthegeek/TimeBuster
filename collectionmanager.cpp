#include "collectionmanager.h"
#include "collection.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include <QDebug>

CollectionManager::CollectionManager(QObject *parent)
    : QObject(parent), m_collectionCounter(0)
{
}

CollectionManager::~CollectionManager()
{
    qDeleteAll(m_collections);
}

void CollectionManager::addCollection(const QString &name, SyncBackend *initialBackend)
{
    QString id = QString("col%1").arg(m_collectionCounter++);
    Collection *col = new Collection(id, name, this);
    m_collections.insert(id, col);

    connect(col, &Collection::calendarsChanged, this, [this]() {
        qDebug() << "CollectionManager: Calendars changed in a collection";
    });

    if (initialBackend) {
        m_backends[id].append(initialBackend);
        initialBackend->setParent(this);
        if (LocalBackend *local = qobject_cast<LocalBackend*>(initialBackend)) {
            QList<CalendarMetadata> calendars = local->fetchCalendars(id);
            for (const CalendarMetadata &cal : calendars) {
                col->addCal(new Cal(cal.id, cal.name, col));
            }
        } else if (CalDAVBackend *caldav = qobject_cast<CalDAVBackend*>(initialBackend)) {
            connect(caldav, &CalDAVBackend::calendarsFetched, this, &CollectionManager::onCalendarsFetched);
            caldav->fetchCalendars(id); // Async, handled in slot
        }
    }

    emit collectionAdded(col);
}

const QMap<QString, QList<SyncBackend*>> &CollectionManager::backends() const
{
    return m_backends;
}

void CollectionManager::onCollectionIdChanged(const QString &newId)
{
    qDebug() << "CollectionManager: Collection ID changed to" << newId;
}

void CollectionManager::onCalendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars)
{
    qDebug() << "CollectionManager: Received calendarsFetched for" << collectionId;
    Collection *col = m_collections.value(collectionId);
    if (!col) {
        qDebug() << "CollectionManager: Unknown collection" << collectionId;
        return;
    }
    for (const CalendarMetadata &cal : calendars) {
        col->addCal(new Cal(cal.id, cal.name, col));
    }
    // Trigger fetchItems for each new Cal
    for (Cal *cal : col->calendars()) {
        for (SyncBackend *backend : m_backends[collectionId]) {
            if (CalDAVBackend *caldav = qobject_cast<CalDAVBackend*>(backend)) {
                qDebug() << "CollectionManager: Triggering fetchItems for" << cal->id();
                caldav->fetchItems(cal);
            }
        }
    }
}
