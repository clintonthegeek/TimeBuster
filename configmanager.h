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

    void saveBackendConfig(const QString &collectionId, const QList<SyncBackend*> &backends);
    QList<SyncBackend*> loadBackendConfig(const QString &collectionId);

private:
    QString configPath(const QString &collectionId) const;
};

#endif // CONFIGMANAGER_H
