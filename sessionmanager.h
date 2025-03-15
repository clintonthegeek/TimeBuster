#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QMap>
#include "syncbackend.h"
#include "collectioncontroller.h"
#include "collection.h" // Added for full Collection definition
#include "deltachange.h"

class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(CollectionController *controller, QObject *parent = nullptr);
    void queueDeltaChange(const QString &calId, const QSharedPointer<CalendarItem> &item, DeltaChange::Type change);
    void applyDeltaChanges();

private:
    CollectionController *m_collectionController;
    QMap<QString, QList<DeltaChange>> m_deltaChanges;};

#endif // SESSIONMANAGER_H
