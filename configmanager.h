#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QList>
#include "syncbackend.h"

class ConfigManager : public QObject
{
    Q_OBJECT
public:
    explicit ConfigManager(QObject *parent = nullptr);
    void setBasePath(const QString &path);
    bool saveBackendConfig(const QString &collectionId, const QString &collectionName, const QList<SyncBackend*> &backends, const QString &kalbPath = QString());
    QList<SyncBackend*> loadBackendConfig(const QString &collectionId, const QString &kalbPath = QString());
    QVariantMap loadConfig(const QString &collectionId, const QString &kalbPath = QString());

    // New struct to hold backend configuration with priority and sync settings
    struct BackendConfig {
        QString type;
        QVariantMap details;
        int priority;
        bool syncOnOpen;
    };

private:
    QString configPath(const QString &collectionId, const QString &kalbPath) const;
    QString m_basePath;
};

#endif // CONFIGMANAGER_H
