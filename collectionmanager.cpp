#include "collectionmanager.h"
#include "configmanager.h" // New

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
    // m_configManager deleted by Qt as a child
}

void CollectionManager::addCollection(const QString &name, SyncBackend *initialBackend)
{
    QString id = QString("col%1").arg(m_collectionCounter++);
    Collection *col = new Collection(id, name, this);
    m_collections.insert(id, col);

    if (initialBackend) {
        m_collectionBackends[id] = { initialBackend };
        QList<SyncBackend*> backends = { initialBackend };
        m_configManager->saveBackendConfig(id, backends); // Save config
    } else {
        m_collectionBackends[id] = {};
    }

    emit collectionAdded(col);
}

Collection* CollectionManager::collection(const QString &id) const
{
    return m_collections.value(id, nullptr);
}
