#ifndef SYNCBACKEND_H
#define SYNCBACKEND_H

#include <QObject>
#include <QList>
#include <QSharedPointer>
#include "calendaritem.h"

class Cal;

struct CalendarMetadata {
    QString id;
    QString name;
};

class SyncBackend : public QObject
{
    Q_OBJECT
public:
    explicit SyncBackend(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~SyncBackend() = default;

    // Pure virtual methods for backend implementations
    virtual QList<CalendarMetadata> loadCalendars(const QString &collectionId) = 0;
    virtual QList<QSharedPointer<CalendarItem>> loadItems(Cal *cal) = 0;
    virtual void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) = 0;
    virtual void storeItems(Cal *cal, const QList<QSharedPointer<CalendarItem>> &items) = 0;
    virtual void updateItem(const QString &calId, const QString &itemId, const QString &icalData) = 0;

signals:
    void calendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void itemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items);
    void dataLoaded();
    void errorOccurred(const QString &error);
};

#endif // SYNCBACKEND_H
