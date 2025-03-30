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
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>

SessionManager::SessionManager(CollectionController *controller, QObject *parent)
    : QObject(parent), m_collectionController(controller), m_sessionId(QUuid::createUuid().toString())
{
    qDebug() << "SessionManager: Initialized with sessionId" << m_sessionId;
}

void SessionManager::queueDeltaChange(const QString &calId, const QSharedPointer<CalendarItem> &item, const QString &userIntent)
{
    qDebug() << "SessionManager: Queued" << userIntent << "change for item" << item->id() << "in calendar" << calId;

    DeltaEntry entry;
    entry.actionId = QUuid::createUuid().toString();
    entry.sessionId = m_sessionId;
    entry.timestamp = QDateTime::currentDateTimeUtc();
    entry.crashFlag = false;
    entry.userIntent = userIntent;
    entry.itemId = item->id();
    entry.calId = calId; // Store the calId explicitly
    entry.icalData = item->toICal().toUtf8().toBase64();

    m_newDeltaChanges.append(entry);
    QString collectionId = calId.split("_").first();
    saveToFile(collectionId);
    saveDeltaEntries(collectionId);
    emit changesStaged(m_newDeltaChanges);
}

void SessionManager::applyDeltaChanges()
{
    qDebug() << "SessionManager: Applying delta changes";

    for (const DeltaEntry &entry : m_newDeltaChanges) {
        QString calId = entry.calId; // Use the stored calId
        Cal *cal = m_collectionController->getCal(calId);
        if (!cal) {
            qDebug() << "SessionManager: No calendar found for" << calId << "—skipping change";
            continue;
        }

        for (const QSharedPointer<CalendarItem> &item : cal->items()) {
            if (item->id() == entry.itemId) {
                KCalendarCore::ICalFormat format;
                KCalendarCore::MemoryCalendar::Ptr tempCal(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
                QString icalString = QString::fromUtf8(QByteArray::fromBase64(entry.icalData.toLatin1()));
                if (format.fromString(tempCal, icalString)) {
                    KCalendarCore::Incidence::Ptr newIncidence = tempCal->incidences().first();
                    item->setIncidence(newIncidence); // Update the item's incidence in memory
                    item->setDirty(true); // Mark as dirty (not committed yet)
                    qDebug() << "SessionManager: Applied" << entry.userIntent << "change to item" << item->id() << "in" << calId;
                    if (entry.userIntent == "remove") {
                        cal->removeItem(item); // Remove from in-memory Cal if needed
                    } else if (entry.userIntent == "add" || entry.userIntent == "modify") {
                        cal->updateItem(item); // Ensure item is updated in Cal’s list
                    }
                } else {
                    qDebug() << "SessionManager: Failed to parse iCal data for" << entry.itemId;
                }
                break;
            }
        }
    }

    // Clear the staged changes after applying (crash recovery complete)
    QString collectionId = m_newDeltaChanges.isEmpty() ? "" : m_newDeltaChanges.first().calId.split("_").first();
    if (!collectionId.isEmpty()) {
        m_newDeltaChanges.clear();
        QFile::remove(deltaFilePath(collectionId) + ".json");
        saveToFile(collectionId, true); // Mark clean exit
    }
}

void SessionManager::loadStagedChanges(const QString &collectionId)
{
    m_newDeltaChanges = loadDeltaEntries(collectionId);
    if (m_newDeltaChanges.isEmpty()) {
        qDebug() << "SessionManager: No staged changes found at" << deltaFilePath(collectionId) + ".json";
        return;
    }

    QFile jsonFile(deltaFilePath(collectionId) + ".json");
    bool cleanExit = false;
    if (jsonFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
        cleanExit = doc.object()["cleanExit"].toBool(false);
        jsonFile.close();
    }

    if (!cleanExit) {
        qDebug() << "SessionManager: Crash detected for" << collectionId << "—uncommitted changes loaded";
        applyDeltaChanges(); // Auto-apply changes without prompt
    } else {
        qDebug() << "SessionManager: Clean exit detected, clearing staged changes for" << collectionId;
        m_newDeltaChanges.clear();
        QFile::remove(deltaFilePath(collectionId) + ".json");
        saveToFile(collectionId, true);
    }

    emit changesStaged(m_newDeltaChanges);
}

void SessionManager::undoLastCommit()
{
    qDebug() << "SessionManager: undoLastCommit (stub)";
}

void SessionManager::redoLastUndo()
{
    qDebug() << "SessionManager: redoLastUndo (stub)";
}

void SessionManager::saveToFile(const QString &collectionId, bool cleanExit)
{
    QFile jsonFile(deltaFilePath(collectionId) + ".json");
    if (!jsonFile.open(QIODevice::WriteOnly)) {
        qDebug() << "SessionManager: Failed to save JSON to" << jsonFile.fileName();
        return;
    }

    QJsonObject root;
    QJsonArray array;
    for (const DeltaEntry &entry : m_newDeltaChanges) {
        array.append(entry.toJson());
    }
    root["changes"] = array;
    root["cleanExit"] = cleanExit;

    QJsonDocument doc(root);
    jsonFile.write(doc.toJson(QJsonDocument::Compact));
    jsonFile.close();
    qDebug() << "SessionManager: Saved" << m_newDeltaChanges.size() << "JSON entries to" << jsonFile.fileName();
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

QString SessionManager::deltaFilePath(const QString &collectionId) const
{
    QString kalbPath = m_collectionController->kalbPath(collectionId);
    if (kalbPath.isEmpty()) {
        return QDir::tempPath() + "/deltas." + collectionId + ".tmp"; // Transient fallback
    }
    return QFileInfo(kalbPath).absolutePath() + "/deltas." + collectionId;
}

QString SessionManager::historyFilePath(const QString &collectionId) const
{
    QString kalbPath = m_collectionController->kalbPath(collectionId);
    if (kalbPath.isEmpty()) {
        return QString("/tmp/commits.%1.log").arg(collectionId); // Transient fallback
    }
    return QFileInfo(kalbPath).absolutePath() + "/history." + collectionId + ".log";
}

void SessionManager::saveDeltaEntries(const QString &collectionId)
{
    QFile file(deltaFilePath(collectionId) + ".json");
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "SessionManager: Failed to save JSON to" << file.fileName();
        return;
    }

    QJsonObject root;
    QJsonArray array;
    for (const DeltaEntry &entry : m_newDeltaChanges) {
        array.append(entry.toJson());
    }
    root["changes"] = array;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();
    qDebug() << "SessionManager: Saved" << m_newDeltaChanges.size() << "JSON entries to" << file.fileName();
}

QList<DeltaEntry> SessionManager::loadDeltaEntries(const QString &collectionId)
{
    QList<DeltaEntry> entries;
    QFile file(deltaFilePath(collectionId) + ".json");
    if (!file.open(QIODevice::ReadOnly)) {
        return entries;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray array = doc.object()["changes"].toArray();
    for (const QJsonValue &value : array) {
        entries.append(DeltaEntry::fromJson(value.toObject()));
    }
    file.close();
    qDebug() << "SessionManager: Loaded" << entries.size() << "JSON entries from" << file.fileName();
    return entries;
}

bool SessionManager::ChangeResolver::resolveUnappliedEdit(Cal* cal, const QSharedPointer<CalendarItem>& item, const QString& newSummary)
{
    if (!cal || !item) return false;

    // Clone the incidence and wrap it in a QSharedPointer
    KCalendarCore::Incidence::Ptr incidence = KCalendarCore::Incidence::Ptr(item->incidence()->clone());
    incidence->setSummary(newSummary);
    item->setIncidence(incidence); // Update the item's incidence
    item->setDirty(true); // Mark as dirty for persistence

    m_session->queueDeltaChange(cal->id(), item, "modify");
    return true;
}

QDataStream &operator<<(QDataStream &out, const DeltaEntry &entry)
{
    out << entry.actionId << entry.sessionId << entry.timestamp << entry.crashFlag
        << entry.userIntent << entry.itemId << entry.icalData;
    return out;
}

QDataStream &operator>>(QDataStream &in, DeltaEntry &entry)
{
    in >> entry.actionId >> entry.sessionId >> entry.timestamp >> entry.crashFlag
        >> entry.userIntent >> entry.itemId >> entry.icalData;
    return in;
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
