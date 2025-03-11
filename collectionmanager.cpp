#include "collectionmanager.h"

CollectionManager::CollectionManager(QObject *parent)
    : QObject(parent), m_collectionCounter(0)
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
    } else {
        m_collectionBackends[id] = {}; // Empty list for now
    }

    emit collectionAdded(col);
}

Collection* CollectionManager::collection(const QString &id) const
{
    return m_collections.value(id, nullptr);
}
