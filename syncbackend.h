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
    explicit SyncBackend(QObject *parent = nullptr);
    virtual ~SyncBackend() override = default;

    // Pure virtual methods - stubs in derived classes
    virtual QList<CalendarMetadata> fetchCalendars(const QString &collectionId) = 0;
    virtual QList<CalendarItem*> fetchItems(const QString &calId) = 0;
    virtual void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) = 0;
    virtual void storeItems(const QString &calId, const QList<CalendarItem*> &items) = 0;

signals:
    void dataFetched();
    void errorOccurred(const QString &error);
};

#endif // SYNCBACKEND_H
