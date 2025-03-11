#ifndef CALDAVBACKEND_H
#define CALDAVBACKEND_H

#include "syncbackend.h"
#include <QMap>
#include <KDAV/DavCollectionsFetchJob>
#include <KDAV/DavItemFetchJob>

class CalDAVBackend : public SyncBackend
{
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl)
    Q_PROPERTY(QString username READ username)
    Q_PROPERTY(QString password READ password)

public:
    explicit CalDAVBackend(const QString &serverUrl, const QString &username, const QString &password, QObject *parent = nullptr);

    QList<CalendarMetadata> fetchCalendars(const QString &collectionId) override;
    QList<CalendarItem*> fetchItems(const QString &calId) override;
    void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) override;
    void storeItems(const QString &calId, const QList<CalendarItem*> &items) override;

    QString serverUrl() const { return m_serverUrl; }
    QString username() const { return m_username; }
    QString password() const { return m_password; }

signals:
    void calendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void itemsFetched(const QString &calId, const QList<CalendarItem*> &items); // New

private slots:
    void onCollectionsFetched(KJob *job);

private:
    void processNextItemFetch(); // New
    void fetchItemData(const QString &calId, const KDAV::DavItem::List &items, int index = 0); // New

    QStringList m_itemFetchQueue; // New

    QString m_serverUrl;
    QString m_username;
    QString m_password;
    QMap<QString, QString> m_idToUrl;
};

#endif // CALDAVBACKEND_H
