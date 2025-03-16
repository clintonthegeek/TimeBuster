#ifndef LOCALBACKEND_H
#define LOCALBACKEND_H

#include "syncbackend.h"
#include <QDir>
#include <QMap>
#include <QSharedPointer>

class LocalBackend : public SyncBackend
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath)
public:
    explicit LocalBackend(const QString &rootPath, QObject *parent = nullptr);

    QString rootPath() const { return m_rootPath; }
    QList<CalendarMetadata> loadCalendars(const QString &collectionId) override;
    QList<QSharedPointer<CalendarItem>> loadItems(Cal *cal) override;
    void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) override;
    void storeItems(Cal *cal, const QList<QSharedPointer<CalendarItem>> &items) override;
    void updateItem(const QString &calId, const QString &itemId, const QString &icalData) override;

    void startSync(const QString &collectionId) override;

private:
    QString m_rootPath;
    QMap<QString, QString> m_idToPath; // Retained for storage/update
};

#endif // LOCALBACKEND_H
