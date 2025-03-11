#ifndef COLLECTIONMANAGER_H
#define COLLECTIONMANAGER_H

#include <QObject>
#include <QMap>
#include "collection.h"

class SyncBackend;
class ConfigManager; // New

class CollectionManager : public QObject
{
    Q_OBJECT

public:
    explicit CollectionManager(QObject *parent = nullptr);
    ~CollectionManager() override;

    void addCollection(const QString &name, SyncBackend *initialBackend = nullptr);
    Collection* collection(const QString &id) const;

signals:
    void collectionAdded(Collection *collection);
    void dataFetched(Collection *collection);
    void syncFinished(const QString &collectionId, bool success);

private:
    QMap<QString, Collection*> m_collections;
    QMap<QString, QList<SyncBackend*>> m_collectionBackends;
    ConfigManager *m_configManager; // New - owned
    int m_collectionCounter;
};

#endif // COLLECTIONMANAGER_H
