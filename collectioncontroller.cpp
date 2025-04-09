#include "collectioncontroller.h"
#include "collection.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include "configmanager.h"
#include <QDebug>
#include <algorithm> // For std::sort

CollectionController::CollectionController(QObject *parent)
    : QObject(parent), m_collectionCounter(0)
{
}

CollectionController::~CollectionController()
{
    qDebug() << "CollectionController: Destroying CollectionController";
    for (const QString &id : m_backends.keys()) {
        for (const BackendInfo &info : m_backends[id]) {
            SyncBackend *backend = info.backend;
            disconnect(backend, nullptr, this, nullptr); // Disconnect all signals
            delete backend; // Delete backends explicitly
        }
    }
    m_backends.clear();
    qDeleteAll(m_collections); // Delete collections after backends
    m_collections.clear();
}

void CollectionController::loadCollection(const QString &name, SyncBackend *initialBackend, bool isTransient, const QString &kalbPath)
{
    QString id = QString("col%1").arg(m_collectionCounter++);

    // Load from .kalb if provided
    if (!kalbPath.isEmpty()) {
        ConfigManager configManager(this);
        QVariantMap config = configManager.loadConfig(id, kalbPath);
        if (!config.isEmpty()) {
            id = config["id"].toString();
            QString loadedName = config["name"].toString();
            QVariantList backendsList = config["backends"].toList();
            QList<BackendInfo> loadedBackends;
            for (const QVariant &backendVar : backendsList) {
                ConfigManager::BackendConfig backendConfig = backendVar.value<ConfigManager::BackendConfig>();
                BackendInfo info;
                info.priority = backendConfig.priority;
                info.syncOnOpen = backendConfig.syncOnOpen;
                if (backendConfig.type == "local") {
                    info.backend = new LocalBackend(backendConfig.details["rootPath"].toString(), this);
                } else if (backendConfig.type == "caldav") {
                    info.backend = new CalDAVBackend(
                        backendConfig.details["serverUrl"].toString(),
                        backendConfig.details["username"].toString(),
                        backendConfig.details["password"].toString(),
                        this
                        );
                }
                loadedBackends.append(info);
            }
            m_backends[id] = loadedBackends;
            isTransient = false; // Loaded collections are persistent
            m_collectionToKalbPath[id] = kalbPath;
            qDebug() << "CollectionController: Parsed collection id:" << id << "name:" << loadedName;
            Collection *col = new Collection(id, loadedName, this);
            m_collections.insert(id, col);
            connect(col, &Collection::calendarsChanged, this, [this]() {
                m_calMap.clear();
                for (Collection *c : m_collections) {
                    for (Cal *cal : c->calendars()) {
                        m_calMap[cal->id()] = cal;
                    }
                }
            });
            emit collectionAdded(col);
        } else {
            qDebug() << "CollectionController: Failed to load config from" << kalbPath;
            return;
        }
    } else {
        // Create new collection if no .kalb path
        Collection *col = new Collection(id, name, this);
        m_collections.insert(id, col);
        connect(col, &Collection::calendarsChanged, this, [this]() {
            m_calMap.clear();
            for (Collection *c : m_collections) {
                for (Cal *cal : c->calendars()) {
                    m_calMap[cal->id()] = cal;
                }
            }
        });

        // Add initial backend if provided
        if (initialBackend) {
            BackendInfo info;
            info.backend = initialBackend;
            info.priority = 1;
            info.syncOnOpen = true;
            m_backends[id].append(info);
            initialBackend->setParent(this);
            if (!isTransient) { // Only save if not transient
                ConfigManager configManager(this);
                QList<BackendInfo> backendList = {info};
                QString savedPath = configManager.saveBackendConfig(id, name, backendList);
                if (!savedPath.isEmpty()) {
                    m_collectionToKalbPath[id] = savedPath;
                } else {
                    qDebug() << "CollectionController: Failed to save non-transient collection" << id;
                }
            }
        }
        emit collectionAdded(col);
    }

    // Connect signals and start sync for all backends with syncOnOpen
    int syncCount = 0;
    for (BackendInfo &info : m_backends[id]) {
        SyncBackend *backend = info.backend;
        connect(backend, &SyncBackend::errorOccurred, this, [](const QString &error) {
            qDebug() << "CollectionController: Error from backend:" << error;
        });
        if (info.syncOnOpen) {
            connect(backend, &SyncBackend::calendarDiscovered, this, &CollectionController::onCalendarDiscovered);
            connect(backend, &SyncBackend::itemLoaded, this, &CollectionController::onItemLoaded);
            connect(backend, &SyncBackend::calendarLoaded, this, &CollectionController::onCalendarLoaded);
            connect(backend, &SyncBackend::syncCompleted, this, &CollectionController::onSyncCompleted);
            syncCount++;
            backend->startSync(id);
        }
    }
    if (syncCount > 0) {
        m_pendingSyncs[id] = syncCount;
    }
}

