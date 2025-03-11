#include "configmanager.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include <QSettings>
#include <QDir>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
}

void ConfigManager::setBasePath(const QString &path)
{
    m_basePath = path;
}

QString ConfigManager::configPath(const QString &collectionId, const QString &kalbPath) const
{
    if (!kalbPath.isEmpty()) {
        return kalbPath; // Use provided path for Open/Attach
    }
    QDir dir(m_basePath.isEmpty() ? "configs" : m_basePath); // Fallback
    if (!dir.exists()) dir.mkpath(".");
    return dir.filePath(collectionId + ".kalb");
}

void ConfigManager::saveBackendConfig(const QString &collectionId, const QList<SyncBackend*> &backends, const QString &kalbPath)
{
    QSettings settings(configPath(collectionId, kalbPath), QSettings::IniFormat);
    settings.clear();
    settings.beginWriteArray("backends");
    for (int i = 0; i < backends.size(); ++i) {
        settings.setArrayIndex(i);
        if (LocalBackend *local = dynamic_cast<LocalBackend*>(backends[i])) {
            settings.setValue("type", "local");
            settings.setValue("rootPath", local->rootPath());
        } else if (CalDAVBackend *caldav = dynamic_cast<CalDAVBackend*>(backends[i])) {
            settings.setValue("type", "caldav");
            settings.setValue("serverUrl", caldav->serverUrl());
            settings.setValue("username", caldav->username());
            settings.setValue("password", caldav->password());
        }
    }
    settings.endArray();
    settings.sync();
}

QList<SyncBackend*> ConfigManager::loadBackendConfig(const QString &collectionId, const QString &kalbPath)
{
    QList<SyncBackend*> backends;
    QSettings settings(configPath(collectionId, kalbPath), QSettings::IniFormat);
    int size = settings.beginReadArray("backends");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString type = settings.value("type").toString();
        if (type == "local") {
            QString rootPath = settings.value("rootPath", "").toString();
            backends.append(new LocalBackend(rootPath, this));
        } else if (type == "caldav") {
            QString serverUrl = settings.value("serverUrl", "").toString();
            QString username = settings.value("username", "").toString();
            QString password = settings.value("password", "").toString();
            backends.append(new CalDAVBackend(serverUrl, username, password, this));
        }
    }
    settings.endArray();
    return backends;
}
