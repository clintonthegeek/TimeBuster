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
    connect(col, &Collection::idChanged, this, &CollectionManager::onCollectionIdChanged);

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

void CollectionManager::onCollectionIdChanged(const QString &oldId, const QString &newId)
{
    if (m_collections.contains(oldId)) {
        Collection *col = m_collections.take(oldId);
        m_collections[newId] = col;
        if (m_collectionBackends.contains(oldId)) {
            QList<SyncBackend*> backends = m_collectionBackends.take(oldId);
            m_collectionBackends[newId] = backends;
        }
    }
}

void CollectionManager::addBackend(const QString &collectionId, SyncBackend *backend)
{
    if (!m_collections.contains(collectionId)) {
        qDebug() << "CollectionManager: Cannot add backend to unknown collection" << collectionId;
        return;
    }
    m_collectionBackends[collectionId].append(backend);
    backend->setParent(this); // Take ownership
    qDebug() << "CollectionManager: Added backend to" << collectionId;
}

void CollectionManager::saveBackendConfig(const QString &collectionId, const QString &kalbPath)
{
    if (!m_collectionBackends.contains(collectionId)) {
        qDebug() << "CollectionManager: No backends to save for" << collectionId;
        return;
    }
    m_configManager->saveBackendConfig(collectionId, m_collectionBackends[collectionId], kalbPath);
    qDebug() << "CollectionManager: Saved backend config for" << collectionId << "to" << kalbPath;
}

QList<Collection*> CollectionManager::collections() const
{
    return m_collections.values();
}


void CollectionManager::onItemsFetched(Cal *cal, const QList<CalendarItem*> &items)
{
    if (!cal) {
        qDebug() << "CollectionManager: Received null Cal pointer";
        qDeleteAll(items);
        return;
    }

    QString calId = cal->id();
    qDebug() << "CollectionManager: onItemsFetched for Cal" << calId << "with" << items.size() << "items";

    // Find the collection containing this Cal
    Collection *col = qobject_cast<Collection*>(cal->parent());
    if (!col) {
        qDebug() << "CollectionManager: No parent collection found for Cal" << calId;
        qDeleteAll(items);
        return;
    }

    qDebug() << "CollectionManager: Received" << items.size() << "items for Cal" << calId;
    for (CalendarItem *item : items) {
        cal->addItem(item);
    }
    // No need to call refreshModel() since addItem already emits signals
    emit dataFetched(col);
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

QList<SyncBackend*> CollectionManager::loadBackendConfig(const QString &collectionId, const QString &kalbPath)
{
    return m_configManager->loadBackendConfig(collectionId, kalbPath);
}

void CollectionManager::setConfigBasePath(const QString &path)
{
    m_configManager->setBasePath(path);
}

void CollectionManager::setBackends(const QString &collectionId, const QList<SyncBackend*> &backends)
{
    m_collectionBackends[collectionId] = backends;
    for (SyncBackend *backend : backends) {
        backend->setParent(this); // Ownership
    }
    qDebug() << "CollectionManager: Set backends for" << collectionId;
}