void CollectionController::unloadCollection(const QString &collectionId)
{
    Collection *collection = m_collections.value(collectionId);
    if (!collection) {
        qDebug() << "CollectionController: No collection found for" << collectionId << "to unload";
        return;
    }

    // Clear calendars from m_calMap and let QSharedPointer handle deletion
    for (Cal *cal : collection->calendars()) {
        m_calMap.remove(cal->id());
    }
    collection->clearCalendars(); // Clear the list, letting QSharedPointer delete the Cal objects

    // Remove the collection from m_collections
    m_collections.remove(collectionId);
    delete collection;

    qDebug() << "CollectionController: Unloaded collection" << collectionId;
}

bool CollectionController::saveCollection(const QString &collectionId, const QString &kalbPath)
{
    if (!m_collections.contains(collectionId)) {
        qDebug() << "CollectionController: Collection" << collectionId << "not found";
        return false;
    }

    Collection *col = m_collections.value(collectionId);
    QList<BackendInfo> backends = m_backends.value(collectionId);

    ConfigManager configManager(this);
    QString savedPath = configManager.saveBackendConfig(collectionId, col->name(), backends, kalbPath);
    if (!savedPath.isEmpty()) {
        m_collectionToKalbPath[collectionId] = savedPath;
        qDebug() << "CollectionController: Saved collection" << collectionId << "to" << savedPath;
        return true;
    } else {
        qDebug() << "CollectionController: Failed to save collection" << collectionId;
        return false;
    }
}

bool CollectionController::isTransient(const QString &collectionId) const
{
    return !m_collectionToKalbPath.contains(collectionId);
}

void CollectionController::attachLocalBackend(const QString &collectionId, SyncBackend *localBackend)
{
    if (!m_collections.contains(collectionId)) {
        qDebug() << "CollectionController: Cannot attach backend, collection" << collectionId << "not found";
        return;
    }

    BackendInfo info;
    info.backend = localBackend;
    info.priority = 2;
    info.syncOnOpen = false;
    m_backends[collectionId].append(info);
    localBackend->setParent(this);

    connect(localBackend, &SyncBackend::errorOccurred, this, [](const QString &error) {
        qDebug() << "CollectionController: Error from backend:" << error;
    });

    Collection *col = m_collections.value(collectionId);
    ConfigManager configManager(this);
    QString existingPath = m_collectionToKalbPath.value(collectionId);
    QString updatedPath = configManager.saveBackendConfig(collectionId, col->name(), m_backends[collectionId], existingPath);
    if (!updatedPath.isEmpty()) {
        m_collectionToKalbPath[collectionId] = updatedPath;
        qDebug() << "CollectionController: Updated .kalb with local backend at" << updatedPath;
    } else {
        qDebug() << "CollectionController: Failed to update .kalb for" << collectionId;
        return;
    }

    QList<Cal*> calendars = col->calendars();
    localBackend->storeCalendars(collectionId, calendars);
    for (Cal *cal : calendars) {
        localBackend->storeItems(cal, cal->items());
    }
    qDebug() << "CollectionController: Synced" << calendars.size() << "calendars to local backend";
}

const QMap<QString, QList<SyncBackend*>> &CollectionController::backends() const
{
    static QMap<QString, QList<SyncBackend*>> rawBackends;
    rawBackends.clear();
    for (const QString &collectionId : m_backends.keys()) {
        QList<SyncBackend*> backendList;
        for (const BackendInfo &info : m_backends[collectionId]) {
            backendList.append(info.backend);
        }
        rawBackends[collectionId] = backendList;
    }
    return rawBackends;
}

