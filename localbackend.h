#ifndef LOCALBACKEND_H
#define LOCALBACKEND_H

#include "syncbackend.h"
#include "cal.h"
#include <QObject>
#include <QMap>

class LocalBackend : public SyncBackend
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath)
public:
    explicit LocalBackend(const QString &rootPath, QObject *parent = nullptr);

    QString rootPath() const { return m_rootPath; }
    QList<CalendarMetadata> fetchCalendars(const QString &collectionId) override;
void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) override;
    void storeItems(Cal *cal, const QList<CalendarItem*> &items) override;
    QList<CalendarItem*> fetchItems(Cal *cal) override;
    void updateItem(const QString &calId, const QString &itemId, const QString &icalData);

private:
    QString m_rootPath;
    QMap<QString, QString> m_idToPath;
};

#endif // LOCALBACKEND_H
