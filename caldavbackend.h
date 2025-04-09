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
    ~CalDAVBackend() override;

    // SyncBackend interface (keep for compatibility, but minimal use)
    QList<CalendarMetadata> loadCalendars(const QString &collectionId) override;
    QList<QSharedPointer<CalendarItem>> loadItems(Cal *cal) override;
    void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) override;
    void storeItems(Cal *cal, const QList<QSharedPointer<CalendarItem>> &items) override;
    void updateItem(const QString &calId, const QString &itemId, const QString &icalData) override;

    void startSync(const QString &collectionId) override;

    QString fetchItemVersionIdentifier(const QString &calId, const QString &itemId) override;
    void removeItem(const QString &calId, const QString &itemId) override;


    QString serverUrl() const { return m_serverUrl; }
    QString username() const { return m_username; }
    QString password() const { return m_password; }

private slots:
    void onCollectionsLoaded(KJob *job);
    void processNextItemLoad();

private:
    QStringList m_itemFetchQueue;
    QString m_serverUrl;
    QString m_username;
    QString m_password;
    QMap<QString, Cal*> m_calMap;        // Temporary Cal objects during sync
    QMap<QString, QString> m_idToUrl;    // Maps calId to DAV URL
    QList<KJob*> m_activeJobs;           // Tracks running KDAV jobs
};

#endif // CALDAVBACKEND_H
