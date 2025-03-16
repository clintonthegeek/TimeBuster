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
    qDeleteAll(m_collections);
    for (const QList<BackendInfo> &backendList : m_backends) {
        for (const BackendInfo &info : backendList) {
            delete info.backend;
        }
    }
}

void CollectionController::addCollection(const QString &name, SyncBackend *initialBackend)
{
    QString id = QString("col%1").arg(m_collectionCounter++);
    Collection *col = new Collection(id, name, this);
    m_collections.insert(id, col);

    connect(col, &Collection::calendarsChanged, this, [this, col]() {
        qDebug() << "CollectionController: Calendars changed in" << col->id();
        m_calMap.clear();
        for (Collection *c : m_collections) {
            for (Cal *cal : c->calendars()) {
                m_calMap[cal->id()] = cal;
            }
        }
    });

    if (initialBackend) {
        BackendInfo info;
        info.backend = initialBackend;
        info.priority = 1; // Default priority
        info.syncOnOpen = false; // Default no sync
        m_backends[id].append(info);
        initialBackend->setParent(this);
        connect(initialBackend, &SyncBackend::calendarsLoaded, this, &CollectionController::onCalendarsLoaded);
        connect(initialBackend, &SyncBackend::itemsLoaded, this, &CollectionController::onItemsLoaded);
        connect(initialBackend, &SyncBackend::dataLoaded, this, &CollectionController::onDataLoaded);
        connect(initialBackend, &SyncBackend::errorOccurred, this, [](const QString &error) {
            qDebug() << "CollectionController: Error from backend:" << error;
        });

        // Save the initial backend configuration
        ConfigManager configManager(this);
        configManager.saveBackendConfig(id, name, {initialBackend});

        initialBackend->loadCalendars(id);
    }

    for (Cal *cal : col->calendars()) {
        m_calMap[cal->id()] = cal;
    }

    emit collectionAdded(col);
}

