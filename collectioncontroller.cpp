#include "collectioncontroller.h"
#include "collection.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include "configmanager.h"
#include <QDebug>

CollectionController::CollectionController(QObject *parent)
    : QObject(parent), m_collectionCounter(0)
{
}

CollectionController::~CollectionController()
{
    qDeleteAll(m_collections);
    for (const QList<SyncBackend*> &backends : m_backends) {
        qDeleteAll(backends);
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
        m_backends[id].append(initialBackend);
        initialBackend->setParent(this);
        connect(initialBackend, &SyncBackend::calendarsLoaded, this, &CollectionController::onCalendarsLoaded);
        connect(initialBackend, &SyncBackend::itemsLoaded, this, &CollectionController::onItemsLoaded);
        connect(initialBackend, &SyncBackend::dataLoaded, this, []() {
            qDebug() << "CollectionController: Data loaded from backend";
        });
        connect(initialBackend, &SyncBackend::errorOccurred, this, [](const QString &error) {
            qDebug() << "CollectionController: Error from backend:" << error;
        });

        // Save the initial backend configuration
        ConfigManager configManager(this);
        configManager.saveBackendConfig(id, name, m_backends[id]);

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

    m_backends[collectionId].append(localBackend);
    localBackend->setParent(this);

    connect(localBackend, &SyncBackend::calendarsLoaded, this, &CollectionController::onCalendarsLoaded);
    connect(localBackend, &SyncBackend::itemsLoaded, this, &CollectionController::onItemsLoaded);
    connect(localBackend, &SyncBackend::dataLoaded, this, []() {
        qDebug() << "CollectionController: Data loaded from backend";
    });
    connect(localBackend, &SyncBackend::errorOccurred, this, [](const QString &error) {
        qDebug() << "CollectionController: Error from backend:" << error;
    });

    ConfigManager configManager(this);
    Collection *collection = m_collections.value(collectionId);
    configManager.saveBackendConfig(collectionId, collection->name(), m_backends[collectionId]);

    QList<Cal*> calendars = collection->calendars();
    localBackend->storeCalendars(collectionId, calendars);
    for (Cal *cal : calendars) {
        localBackend->storeItems(cal, cal->items());
    }

    qDebug() << "CollectionController: Attached LocalBackend to collection" << collectionId << "with" << calendars.size() << "calendars";
}

const QMap<QString, QList<SyncBackend*>> &CollectionController::backends() const
{
    return m_backends;
}

bool CollectionController::loadCollection(const QString &kalbPath)
{
    qDebug() << "CollectionController: Loading collection from" << kalbPath;
    ConfigManager configManager(this);
    QVariantMap config = configManager.loadConfig("", kalbPath); // Use empty collectionId to load from file path

    if (config.isEmpty()) {
        qDebug() << "CollectionController: Failed to load config from" << kalbPath;
        return false;
    }

    QString id = config["id"].toString();
    QString name = config["name"].toString();
    qDebug() << "CollectionController: Parsed collection id:" << id << "name:" << name;

    Collection *col = new Collection(id, name, this);
    m_collections.insert(id, col);

    QVariantList backendsList = config["backends"].toList();
    qDebug() << "CollectionController: Found" << backendsList.size() << "backends in config";
    for (const QVariant &backendVar : backendsList) {
        QVariantMap backendConfig = backendVar.toMap();
        QString backendType = backendConfig["type"].toString();
        qDebug() << "CollectionController: Processing backend type:" << backendType;
        if (backendType == "local") {
            QString rootPath = backendConfig["rootPath"].toString();
            qDebug() << "CollectionController: Creating LocalBackend with rootPath:" << rootPath;
            LocalBackend *localBackend = new LocalBackend(rootPath, this);
            m_backends[id].append(localBackend);
            localBackend->setParent(this);

            connect(localBackend, &SyncBackend::calendarsLoaded, this, &CollectionController::onCalendarsLoaded);
            connect(localBackend, &SyncBackend::itemsLoaded, this, &CollectionController::onItemsLoaded);
            connect(localBackend, &SyncBackend::dataLoaded, this, []() {
                qDebug() << "CollectionController: Data loaded from backend";
            });
            connect(localBackend, &SyncBackend::errorOccurred, this, [](const QString &error) {
                qDebug() << "CollectionController: Error from backend:" << error;
            });

            localBackend->loadCalendars(id);
        }
        // Add support for CalDAVBackend if needed later
    }

    qDebug() << "CollectionController: Backends for collection" << id << ":" << m_backends[id].size();
    for (Cal *cal : col->calendars()) {
        m_calMap[cal->id()] = cal;
    }

    qDebug() << "CollectionController: Loaded collection" << id << "with" << col->calendars().size() << "calendars";
    emit collectionAdded(col);
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
    configManager.saveBackendConfig(collectionId, col->name(), m_backends.value(collectionId), kalbPath);

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
    for (const CalendarMetadata &cal : calendars) {
        col->addCal(new Cal(cal.id, cal.name, col));
    }
    emit calendarsLoaded(collectionId, calendars);
    for (Cal *cal : col->calendars()) {
        for (SyncBackend *backend : m_backends[collectionId]) {
            qDebug() << "CollectionController: Triggering loadItems for" << cal->id();
            backend->loadItems(cal);
        }
    }
}

void CollectionController::onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items)
{
    qDebug() << "CollectionController: Received itemsLoaded for" << cal->id() << "with" << items.size() << "items";
    for (const QSharedPointer<CalendarItem> &item : items) {
        cal->addItem(item);
    }
    emit itemsLoaded(cal, items);
}
