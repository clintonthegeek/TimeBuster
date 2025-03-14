#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QMap>
#include "syncbackend.h"
#include "collectioncontroller.h"
#include "collection.h" // Added for full Collection definition

class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(CollectionController *collectionController, const QString &sessionPath, QObject *parent = nullptr);
    void stageChange(const QString &calId, const QString &itemId, const QString &icalData);
    void commitChanges(SyncBackend *activeBackend, Collection *activeCollection);
    void applyDeltaChanges();

private:
    CollectionController *m_collectionController;
    QString m_sessionPath;
    QMap<QString, QString> m_stagedChanges; // calId_itemId -> icalData
};

#endif // SESSIONMANAGER_H
