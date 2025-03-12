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

void CollectionManager::onItemsFetched(const QString &calId, const QList<CalendarItem*> &items)
{
    if (m_collections.isEmpty()) {
        qDebug() << "CollectionManager: No collections available";
        return;
    }

    for (auto it = m_collections.constBegin(); it != m_collections.constEnd(); ++it) {
        Collection *col = it.value();
        Cal *cal = col->calendar(calId); // Look up the Cal by ID
        if (cal) { // If found, the calendar exists in this collection
            qDebug() << "CollectionManager: Received" << items.size() << "items for Cal" << calId;
            cal->refreshModel(); // Notify views to update
            emit dataFetched(col); // Notify MainWindow or other listeners
            break;
        }
    }
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
