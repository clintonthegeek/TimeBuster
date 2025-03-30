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
    Commit commit;
    commit.timestamp = QDateTime::currentDateTime();

    for (const QString &calId : m_deltaChanges.keys()) {
        QString collectionId = calId.split("_").first();
        Collection *col = m_collectionController->collection(collectionId);
        if (!col) continue;
        Cal *cal = m_collectionController->getCal(calId);
        if (!cal) continue;
        SyncBackend *primaryBackend = nullptr;
        for (SyncBackend *backend : m_collectionController->backends().value(col->id())) {
            if (dynamic_cast<LocalBackend*>(backend)) {
                primaryBackend = backend;
                break;
            }
        }
        if (!primaryBackend) continue;

        QList<QSharedPointer<CalendarItem>> modifiedItems;
        for (const DeltaChange &delta : m_deltaChanges[calId]) {
            if (delta.change() == DeltaChange::Add) {
                cal->addItem(delta.getItem());
                modifiedItems.append(delta.getItem());
            } else if (delta.change() == DeltaChange::Modify) {
                cal->updateItem(delta.getItem());
                modifiedItems.append(delta.getItem());
            } else if (delta.change() == DeltaChange::Remove) {
                cal->removeItem(delta.getItem());
            }
            delta.getItem()->setDirty(false);
            commit.changes.append(delta);
        }

        if (!modifiedItems.isEmpty()) {
            primaryBackend->storeItems(cal, modifiedItems);
            qDebug() << "SessionManager: Stored" << modifiedItems.size() << "items to LocalBackend for" << calId;
        }
    }

    if (!commit.changes.isEmpty()) {
        m_history.append(commit);
        QString collectionId = m_deltaChanges.keys().first().split("_").first();
        saveHistory(collectionId);
    }

    QString collectionId = m_deltaChanges.keys().isEmpty() ? "" : m_deltaChanges.keys().first().split("_").first();
    m_deltaChanges.clear();
    if (!collectionId.isEmpty()) {
        QFile::remove(deltaFilePath(collectionId));
        saveToFile(collectionId); // Update deltas (empty now)
    }
}

void SessionManager::saveHistory(const QString &collectionId)
{
    QFile file(historyFilePath(collectionId));
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);
        out << m_history;
        file.close();
        qDebug() << "SessionManager: Saved history to" << file.fileName();
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


QDataStream &operator<<(QDataStream &out, const SessionManager::Commit &commit)
{
    out << commit.timestamp << commit.changes;
    return out;
}

QDataStream &operator>>(QDataStream &in, SessionManager::Commit &commit)
{
    in >> commit.timestamp >> commit.changes;
    return in;
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

QString SessionManager::historyFilePath(const QString &collectionId) const
{
    return QString("/tmp/commits.%1.log").arg(collectionId);
}


bool SessionManager::ChangeResolver::resolveUnappliedEdit(Cal* cal, const QSharedPointer<CalendarItem>& item, const QString& newSummary)
{
    if (!item || newSummary == item->incidence()->summary()) {
        qDebug() << "ChangeResolver: No changes to resolve for" << (item ? item->id() : "null");
        return true;
    }

    item->incidence()->setSummary(newSummary);
    item->setDirty(true);
    if (cal) {
        cal->updateItem(item);
        m_session->queueDeltaChange(cal->id(), item, DeltaChange::Modify);
        qDebug() << "ChangeResolver: Auto-applied edit to" << item->id() << "in" << cal->id();
        return true;
    } else {
        qDebug() << "ChangeResolver: No calendar provided—edit discarded";
        return false;
    }
}
