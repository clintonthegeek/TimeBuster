#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QString>
#include "collectionmanager.h"

struct Change {
    QString calId;
    QString itemId;
    QString operation; // e.g., "UPDATE", "DELETE"
    QString data;      // iCal string or serialized data
    QDateTime timestamp;
};

class SyncBackend;

class SessionManager : public QObject
{
    Q_OBJECT

public:
    explicit SessionManager(const QString &kalbFilePath, QObject *parent = nullptr);
    ~SessionManager();
    void setCollectionManager(CollectionManager *cm) { m_collectionManager = cm; }

    void stageChange(const Change &change);
    void saveCache();
    void loadCache();
    void clearCache();
    void applyToBackend(SyncBackend *backend);
    void commitChanges(SyncBackend *backend); // Commits changes to backend
    int changesCount() const { return m_changes.size(); }



    // Set the .kalb file path to determine the delta file location
    void setKalbFilePath(const QString &kalbFilePath);

signals:
    void changesApplied(); // Signal to notify UI of applied changes

private:
    QString m_kalbFilePath;
    QString m_deltaFilePath;
    QList<Change> m_changes;
    CollectionManager *m_collectionManager = nullptr;
};

#endif // SESSIONMANAGER_H
