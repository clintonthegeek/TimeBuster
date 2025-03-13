#ifndef SYNCBACKEND_H
#define SYNCBACKEND_H

#include <QObject>
#include <QList>
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

    virtual QList<CalendarMetadata> fetchCalendars(const QString &collectionId) = 0;
    virtual void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) = 0;
    virtual void storeItems(Cal *cal, const QList<CalendarItem*> &items) = 0;
    virtual QList<CalendarItem*> fetchItems(Cal *cal) = 0;

signals:
    void calendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void itemsFetched(Cal *cal, QList<CalendarItem*> items);
    void dataFetched();
    void errorOccurred(const QString &error);

public slots:
    virtual void updateItem(const QString &calId, const QString &itemId, const QString &icalData) = 0;
};

#endif // SYNCBACKEND_H
