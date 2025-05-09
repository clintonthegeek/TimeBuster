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

    // Existing virtual functions for calendar loading and storage
    virtual QList<CalendarMetadata> loadCalendars(const QString &collectionId) = 0;
    virtual QList<QSharedPointer<CalendarItem>> loadItems(Cal *cal) = 0;
    virtual void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) = 0;
    virtual void storeItems(Cal *cal, const QList<QSharedPointer<CalendarItem>> &items) = 0;
    virtual void updateItem(const QString &calId, const QString &itemId, const QString &icalData) = 0;

    // New entry point for loading
    virtual void startSync(const QString &collectionId) = 0;

    // New virtual functions for version metadata and deletion
    virtual QString fetchItemVersionIdentifier(const QString &calId, const QString &itemId) = 0;
    virtual void removeItem(const QString &calId, const QString &itemId) = 0;

signals:
    // Existing error signal
    void errorOccurred(const QString &error);

    // Modified: itemLoaded signal now passes the versionIdentifier.
    void calendarDiscovered(const QString &collectionId, const CalendarMetadata &calendar);
    void itemLoaded(Cal *cal, QSharedPointer<CalendarItem> item, const QString &versionIdentifier);
    void calendarLoaded(Cal *cal);
    void syncCompleted(const QString &collectionId);
};

#endif // SYNCBACKEND_H
