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
    void saveBackendConfig(const QString &collectionId, const QString &collectionName, const QList<SyncBackend*> &backends, const QString &kalbPath = QString());
    QList<SyncBackend*> loadBackendConfig(const QString &collectionId, const QString &kalbPath = QString());
    QVariantMap loadConfig(const QString &collectionId, const QString &kalbPath = QString()); // New method

private:
    QString configPath(const QString &collectionId, const QString &kalbPath) const;
    QString m_basePath;
};

#endif // CONFIGMANAGER_H
