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
    virtual ~CalDAVBackend(); // Ensure virtual destructor is declared

    QList<CalendarMetadata> fetchCalendars(const QString &collectionId) override;
    QList<CalendarItem*> fetchItems(Cal *cal) override;
    void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) override;
    void storeItems(Cal *cal, const QList<CalendarItem*> &items) override;

    QString serverUrl() const { return m_serverUrl; }
    QString username() const { return m_username; }
    QString password() const { return m_password; }

signals:
    void calendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void itemsFetched(Cal *cal, const QList<CalendarItem*> &items); // Updated to pass Cal*

private slots:
    void onCollectionsFetched(KJob *job);

private:
    void processNextItemFetch(); // New

    QStringList m_itemFetchQueue; // New

    QString m_serverUrl;
    QString m_username;
    QString m_password;
    QMap<QString, Cal*> m_calMap; // Map calId to Cal* for async access
    QMap<QString, QString> m_idToUrl;

    // Getters and setters for m_calMap (optional but good practice)
    Cal* getCal(const QString &calId) const { return m_calMap.value(calId); }
    void setCal(const QString &calId, Cal *cal) { m_calMap.insert(calId, cal); }
    void removeCal(const QString &calId) { m_calMap.remove(calId); }
    QList<KJob*> m_activeJobs; // Track active jobs
};

#endif // CALDAVBACKEND_H
