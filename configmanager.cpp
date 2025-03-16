#include "configmanager.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include <QFile>
#include <QDir>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>

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
        return kalbPath;
    }
    QDir dir(m_basePath.isEmpty() ? "configs" : m_basePath);
    if (!dir.exists()) dir.mkpath(".");
    return dir.filePath(collectionId + ".kalb");
}

bool ConfigManager::saveBackendConfig(const QString &collectionId, const QString &collectionName, const QList<SyncBackend*> &backends, const QString &kalbPath)
{
    QString path = configPath(collectionId, kalbPath);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "ConfigManager: Failed to open file for writing:" << path;
        return false;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();

    // Root element
    writer.writeStartElement("TimeBusterConfig");

    // Collection element
    writer.writeStartElement("Collection");
    writer.writeTextElement("Id", collectionId);
    writer.writeTextElement("Name", collectionName);
    writer.writeEndElement(); // Collection

    // Backends element
    writer.writeStartElement("Backends");
    for (SyncBackend *backend : backends) {
        writer.writeStartElement("Backend");
        if (LocalBackend *local = dynamic_cast<LocalBackend*>(backend)) {
            writer.writeAttribute("type", "local");
            writer.writeTextElement("RootPath", local->rootPath());
            writer.writeTextElement("priority", "1"); // Default priority for local
            writer.writeTextElement("SyncOnOpen", "false"); // Default no sync
        } else if (CalDAVBackend *caldav = dynamic_cast<CalDAVBackend*>(backend)) {
            writer.writeAttribute("type", "caldav");
            writer.writeTextElement("ServerUrl", caldav->serverUrl());
            writer.writeTextElement("Username", caldav->username());
            writer.writeTextElement("Password", caldav->password());
            writer.writeTextElement("priority", "2"); // Default priority for caldav
            writer.writeTextElement("SyncOnOpen", "false"); // Default no sync
        }
        writer.writeEndElement(); // Backend
    }
    writer.writeEndElement(); // Backends

    writer.writeEndElement(); // TimeBusterConfig
    writer.writeEndDocument();

    file.close();
    qDebug() << "ConfigManager: Saved config to" << path;
    return true;
}

QList<SyncBackend*> ConfigManager::loadBackendConfig(const QString &collectionId, const QString &kalbPath)
{
    QList<SyncBackend*> backends;
    QVariantMap config = loadConfig(collectionId, kalbPath);
    QVariantList backendsList = config.value("backends").toList();

    for (const QVariant &backendVar : backendsList) {
        QVariantMap backendMap = backendVar.toMap();
        QString type = backendMap.value("type").toString();
        if (type == "local") {
            QString rootPath = backendMap.value("rootPath").toString();
            backends.append(new LocalBackend(rootPath, this));
        } else if (type == "caldav") {
            QString serverUrl = backendMap.value("serverUrl").toString();
            QString username = backendMap.value("username").toString();
            QString password = backendMap.value("password").toString();
            backends.append(new CalDAVBackend(serverUrl, username, password, this));
        }
    }

    return backends;
}

QVariantMap ConfigManager::loadConfig(const QString &collectionId, const QString &kalbPath)
{
    QVariantMap config;
    QString path = configPath(collectionId, kalbPath);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "ConfigManager: Failed to open file for reading:" << path;
        return config;
    }

    QXmlStreamReader reader(&file);
    QVariantList backendsList;

    while (!reader.atEnd() && !reader.hasError()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == "Collection") {
                while (!(reader.isEndElement() && reader.name() == "Collection")) {
                    reader.readNext();
                    if (reader.isStartElement()) {
                        if (reader.name() == "Id") {
                            config["id"] = reader.readElementText();
                        } else if (reader.name() == "Name") {
                            config["name"] = reader.readElementText();
                        }
                    }
                }
            } else if (reader.name() == "Backend") {
                ConfigManager::BackendConfig backend;
                backend.type = reader.attributes().value("type").toString();
                backend.details.clear();
                backend.priority = 0; // Default priority
                backend.syncOnOpen = false; // Default no sync

                while (!(reader.isEndElement() && reader.name() == "Backend")) {
                    reader.readNext();
                    if (reader.isStartElement()) {
                        if (reader.name() == "RootPath") {
                            backend.details["rootPath"] = reader.readElementText();
                        } else if (reader.name() == "ServerUrl") {
                            backend.details["serverUrl"] = reader.readElementText();
                        } else if (reader.name() == "Username") {
                            backend.details["username"] = reader.readElementText();
                        } else if (reader.name() == "Password") {
                            backend.details["password"] = reader.readElementText();
                        } else if (reader.name() == "priority") {
                            bool ok;
                            int pri = reader.readElementText().toInt(&ok);
                            if (ok) backend.priority = pri;
                        } else if (reader.name() == "SyncOnOpen") {
                            QString value = reader.readElementText().toLower();
                            backend.syncOnOpen = (value == "true");
                        }
                    }
                }
                backendsList.append(QVariant::fromValue(backend));
            }
        }
    }

    if (reader.hasError()) {
        qDebug() << "ConfigManager: XML parse error:" << reader.errorString();
    }

    config["backends"] = backendsList;
    file.close();
    return config;
}
