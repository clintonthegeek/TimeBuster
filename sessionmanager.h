#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QMap>
#include "syncbackend.h"
#include "collectionmanager.h"
#include "collection.h" // Added for full Collection definition

class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(CollectionManager *collectionManager, const QString &sessionPath, QObject *parent = nullptr);
    void stageChange(const QString &calId, const QString &itemId, const QString &icalData);
    void commitChanges(SyncBackend *activeBackend, Collection *activeCollection);
    void applyDeltaChanges();

private:
    CollectionManager *m_collectionManager;
    QString m_sessionPath;
    QMap<QString, QString> m_stagedChanges; // calId_itemId -> icalData
};

#endif // SESSIONMANAGER_H
