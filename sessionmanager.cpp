#include "sessionmanager.h"
#include "collectioncontroller.h"
#include "collection.h"
#include "cal.h"
#include "calendaritem.h"
#include <QDebug>

SessionManager::SessionManager(CollectionController *controller, QObject *parent)
    : QObject(parent), m_collectionController(controller)
{
}

void SessionManager::queueDeltaChange(const QString &calId, const QSharedPointer<CalendarItem> &item, DeltaChange::Type change)
{
    m_deltaChanges[calId].append(DeltaChange(change, item));
    qDebug() << "SessionManager: Queued" << (change == DeltaChange::Modify ? "Modify" : change == DeltaChange::Add ? "Add" : "Remove")
             << "change for item" << item->id() << "in calendar" << calId;
}

void SessionManager::applyDeltaChanges()
{
    qDebug() << "SessionManager: Applying delta changes";
    for (const QString &calId : m_deltaChanges.keys()) {
        Collection *col = m_collectionController->collection(calId);
        if (!col) {
            qDebug() << "SessionManager: Collection for calId" << calId << "not found";
            continue;
        }
        Cal *cal = m_collectionController->getCal(calId);
        if (!cal) {
            qDebug() << "SessionManager: No calendar found for" << calId;
            continue;
        }
        for (const DeltaChange &delta : m_deltaChanges[calId]) {
            if (delta.change() == DeltaChange::Add) {
                cal->addItem(delta.getItem());
                qDebug() << "SessionManager: Added item" << delta.getItem()->id() << "to calendar" << calId;
            } else if (delta.change() == DeltaChange::Modify) {
                cal->updateItem(delta.getItem());
                qDebug() << "SessionManager: Modified item" << delta.getItem()->id() << "in calendar" << calId;
            } else if (delta.change() == DeltaChange::Remove) {
                cal->removeItem(delta.getItem());
                qDebug() << "SessionManager: Removed item" << delta.getItem()->id() << "from calendar" << calId;
            }
            delta.getItem()->setDirty(false);
        }
    }
    m_deltaChanges.clear();
}
