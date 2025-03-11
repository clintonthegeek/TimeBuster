#ifndef COLLECTIONMANAGER_H
#define COLLECTIONMANAGER_H

#include <QObject>
#include <QMap>
#include "collection.h"

class SyncBackend;
class ConfigManager; // New
class CalendarMetadata;

class CollectionManager : public QObject
{
    Q_OBJECT

public:
    explicit CollectionManager(QObject *parent = nullptr);
    ~CollectionManager() override;

    void addCollection(const QString &name, SyncBackend *initialBackend = nullptr);
    void addBackend(const QString &collectionId, SyncBackend *backend); // New
    void saveBackendConfig(const QString &collectionId, const QString &kalbPath = QString()); // New
    QList<SyncBackend*> loadBackendConfig(const QString &collectionId, const QString &kalbPath = QString()); // New
    void setConfigBasePath(const QString &path); // New
    Collection* collection(const QString &id) const;

    QMap<QString, QList<SyncBackend*>> backends() const { return m_collectionBackends; }
    void setBackends(const QString &collectionId, const QList<SyncBackend*> &backends); // New

    void onCalendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void onItemsFetched(const QString &calId, const QList<CalendarItem*> &items);


signals:
    void collectionAdded(Collection *collection);
    void dataFetched(Collection *collection);
    void syncFinished(const QString &collectionId, bool success);

private:
    QMap<QString, Collection*> m_collections;
    QMap<QString, QList<SyncBackend*>> m_collectionBackends;
    ConfigManager *m_configManager;
    int m_collectionCounter;
};


#endif // COLLECTIONMANAGER_H
