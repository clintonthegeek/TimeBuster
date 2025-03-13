#include "sessionmanager.h"
#include "cal.h"
#include <QFile>
#include <QDebug>

SessionManager::SessionManager(CollectionManager *collectionManager, const QString &sessionPath, QObject *parent)
    : QObject(parent), m_collectionManager(collectionManager), m_sessionPath(sessionPath)
{
}

void SessionManager::stageChange(const QString &calId, const QString &itemId, const QString &icalData)
{
    QString key = calId + "_" + itemId;
    m_stagedChanges[key] = icalData;
    qDebug() << "SessionManager: Staged change for" << key;
}

void SessionManager::applyDeltaChanges()
{
    QFile deltaFile(m_sessionPath + "/.kalb.delta");
    if (!deltaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "SessionManager: No delta file found at" << deltaFile.fileName();
        return;
    }

    QTextStream in(&deltaFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(" ");
        if (parts.size() < 4) continue;

        QString operation = parts[0];
        QString calId = parts[1];
        QString itemId = parts[2];
        QString icalData = parts.mid(3).join(" ");

        if (operation == "UPDATE") {
            for (Collection *col : m_collectionManager->collections()) {
                for (Cal *cal : col->calendars()) {
                    if (cal->id() == calId) {
                        qDebug() << "SessionManager: Applying delta update for" << itemId;
                        // Stubbed: apply update logic here
                    }
                }
            }
        }
    }
    deltaFile.close();
}

void SessionManager::commitChanges(SyncBackend *activeBackend, Collection *activeCollection)
{
    if (!activeBackend || !activeCollection) {
        qDebug() << "SessionManager: No active backend or collection to commit";
        return;
    }

    for (Cal *cal : activeCollection->calendars()) {
        for (const auto &change : m_stagedChanges) {
            QString calId = change.split("_").first();
            if (cal->id() == calId) {
                qDebug() << "SessionManager: Committing change for" << change;
                // Stubbed: commit logic here
            }
        }
    }
    m_stagedChanges.clear();
}
