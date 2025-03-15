#ifndef COLLECTIONCONTROLLER_H
#define COLLECTIONCONTROLLER_H

#include <QObject>
#include <QMap>
#include "syncbackend.h"

class Collection;
class Cal;

class CollectionController : public QObject
{
    Q_OBJECT
public:
    explicit CollectionController(QObject *parent = nullptr);
    ~CollectionController() override;
    void addCollection(const QString &name, SyncBackend *initialBackend = nullptr);
    void attachLocalBackend(const QString &collectionId, SyncBackend *localBackend); // New
    const QMap<QString, QList<SyncBackend*>> &backends() const;
    const QMap<QString, Collection*> &collections() const { return m_collections; }
    Cal* getCal(const QString &calId) const { return m_calMap.value(calId); }

signals:
    void collectionAdded(Collection *collection);
    void calendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void itemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items);

private slots:
    void onCollectionIdChanged(const QString &newId);
    void onCalendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items);

private:
    QMap<QString, Collection*> m_collections;
    QMap<QString, QList<SyncBackend*>> m_backends;
    QMap<QString, Cal*> m_calMap; // Non-owning pointers
    int m_collectionCounter;
};

#endif // COLLECTIONCONTROLLER_H
