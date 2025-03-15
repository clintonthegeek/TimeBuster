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

    // Add the new LocalBackend to the collection's backends
    m_backends[collectionId].append(localBackend);
    localBackend->setParent(this);

    // Connect signals for future loads (though not needed for this operation)
    connect(localBackend, &SyncBackend::calendarsLoaded, this, &CollectionController::onCalendarsLoaded);
    connect(localBackend, &SyncBackend::itemsLoaded, this, &CollectionController::onItemsLoaded);
    connect(localBackend, &SyncBackend::dataLoaded, this, []() {
        qDebug() << "CollectionController: Data loaded from backend";
    });
    connect(localBackend, &SyncBackend::errorOccurred, this, [](const QString &error) {
        qDebug() << "CollectionController: Error from backend:" << error;
    });

    // Save the updated backend configuration
    ConfigManager configManager(this);
    configManager.saveBackendConfig(collectionId, m_backends[collectionId]);

    // Get the collection and its calendars
    Collection *collection = m_collections.value(collectionId);
    QList<Cal*> calendars = collection->calendars();

    // Store the calendars to the LocalBackend
    localBackend->storeCalendars(collectionId, calendars);

    // Store the items for each calendar
    for (Cal *cal : calendars) {
        localBackend->storeItems(cal, cal->items());
    }

    qDebug() << "CollectionController: Attached LocalBackend to collection" << collectionId << "with" << calendars.size() << "calendars";
}

const QMap<QString, QList<SyncBackend*>> &CollectionController::backends() const
{
    return m_backends;
}

void CollectionController::onCollectionIdChanged(const QString &newId)
{
    qDebug() << "CollectionController: Collection ID changed to" << newId;
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
