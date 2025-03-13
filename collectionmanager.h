#ifndef COLLECTIONMANAGER_H
#define COLLECTIONMANAGER_H

#include <QObject>
#include <QMap>
#include "syncbackend.h"

class Collection;

class CollectionManager : public QObject
{
    Q_OBJECT
public:
    explicit CollectionManager(QObject *parent = nullptr);
    ~CollectionManager() override;
    void addCollection(const QString &name, SyncBackend *initialBackend = nullptr);
    const QMap<QString, QList<SyncBackend*>> &backends() const;
    const QMap<QString, Collection*> &collections() const { return m_collections; }

signals:
    void collectionAdded(Collection *collection);
    void calendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars); // Forwarded
    void itemsFetched(Cal *cal, QList<CalendarItem*> items); // Forwarded

private slots:
    void onCollectionIdChanged(const QString &newId);
    void onCalendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void onItemsFetched(Cal *cal, QList<CalendarItem*> items);

private:
    QMap<QString, Collection*> m_collections;
    QMap<QString, QList<SyncBackend*>> m_backends;
    int m_collectionCounter;
};

#endif // COLLECTIONMANAGER_H
