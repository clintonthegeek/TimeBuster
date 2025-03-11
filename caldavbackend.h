#ifndef CALDAVBACKEND_H
#define CALDAVBACKEND_H

#include "syncbackend.h"
#include <QMap>

class CalDAVBackend : public SyncBackend
{
    Q_OBJECT

public:
    explicit CalDAVBackend(const QString &serverUrl, const QString &username, const QString &password, QObject *parent = nullptr);

    QList<CalendarMetadata> fetchCalendars(const QString &collectionId) override;
    QList<CalendarItem*> fetchItems(const QString &calId) override;
    void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) override;
    void storeItems(const QString &calId, const QList<CalendarItem*> &items) override;

private:
    QString m_serverUrl;
    QString m_username;
    QString m_password;
    QMap<QString, QString> m_idToUrl; // Fake mapping for now
};

#endif // CALDAVBACKEND_H
