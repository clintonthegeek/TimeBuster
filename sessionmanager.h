#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QList>
#include <QVariant>
#include "syncbackend.h"

struct Change {
    QString calId;
    QString itemId;
    QString operation;
    QVariant data;
    QDateTime timestamp;
};

class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(const QString &kalbPath, QObject *parent = nullptr);
    ~SessionManager();

    void stageChange(const Change &change);
    void saveCache();
    void loadCache();
    void applyToBackend(SyncBackend *backend);
    void clearCache();

private:
    QString m_kalbPath;
    QList<Change> m_changes;
};

#endif // SESSIONMANAGER_H
