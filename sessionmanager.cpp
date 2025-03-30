#include "sessionmanager.h"
#include "collectioncontroller.h"
#include "collection.h"
#include "cal.h"
#include "calendaritem.h"
#include "localbackend.h"
#include "calendaritemfactory.h"
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <QDir>

SessionManager::SessionManager(CollectionController *controller, QObject *parent)
    : QObject(parent), m_collectionController(controller)
{
}

void SessionManager::queueDeltaChange(const QString &calId, const QSharedPointer<CalendarItem> &item, DeltaChange::Type change)
{
    m_deltaChanges[calId].append(DeltaChange(change, item));
    qDebug() << "SessionManager: Queued" << (change == DeltaChange::Modify ? "Modify" : change == DeltaChange::Add ? "Add" : "Remove")
             << "change for item" << item->id() << "in calendar" << calId;

    QString collectionId = calId.split("_").first();
    saveToFile(collectionId);
}

void SessionManager::applyDeltaChanges()
{
    qDebug() << "SessionManager: Applying delta changes";
    for (const QString &calId : m_deltaChanges.keys()) {
        QString collectionId = calId.split("_").first();
        Collection *col = m_collectionController->collection(collectionId);
        if (!col) {
            qDebug() << "SessionManager: Collection for collectionId" << collectionId << "not found";
            continue;
        }
        Cal *cal = m_collectionController->getCal(calId);
        if (!cal) {
            qDebug() << "SessionManager: No calendar found for" << calId;
            continue;
        }
        const QMap<QString, QList<SyncBackend*>> &allBackends = m_collectionController->backends();
        const QList<SyncBackend*> &backends = allBackends.value(col->id());
        if (backends.isEmpty()) {
            qDebug() << "SessionManager: No backends for collection" << col->id();
            continue;
        }
        SyncBackend *primaryBackend = nullptr;
        for (SyncBackend *backend : backends) {
            if (dynamic_cast<LocalBackend*>(backend)) {
                primaryBackend = backend;
                break;
            }
        }
        if (!primaryBackend) {
            qDebug() << "SessionManager: No LocalBackend found for collection" << col->id();
            continue;
        }

        QList<QSharedPointer<CalendarItem>> modifiedItems;
        for (const DeltaChange &delta : m_deltaChanges[calId]) {
            if (delta.change() == DeltaChange::Add) {
                cal->addItem(delta.getItem());
                modifiedItems.append(delta.getItem());
                qDebug() << "SessionManager: Added item" << delta.getItem()->id() << "to calendar" << calId;
            } else if (delta.change() == DeltaChange::Modify) {
                cal->updateItem(delta.getItem());
                modifiedItems.append(delta.getItem());
                qDebug() << "SessionManager: Modified item" << delta.getItem()->id() << "in calendar" << calId;
            } else if (delta.change() == DeltaChange::Remove) {
                cal->removeItem(delta.getItem());
                qDebug() << "SessionManager: Removed item" << delta.getItem()->id() << "from calendar" << calId;
            }
            delta.getItem()->setDirty(false);
        }

        // Persist to backend
        if (!modifiedItems.isEmpty()) {
            primaryBackend->storeItems(cal, modifiedItems);
            qDebug() << "SessionManager: Stored" << modifiedItems.size() << "items to LocalBackend for" << calId;
        }
    }

    QString collectionId = m_deltaChanges.keys().isEmpty() ? "" : m_deltaChanges.keys().first().split("_").first();
    m_deltaChanges.clear();
    if (!collectionId.isEmpty()) {
        QFile::remove(deltaFilePath(collectionId));
        qDebug() << "SessionManager: Cleared delta file for" << collectionId;
    }
}

