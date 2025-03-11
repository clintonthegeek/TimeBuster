#include "configmanager.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include <QSettings>
#include <QDir>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
}

void ConfigManager::saveBackendConfig(const QString &collectionId, const QList<SyncBackend*> &backends)
{
    QSettings settings(configPath(collectionId), QSettings::IniFormat);
    settings.clear();

    settings.beginWriteArray("backends");
    for (int i = 0; i < backends.size(); ++i) {
        settings.setArrayIndex(i);
        if (LocalBackend *local = dynamic_cast<LocalBackend*>(backends[i])) {
            settings.setValue("type", "local");
            settings.setValue("rootPath", local->property("rootPath").toString());
        } else if (CalDAVBackend *caldav = dynamic_cast<CalDAVBackend*>(backends[i])) {
            settings.setValue("type", "caldav");
            settings.setValue("serverUrl", caldav->property("serverUrl").toString());
            settings.setValue("username", caldav->property("username").toString());
            settings.setValue("password", caldav->property("password").toString());
        }
    }
    settings.endArray();
    settings.sync(); // Stubbed - writes to a fake .kalb file
}

QList<SyncBackend*> ConfigManager::loadBackendConfig(const QString &collectionId)
{
    QList<SyncBackend*> backends;
    QSettings settings(configPath(collectionId), QSettings::IniFormat);

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

    return backends; // Stubbed - returns fake data if no file
}

QString ConfigManager::configPath(const QString &collectionId) const
{
    QDir dir("configs"); // Stubbed - fake directory
    if (!dir.exists()) dir.mkpath(".");
    return dir.filePath(collectionId + ".kalb");
}
