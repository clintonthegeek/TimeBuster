#include "collectionmanager.h"
#include "configmanager.h"
#include "caldavbackend.h"
#include <QDebug>

CollectionManager::CollectionManager(QObject *parent)
    : QObject(parent), m_collectionCounter(0), m_configManager(new ConfigManager(this))
{
}

CollectionManager::~CollectionManager()
{
    qDeleteAll(m_collections);
    for (const QList<SyncBackend*> &backends : m_collectionBackends) {
        qDeleteAll(backends);
    }
}

void CollectionManager::addCollection(const QString &name, SyncBackend *initialBackend)
{
    QString id = QString("col%1").arg(m_collectionCounter++);
    Collection *col = new Collection(id, name, this);
    m_collections.insert(id, col);

    if (initialBackend) {
        m_collectionBackends[id] = { initialBackend };
        QList<SyncBackend*> backends = { initialBackend };
        m_configManager->saveBackendConfig(id, backends);

        if (CalDAVBackend *caldav = qobject_cast<CalDAVBackend*>(initialBackend)) {
            qDebug() << "CollectionManager: Connecting to CalDAVBackend for" << id;
            connect(caldav, &CalDAVBackend::calendarsFetched, this, &CollectionManager::onCalendarsFetched);
            connect(caldav, &CalDAVBackend::itemsFetched, this, &CollectionManager::onItemsFetched); // New
            connect(caldav, &CalDAVBackend::errorOccurred, this, [this, id](const QString &error) {
                qDebug() << "CollectionManager: CalDAV error for" << id << ":" << error;
            });
            caldav->fetchCalendars(id);
        }
    } else {
        m_collectionBackends[id] = {};
        emit collectionAdded(col);
    }
}

void CollectionManager::onItemsFetched(const QString &calId, const QList<CalendarItem*> &items)
{
    qDebug() << "CollectionManager: onItemsFetched for" << calId << "with" << items.size() << "items";
    for (Collection *col : m_collections) {
        if (Cal *cal = col->calendar(calId)) {
            for (CalendarItem *item : items) {
                cal->addItem(item);
            }
            qDebug() << "CollectionManager: Updated Cal" << calId << "with" << items.size() << "items";
            emit dataFetched(col);
            return;
        }
    }
    qDebug() << "CollectionManager: No calendar found for" << calId;
    qDeleteAll(items); // Clean up if no match
}

void CollectionManager::onCalendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars)
{
    qDebug() << "CollectionManager: onCalendarsFetched for" << collectionId << "with" << calendars.size() << "calendars";
    Collection *col = m_collections.value(collectionId);
    if (!col) {
        qDebug() << "CollectionManager: No collection found for" << collectionId;
        return;
    }

    for (const CalendarMetadata &meta : calendars) {
        qDebug() << "CollectionManager: Adding Cal" << meta.id << meta.name;
        Cal *cal = new Cal(meta.id, meta.name, col);
        col->addCal(cal);
    }
    qDebug() << "CollectionManager: Emitting collectionAdded for" << collectionId;
    emit collectionAdded(col);
}