void CollectionController::attachLocalBackend(const QString &collectionId, SyncBackend *localBackend)
{
    if (!m_collections.contains(collectionId)) {
        qDebug() << "CollectionController: Cannot attach backend, collection" << collectionId << "not found";
        return;
    }

    if (!localBackend) {
        qDebug() << "CollectionController: Cannot attach null backend to collection" << collectionId;
        return;
    }

    BackendInfo info;
    info.backend = localBackend;
    info.priority = 1; // Default priority for local
    info.syncOnOpen = false; // Default no sync
    m_backends[collectionId].append(info);
    localBackend->setParent(this);

    connect(localBackend, &SyncBackend::calendarsLoaded, this, &CollectionController::onCalendarsLoaded);
    connect(localBackend, &SyncBackend::itemsLoaded, this, &CollectionController::onItemsLoaded);
    connect(localBackend, &SyncBackend::dataLoaded, this, &CollectionController::onDataLoaded);
    connect(localBackend, &SyncBackend::errorOccurred, this, [](const QString &error) {
        qDebug() << "CollectionController: Error from backend:" << error;
    });

    ConfigManager configManager(this);
    Collection *collection = m_collections.value(collectionId);
    QList<SyncBackend*> backendsToSave;
    for (const BackendInfo &bInfo : m_backends[collectionId]) {
        backendsToSave.append(bInfo.backend);
    }
    configManager.saveBackendConfig(collectionId, collection->name(), backendsToSave);

    QList<Cal*> calendars = collection->calendars();
    localBackend->storeCalendars(collectionId, calendars);
    for (Cal *cal : calendars) {
        localBackend->storeItems(cal, cal->items());
    }

    qDebug() << "CollectionController: Attached LocalBackend to collection" << collectionId << "with" << calendars.size() << "calendars";
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

bool CollectionController::loadCollection(const QString &kalbPath)
{
    ConfigManager configManager(this);
    QVariantMap config = configManager.loadConfig(QString(), kalbPath);
    if (config.isEmpty()) {
        qDebug() << "CollectionController: Failed to load config from" << kalbPath;
        return false;
    }

    QString collectionId = config.value("id").toString();
    QString collectionName = config.value("name").toString();
    qDebug() << "CollectionController: Parsed collection id:" << collectionId << "name:" << collectionName;

    // Process backends
    QVariantList backendsList = config.value("backends").toList();
    qDebug() << "CollectionController: Found" << backendsList.size() << "backends in config";
    QList<BackendInfo> backends;
    for (const QVariant &backendVar : backendsList) {
        ConfigManager::BackendConfig backendConfig = backendVar.value<ConfigManager::BackendConfig>();
        QString type = backendConfig.type;
        qDebug() << "CollectionController: Processing backend type:" << type << "priority:" << backendConfig.priority << "syncOnOpen:" << backendConfig.syncOnOpen;

        BackendInfo info;
        info.priority = backendConfig.priority;
        info.syncOnOpen = backendConfig.syncOnOpen;

        if (type == "local") {
            QString rootPath = backendConfig.details.value("rootPath").toString();
            qDebug() << "CollectionController: Creating LocalBackend with rootPath:" << rootPath;
            info.backend = new LocalBackend(rootPath, this);
        } else if (type == "caldav") {
            QString serverUrl = backendConfig.details.value("serverUrl").toString();
            QString username = backendConfig.details.value("username").toString();
            QString password = backendConfig.details.value("password").toString();
            info.backend = new CalDAVBackend(serverUrl, username, password, this);
        } else {
            qDebug() << "CollectionController: Unknown backend type:" << type;
            continue;
        }

        backends.append(info);
    }

    if (backends.isEmpty()) {
        qDebug() << "CollectionController: No backends found in config";
        return false;
    }

    // Sort backends by priority (ascending order: lower priority value = higher precedence)
    std::sort(backends.begin(), backends.end(), [](const BackendInfo &a, const BackendInfo &b) {
        return a.priority < b.priority;
    });

    Collection *collection = new Collection(collectionId, collectionName, this);
    m_collections.insert(collectionId, collection);
    m_backends.insert(collectionId, backends);

    // Connect signals for all backends
    for (BackendInfo &info : backends) {
        SyncBackend *backend = info.backend;
        connect(backend, &SyncBackend::calendarsLoaded, this, &CollectionController::onCalendarsLoaded);
        connect(backend, &SyncBackend::itemsLoaded, this, &CollectionController::onItemsLoaded);
        connect(backend, &SyncBackend::dataLoaded, this, &CollectionController::onDataLoaded);
        connect(backend, &SyncBackend::errorOccurred, this, [](const QString &error) {
            qDebug() << "CollectionController: Error from backend:" << error;
        });
    }

    // Load calendars based on priority and SyncOnOpen
    bool loaded = false;
    for (BackendInfo &info : backends) {
        bool isLocalBackend = dynamic_cast<LocalBackend*>(info.backend) != nullptr;
        if (isLocalBackend || info.syncOnOpen) {
            qDebug() << "CollectionController: Loading calendars from backend with priority" << info.priority << "syncOnOpen:" << info.syncOnOpen;
            info.backend->loadCalendars(collectionId);
            loaded = true;
        } else {
            qDebug() << "CollectionController: Skipping load from backend with priority" << info.priority << "syncOnOpen:" << info.syncOnOpen;
        }
    }

    if (!loaded) {
        qDebug() << "CollectionController: No backends were loaded (none had SyncOnOpen=true and no LocalBackend present)";
        return false;
    }

    qDebug() << "CollectionController: Backends for collection" << collectionId << ":" << m_backends.value(collectionId).size();
    qDebug() << "CollectionController: Loaded collection" << collectionId << "with" << collection->calendars().size() << "calendars";
    return true;
}

bool CollectionController::saveCollection(const QString &kalbPath, const QString &collectionId)
{
    qDebug() << "CollectionController: Saving collection" << collectionId << "to" << kalbPath;
    if (!m_collections.contains(collectionId)) {
        qDebug() << "CollectionController: Collection" << collectionId << "not found";
        return false;
    }

    Collection *col = m_collections.value(collectionId);
    ConfigManager configManager(this);
    QList<SyncBackend*> backendsToSave;
    for (const BackendInfo &info : m_backends.value(collectionId)) {
        backendsToSave.append(info.backend);
    }
    configManager.saveBackendConfig(collectionId, col->name(), backendsToSave, kalbPath);

    qDebug() << "CollectionController: Saved collection" << collectionId << "successfully";
    return true;
}

void CollectionController::onCalendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars)
{
    qDebug() << "CollectionController: Received calendarsLoaded for" << collectionId;
    Collection *col = m_collections.value(collectionId);
    if (!col) {
        qDebug() << "CollectionController: Unknown collection" << collectionId;
        return;
    }

    // Only add calendars if they haven't been added yet
    QSet<QString> existingCalIds;
    for (Cal *cal : col->calendars()) {
        existingCalIds.insert(cal->id());
    }

    for (const CalendarMetadata &cal : calendars) {
        if (!existingCalIds.contains(cal.id)) {
            col->addCal(new Cal(cal.id, cal.name, col));
            existingCalIds.insert(cal.id);
        }
    }

    emit calendarsLoaded(collectionId, calendars);

    // Load items only for calendars that haven't had items loaded yet
    for (Cal *cal : col->calendars()) {
        if (cal->items().isEmpty()) { // Only load items if not already loaded
            for (const BackendInfo &info : m_backends[collectionId]) {
                bool isLocalBackend = dynamic_cast<LocalBackend*>(info.backend) != nullptr;
                if (isLocalBackend || info.syncOnOpen) {
                    qDebug() << "CollectionController: Triggering loadItems for" << cal->id() << "from backend with priority" << info.priority;
                    info.backend->loadItems(cal);
                }
            }
        } else {
            qDebug() << "CollectionController: Skipping loadItems for" << cal->id() << "(items already loaded)";
        }
    }
}

void CollectionController::onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items)
{
    qDebug() << "CollectionController: Received itemsLoaded for" << cal->id() << "with" << items.size() << "items";

    // Add items, avoiding duplicates
    QSet<QString> existingItemIds;
    for (const QSharedPointer<CalendarItem> &item : cal->items()) {
        existingItemIds.insert(item->id());
    }

    for (const QSharedPointer<CalendarItem> &item : items) {
        if (!existingItemIds.contains(item->id())) {
            cal->addItem(item);
            existingItemIds.insert(item->id());
        }
    }

    emit itemsLoaded(cal, cal->items());
}

void CollectionController::onDataLoaded()
{
    qDebug() << "CollectionController: Data loaded from backend";
}