void CollectionController::onDataLoaded()
{
    qDebug() << "CollectionController: Data loaded from backend (legacy signal)";
    QList<QString> collectionIds = m_pendingDataLoads.keys();
    for (const QString &collectionId : collectionIds) {
        m_pendingDataLoads[collectionId]--;
        if (m_pendingDataLoads[collectionId] <= 0) {
            qDebug() << "CollectionController: Legacy data load complete for" << collectionId;
            m_pendingDataLoads.remove(collectionId);
        }
    }
}

void CollectionController::onCalendarDiscovered(const QString &collectionId, const CalendarMetadata &calendar)
{
    Collection *col = m_collections.value(collectionId);
    if (!col) {
        qDebug() << "CollectionController: No collection for" << collectionId;
        return;
    }

    Cal *cal = nullptr;
    Cal *existingCal = m_calMap.value(calendar.id);
    if (existingCal) {
        qDebug() << "CollectionController: Updating existing calendar" << calendar.id;
        existingCal->setName(calendar.name);
        cal = existingCal;
    } else {
        cal = new Cal(calendar.id, calendar.name, col);
        col->addCal(cal);
        m_calMap[calendar.id] = cal;
        qDebug() << "CollectionController: Added calendar" << calendar.id << "to" << collectionId;
    }

    // Emit calendarAdded with the Cal* object
    emit calendarAdded(cal);
}

void CollectionController::onItemLoaded(Cal *tempCal, QSharedPointer<CalendarItem> item)
{
    Cal *realCal = m_calMap.value(tempCal->id());
    if (!realCal) {
        qWarning() << "CollectionController: No real Cal for" << tempCal->id() << "on item load";
        return;
    }
    realCal->addItem(item);

    // If the item is not dirty, fetch the version identifier from the backend.
    if (!item->isDirty()) {
        // For simplicity, assume a single backend per collection.
        // Extract collection ID from the cal's id (assuming format "col0_calendarName").
        QString collectionId = realCal->id().split("_").first();
        QList<BackendInfo> backends = m_backends.value(collectionId);
        if (!backends.isEmpty()) {
            SyncBackend *backend = backends.first().backend;
            QString newVer = backend->fetchItemVersionIdentifier(realCal->id(), item->id());
            QString oldVer = item->versionIdentifier();
            if (!oldVer.isEmpty() && oldVer != newVer) {
                qDebug() << "CollectionController: Conflict detected for item" << item->id()
                << ": stored version =" << oldVer << ", new version =" << newVer;
                item->setConflictStatus(CalendarItem::ConflictStatus::Pending);
            } else {
                item->setConflictStatus(CalendarItem::ConflictStatus::None);
            }
            item->setVersionIdentifier(newVer);
            qDebug() << "CollectionController: Set versionIdentifier for item"
                     << item->id() << "to" << newVer;
        }
    }
    emit itemAdded(realCal, item);
}



void CollectionController::onCalendarLoaded(Cal *tempCal)
{
    Cal *realCal = m_calMap.value(tempCal->id());
    if (!realCal) {
        qWarning() << "CollectionController: No real Cal for" << tempCal->id() << "on calendar load";
        return;
    }
    qDebug() << "CollectionController: Calendar loaded:" << realCal->id();
    emit calendarLoaded(realCal);
}

void CollectionController::onSyncCompleted(const QString &collectionId)
{
    m_pendingSyncs[collectionId]--;
    if (m_pendingSyncs[collectionId] <= 0) {
        qDebug() << "CollectionController: All backends completed for" << collectionId;
        m_pendingSyncs.remove(collectionId);
        emit loadingProgress(100); // Stub for Stage 1
        emit allSyncsCompleted(collectionId); // Emit new signal
    }
}



void CollectionController::onCalendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars)
{
    Q_UNUSED(collectionId);
    Q_UNUSED(calendars);
    qDebug() << "CollectionController: Ignoring legacy calendarsLoaded for" << collectionId;
    // Do nothing—let onCalendarDiscovered handle it
}

void CollectionController::onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items)
{
    Q_UNUSED(cal);
    Q_UNUSED(items);
    qDebug() << "CollectionController: Ignoring legacy itemsLoaded for" << cal->id();
    // Do nothing—let onItemLoaded handle it
}

