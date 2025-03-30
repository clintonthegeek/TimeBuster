#ifndef COLLECTIONCONTROLLER_H
#define COLLECTIONCONTROLLER_H

#include <QObject>
#include <QMap>
#include "collection.h"
#include "syncbackend.h"
#include "backendinfo.h"

class CollectionController : public QObject
{
    Q_OBJECT

public:
    explicit CollectionController(QObject *parent = nullptr);
    ~CollectionController() override;

    const QMap<QString, QList<SyncBackend*>> &backends() const;
    const QMap<QString, Collection*> &collections() const { return m_collections; }
    Collection *collection(const QString &id) const { return m_collections.value(id); }
    Cal *getCal(const QString &calId) const { return m_calMap.value(calId); }
    bool isTransient(const QString &collectionId) const;
    QString kalbPath(const QString &collectionId) const { return m_collectionToKalbPath.value(collectionId); }

    void loadCollection(const QString &name, SyncBackend *initialBackend = nullptr, bool isTransient = false, const QString &kalbPath = QString());
    bool saveCollection(const QString &collectionId, const QString &kalbPath = QString());
    void attachLocalBackend(const QString &collectionId, SyncBackend *localBackend);

signals:
    void collectionAdded(Collection *collection);
    void calendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void itemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items); // Legacy
    void allSyncsCompleted(const QString &collectionId); // New signal
    void calendarAdded(Cal *cal);
    void itemAdded(Cal *cal, QSharedPointer<CalendarItem> item);
    void calendarLoaded(Cal *cal);
    void loadingProgress(int progress);
    void allBackendsCompleted(const QString &collectionId);

private slots:
    void onCalendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items);
    void onDataLoaded();
    void onCalendarDiscovered(const QString &collectionId, const CalendarMetadata &calendar);
    void onItemLoaded(Cal *cal, QSharedPointer<CalendarItem> item);
    void onCalendarLoaded(Cal *cal);
    void onSyncCompleted(const QString &collectionId);

private:
    QMap<QString, Collection*> m_collections;
    QMap<QString, QList<BackendInfo>> m_backends;
    QMap<QString, Cal*> m_calMap;
    QMap<QString, int> m_pendingDataLoads;
    QMap<QString, int> m_pendingSyncs;
    int m_collectionCounter;
    QMap<QString, QString> m_collectionToKalbPath;
};

#endif // COLLECTIONCONTROLLER_H
