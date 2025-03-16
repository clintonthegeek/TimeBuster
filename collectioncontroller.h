#ifndef COLLECTIONCONTROLLER_H
#define COLLECTIONCONTROLLER_H

#include <QObject>
#include <QMap>
#include "collection.h"
#include "syncbackend.h"

class ConfigManager;

class CollectionController : public QObject
{
    Q_OBJECT

public:
    explicit CollectionController(QObject *parent = nullptr);
    ~CollectionController() override;

    void addCollection(const QString &name, SyncBackend *initialBackend = nullptr);
    void attachLocalBackend(const QString &collectionId, SyncBackend *localBackend);
    const QMap<QString, QList<SyncBackend*>> &backends() const;
    Collection *collection(const QString &id) const { return m_collections.value(id); }
    Cal *getCal(const QString &calId) const { return m_calMap.value(calId); }

    bool loadCollection(const QString &kalbPath);
    bool saveCollection(const QString &kalbPath, const QString &collectionId);
    const QMap<QString, Collection*> &collections() const { return m_collections; }

signals:
    void collectionAdded(Collection *collection);
    void calendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void itemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items);

private slots:
    void onCalendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items);
    void onDataLoaded();

private:
    // Struct to track backend metadata
    struct BackendInfo {
        SyncBackend *backend;
        int priority;
        bool syncOnOpen;
    };

    QMap<QString, Collection*> m_collections;
    QMap<QString, QList<BackendInfo>> m_backends; // Now stores BackendInfo instead of raw SyncBackend*
    QMap<QString, Cal*> m_calMap;
    int m_collectionCounter;
};

#endif // COLLECTIONCONTROLLER_H
