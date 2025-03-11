#ifndef LOCALBACKEND_H
#define LOCALBACKEND_H

#include "syncbackend.h"
#include <QMap>

class LocalBackend : public SyncBackend
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath)

public:
    explicit LocalBackend(const QString &rootPath, QObject *parent = nullptr);
    QString rootPath() const { return m_rootPath; }

    QList<CalendarMetadata> fetchCalendars(const QString &collectionId) override;
    QList<CalendarItem*> fetchItems(const QString &calId) override;
    void storeCalendars(const QString &collectionId, const QList<Cal*> &calendars) override;
    void storeItems(const QString &calId, const QList<CalendarItem*> &items) override;

private:
    QString m_rootPath; // Stubbed - no real file IO yet
    QMap<QString, QString> m_idToPath; // Fake mapping for now
};

#endif // LOCALBACKEND_H
