#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QString>
#include "collectionmanager.h"
#include "ui_mainwindow.h"


struct Change {
    QString calId;
    QString itemId;
    QString operation; // e.g., "UPDATE", "DELETE"
    QString data;      // iCal string or serialized data
    QDateTime timestamp;
};

class SyncBackend;
class Collection;     // Forward declaration

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
        void setMainWindow(Ui::MainWindow *ui) { this->ui = ui; } // New method to set UI
    //void applyToBackend(SyncBackend *backend);
    int changesCount() const { return m_changes.size(); }
    void commitChanges(SyncBackend *backend, Collection *activeCollection); // Updated to take activeCollection


    // Set the .kalb file path to determine the delta file location
    void setKalbFilePath(const QString &kalbFilePath);

signals:
    void changesApplied(); // Signal to notify UI of applied changes

private:
    QString m_kalbFilePath;
    QString m_deltaFilePath;
    QList<Change> m_changes;
    CollectionManager *m_collectionManager = nullptr;
    void applyDeltaChanges(); // New method to apply changes in-memory

        Ui::MainWindow *ui; // New member for UI access
};

#endif // SESSIONMANAGER_H
