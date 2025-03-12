#include "sessionmanager.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include "cal.h"
#include "calendaritemfactory.h"
#include "localbackend.h"
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

SessionManager::SessionManager(const QString &kalbFilePath, QObject *parent)
    : QObject(parent), m_kalbFilePath(kalbFilePath)
{
    setKalbFilePath(kalbFilePath);
}

SessionManager::~SessionManager()
{
    saveCache();
}

void SessionManager::setKalbFilePath(const QString &kalbFilePath)
{
    m_kalbFilePath = kalbFilePath;
    m_deltaFilePath = QFileInfo(m_kalbFilePath).absolutePath() + "/.kalb.delta";
    qDebug() << "SessionManager: Set delta path to" << m_deltaFilePath;
}

void SessionManager::stageChange(const Change &change)
{
    if (!change.data.isEmpty()) { // Skip incomplete changes
        m_changes.append(change);
        saveCache();
    } else {
        qDebug() << "SessionManager: Skipping incomplete change for item" << change.itemId;
    }
}

void SessionManager::saveCache()
{
    // Only save if the delta file path is valid and corresponds to a .kalb file
    if (m_deltaFilePath.isEmpty() || !QFileInfo(m_kalbFilePath).exists()) {
        qDebug() << "SessionManager: Skipping saveCache - no valid .kalb file associated";
        return;
    }

    QFile file(m_deltaFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        QTextStream out(&file);
        for (const Change &change : m_changes) {
            if (!change.data.isEmpty()) {
                out << "calId:" << change.calId << "\n"
                    << "itemId:" << change.itemId << "\n"
                    << "operation:" << change.operation << "\n"
                    << "data:" << change.data << "\n"
                    << "timestamp:" << change.timestamp.toString() << "\n"
                    << "---\n";
            }
        }
        file.close();
        qDebug() << "SessionManager: Saved" << m_changes.size() << "changes to" << m_deltaFilePath;
    } else {
        qDebug() << "SessionManager: Failed to save cache to" << m_deltaFilePath;
    }
}
void SessionManager::loadCache()
{
    m_changes.clear();
    QFile file(m_deltaFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        Change change;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line == "---") {
                if (!change.calId.isEmpty() && !change.data.isEmpty()) {
                    m_changes.append(change);
                    change = Change();
                }
                continue;
            }
            if (line.startsWith("calId:")) change.calId = line.mid(6);
            else if (line.startsWith("itemId:")) change.itemId = line.mid(7);
            else if (line.startsWith("operation:")) change.operation = line.mid(10);
            else if (line.startsWith("data:")) change.data = line.mid(5);
            else if (line.startsWith("timestamp:")) change.timestamp = QDateTime::fromString(line.mid(10));
        }
        if (!change.calId.isEmpty() && !change.data.isEmpty()) m_changes.append(change);
        file.close();
        qDebug() << "SessionManager: Loaded" << m_changes.size() << "changes";
        applyDeltaChanges();
    } else {
        qDebug() << "SessionManager: No cache file found at" << m_deltaFilePath;
    }
}

void SessionManager::applyDeltaChanges()
{
    if (!m_collectionManager) {
        qDebug() << "SessionManager: No CollectionManager available";
        return;
    }

    for (const Change &change : m_changes) {
        if (change.operation == "UPDATE") {
            qDebug() << "SessionManager: Applying change for calId:" << change.calId << "itemId:" << change.itemId;
            bool found = false;
            for (Collection *col : m_collectionManager->collections()) {
                for (Cal *cal : col->calendars()) {
                    QString expectedCalId = cal->id().split('_').last(); // Match by name-derived part
                    if (cal->id() == change.calId || expectedCalId == change.calId.split('_').last()) {
                        qDebug() << "SessionManager: Found calendar" << cal->name() << "for calId" << change.calId;
                        for (int i = 0; i < cal->items().size(); ++i) {
                            CalendarItem *item = cal->items().at(i);
                            if (item->id() == change.itemId) {
                                KCalendarCore::ICalFormat format;
                                KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone("UTC")));
                                if (format.fromString(tempCalendar, change.data)) {
                                    KCalendarCore::Incidence::List incidences = tempCalendar->incidences();
                                    if (!incidences.isEmpty()) {
                                        item->setIncidence(incidences.first());
                                        cal->dataChanged(cal->index(i, 0), cal->index(i, cal->columnCount() - 1));
                                        qDebug() << "SessionManager: Applied delta update for item" << change.itemId;
                                        found = true;
                                    }
                                }
                                break;
                            }
                        }
                        break;
                    }
                }
                if (found) break;
            }
            if (!found) qDebug() << "SessionManager: Calendar not found for calId" << change.calId;
        }
    }
    emit changesApplied();
}

void SessionManager::commitChanges(SyncBackend *backend, Collection *activeCollection)
{
    if (LocalBackend *local = qobject_cast<LocalBackend*>(backend)) {
        if (!activeCollection) {
            qDebug() << "SessionManager: No active collection";
            return;
        }
        for (Cal *cal : activeCollection->calendars()) {
            for (CalendarItem *item : cal->items()) {
                KCalendarCore::ICalFormat format;
                KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone("UTC")));
                tempCalendar->addIncidence(item->incidence());
                QString icalData = format.toString(tempCalendar);
                if (!icalData.isEmpty()) {
                    local->updateItem(cal->id(), item->id(), icalData);
                    qDebug() << "SessionManager: Committed item" << item->id() << "to local storage";
                } else {
                    qDebug() << "SessionManager: Failed to serialize item" << item->id();
                }
            }
        }
        clearCache();
        if (ui && ui->logTextEdit) {
            ui->logTextEdit->append("Changes committed to local storage.");
        }
    }
}

void SessionManager::clearCache()
{
    QFile::remove(m_deltaFilePath);
    m_changes.clear();
}