void SessionManager::loadStagedChanges(const QString &collectionId)
{
    QString filePath = deltaFilePath(collectionId);
    QFile file(filePath);
    if (!file.exists()) {
        qDebug() << "SessionManager: No staged changes found at" << filePath;
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "SessionManager: Failed to open delta file" << filePath << ":" << file.errorString();
        return;
    }

    QDataStream in(&file);
    quint32 size;
    in >> size;
    for (quint32 i = 0; i < size; ++i) {
        QString calId;
        in >> calId;
        QList<DeltaChange> deltas;
        quint32 deltaCount;
        in >> deltaCount;
        for (quint32 j = 0; j < deltaCount; ++j) {
            DeltaChange delta;
            in >> delta;
            deltas.append(delta);
        }
        m_deltaChanges[calId] = deltas;
    }
    file.close();
    qDebug() << "SessionManager: Loaded" << m_deltaChanges.size() << "calendars with staged changes from" << filePath;

    // Apply deltas after loading
    for (const QString &calId : m_deltaChanges.keys()) {
        Cal *cal = m_collectionController->getCal(calId);
        if (!cal) {
            qDebug() << "SessionManager: No calendar found for" << calId << "while applying staged changes";
            continue;
        }
        for (const DeltaChange &delta : m_deltaChanges[calId]) {
            if (delta.change() == DeltaChange::Add) {
                cal->addItem(delta.getItem());
                qDebug() << "SessionManager: Applied staged add for item" << delta.getItem()->id() << "to" << calId;
            } else if (delta.change() == DeltaChange::Modify) {
                cal->updateItem(delta.getItem());
                qDebug() << "SessionManager: Applied staged modify for item" << delta.getItem()->id() << "to" << calId;
            } else if (delta.change() == DeltaChange::Remove) {
                cal->removeItem(delta.getItem());
                qDebug() << "SessionManager: Applied staged remove for item" << delta.getItem()->id() << "from" << calId;
            }
        }
    }
    qDebug() << "SessionManager: Applied all staged changes for collection" << collectionId;
}

void SessionManager::saveToFile(const QString &collectionId)
{
    QString filePath = deltaFilePath(collectionId);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "SessionManager: Failed to open delta file" << filePath << ":" << file.errorString();
        return;
    }

    QDataStream out(&file);
    out << quint32(m_deltaChanges.size());
    for (auto it = m_deltaChanges.constBegin(); it != m_deltaChanges.constEnd(); ++it) {
        out << it.key(); // calId
        out << quint32(it.value().size());
        for (const DeltaChange &delta : it.value()) {
            out << delta;
        }
    }
    file.close();
    qDebug() << "SessionManager: Saved" << m_deltaChanges.size() << "calendars with staged changes to" << filePath;
}

QString SessionManager::deltaFilePath(const QString &collectionId) const
{
    return QDir::tempPath() + "/deltas." + collectionId + ".tmp";
}

QDataStream &operator<<(QDataStream &out, const DeltaChange &delta)
{
    out << quint32(delta.change());
    out << delta.getItem()->calId();
    out << delta.getItem()->id();
    out << delta.getItem()->toICal();
    return out;
}

QDataStream &operator>>(QDataStream &in, DeltaChange &delta)
{
    quint32 changeType;
    in >> changeType;
    delta = DeltaChange(static_cast<DeltaChange::Type>(changeType), QSharedPointer<CalendarItem>());

    QString calId, itemId, icalData;
    in >> calId >> itemId >> icalData;

    KCalendarCore::ICalFormat format;
    KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
    if (format.fromString(tempCalendar, icalData)) {
        KCalendarCore::Incidence::Ptr incidence = tempCalendar->incidences().first();
        CalendarItem *item = CalendarItemFactory::createItem(calId, incidence);
        delta = DeltaChange(static_cast<DeltaChange::Type>(changeType), QSharedPointer<CalendarItem>(item));
    } else {
        qDebug() << "SessionManager: Failed to parse iCal data for item" << itemId;
    }
    return in;
}

bool SessionManager::ChangeResolver::resolveUnappliedEdit(const QSharedPointer<CalendarItem>& item, const QString& newSummary)
{
    if (!item || newSummary == item->incidence()->summary()) {
        qDebug() << "ChangeResolver: No changes to resolve for" << (item ? item->id() : "null");
        return true; // Nothing to do
    }

    // For Stage 1: Auto-apply and queue (no UI yet)
    item->incidence()->setSummary(newSummary);
    item->setDirty(true);
    QString calId = item->calId();
    Cal* cal = m_session->m_collectionController->getCal(calId);
    if (cal) {
        cal->updateItem(item);
        m_session->queueDeltaChange(calId, item, DeltaChange::Modify);
        qDebug() << "ChangeResolver: Auto-applied edit to" << item->id() << "in" << calId;
        return true;
    } else {
        qDebug() << "ChangeResolver: No calendar found for" << calId << "—edit discarded";
        return false; // Can’t apply, discard for now
    }

    // TODO: Stage 2—add UI escalation (e.g., QMessageBox::Apply/Discard/Cancel)
}
