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

    connect(col, &Collection::calendarsChanged, this, [this, col]() {
        qDebug() << "CollectionManager: Calendars changed in" << col->id();
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
        // Connect common signals
        connect(initialBackend, &SyncBackend::calendarsFetched, this, &CollectionManager::onCalendarsFetched);
        connect(initialBackend, &SyncBackend::itemsFetched, this, &CollectionManager::onItemsFetched);
        connect(initialBackend, &SyncBackend::dataFetched, this, []() {
            qDebug() << "CollectionManager: Data fetched from backend";
        });
        connect(initialBackend, &SyncBackend::errorOccurred, this, [](const QString &error) {
            qDebug() << "CollectionManager: Error from backend:" << error;
        });

        // Trigger calendar fetch
        initialBackend->fetchCalendars(id);
    }

    for (Cal *cal : col->calendars()) {
        m_calMap[cal->id()] = cal;
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
    emit calendarsFetched(collectionId, calendars);
    for (Cal *cal : col->calendars()) {
        for (SyncBackend *backend : m_backends[collectionId]) {
            qDebug() << "CollectionManager: Triggering fetchItems for" << cal->id();
            backend->fetchItems(cal);
        }
    }
}

void CollectionManager::onItemsFetched(Cal *cal, QList<CalendarItem*> items)
{
    qDebug() << "CollectionManager: Received itemsFetched for" << cal->id() << "with" << items.size() << "items";
    for (CalendarItem *item : items) {
        cal->addItem(item);
    }
    emit itemsFetched(cal, items);
}
