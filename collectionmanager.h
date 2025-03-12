#ifndef COLLECTIONMANAGER_H
#define COLLECTIONMANAGER_H

#include <QObject>
#include <QMap>
#include "collection.h"

class SyncBackend;
class ConfigManager;
class CalendarMetadata;

class CollectionManager : public QObject
{
    Q_OBJECT

public:
    explicit CollectionManager(QObject *parent = nullptr);
    ~CollectionManager() override;

    void addCollection(const QString &name, SyncBackend *initialBackend = nullptr);
    void addBackend(const QString &collectionId, SyncBackend *backend);
    void saveBackendConfig(const QString &collectionId, const QString &kalbPath = QString());
    QList<SyncBackend*> loadBackendConfig(const QString &collectionId, const QString &kalbPath = QString());
    void setConfigBasePath(const QString &path);
    Collection* collection(const QString &id) const;
    QList<Collection*> collections() const; // New method to return all collections

    QMap<QString, QList<SyncBackend*>> backends() const { return m_collectionBackends; }
    void setBackends(const QString &collectionId, const QList<SyncBackend*> &backends);

signals:
    void collectionAdded(Collection *collection);
    void dataFetched(Collection *collection);
    void syncFinished(const QString &collectionId, bool success);

public slots:
    void onCalendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void onItemsFetched(Cal *cal, const QList<CalendarItem*> &items);

private:
    QMap<QString, Collection*> m_collections;
    QMap<QString, QList<SyncBackend*>> m_collectionBackends;
    ConfigManager *m_configManager;
    int m_collectionCounter;

    void onCollectionIdChanged(const QString &oldId, const QString &newId);
};

#endif // COLLECTIONMANAGER_H
