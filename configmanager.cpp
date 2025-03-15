#include "configmanager.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include <QFile>
#include <QDir>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>
#include <QString>


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
    for (int i = 0; i < backends.size(); ++i) {
        writer.writeStartElement("Backend");
        if (LocalBackend *local = dynamic_cast<LocalBackend*>(backends[i])) {
            writer.writeAttribute("type", "local");
            writer.writeTextElement("RootPath", local->rootPath());
        } else if (CalDAVBackend *caldav = dynamic_cast<CalDAVBackend*>(backends[i])) {
            writer.writeAttribute("type", "caldav");
            writer.writeTextElement("ServerUrl", caldav->serverUrl());
            writer.writeTextElement("Username", caldav->username());
            writer.writeTextElement("Password", caldav->password());
        }
        writer.writeEndElement(); // Backend
    }
    writer.writeEndElement(); // Backends

    writer.writeEndElement(); // TimeBusterConfig
    writer.writeEndDocument();

    file.close();
    qDebug() << "ConfigManager: Saved config to" << path;
    return true; // Return true on success
}

QList<SyncBackend*> ConfigManager::loadBackendConfig(const QString &collectionId, const QString &kalbPath)
{
    QList<SyncBackend*> backends;
    QString path = configPath(collectionId, kalbPath);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "ConfigManager: Failed to open file for reading:" << path;
        return backends;
    }

    QXmlStreamReader reader(&file);
    while (!reader.atEnd() && !reader.hasError()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == "Backend") {
            QString type = reader.attributes().value("type").toString();
            if (type == "local") {
                QString rootPath;
                while (!(reader.isEndElement() && reader.name() == "Backend")) {
                    reader.readNext();
                    if (reader.isStartElement() && reader.name() == "RootPath") {
                        rootPath = reader.readElementText();
                    }
                }
                backends.append(new LocalBackend(rootPath, this));
            } else if (type == "caldav") {
                QString serverUrl, username, password;
                while (!(reader.isEndElement() && reader.name() == "Backend")) {
                    reader.readNext();
                    if (reader.isStartElement()) {
                        if (reader.name() == "ServerUrl") {
                            serverUrl = reader.readElementText();
                        } else if (reader.name() == "Username") {
                            username = reader.readElementText();
                        } else if (reader.name() == "Password") {
                            password = reader.readElementText();
                        }
                    }
                }
                backends.append(new CalDAVBackend(serverUrl, username, password, this));
            }
        }
    }

    if (reader.hasError()) {
        qDebug() << "ConfigManager: XML parse error:" << reader.errorString();
    }

    file.close();
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
                QVariantMap backend;
                QString type = reader.attributes().value("type").toString();
                backend["type"] = type;
                if (type == "local") {
                    while (!(reader.isEndElement() && reader.name() == "Backend")) {
                        reader.readNext();
                        if (reader.isStartElement() && reader.name() == "RootPath") {
                            backend["rootPath"] = reader.readElementText();
                        }
                    }
                } else if (type == "caldav") {
                    while (!(reader.isEndElement() && reader.name() == "Backend")) {
                        reader.readNext();
                        if (reader.isStartElement()) {
                            if (reader.name() == "ServerUrl") {
                                backend["serverUrl"] = reader.readElementText();
                            } else if (reader.name() == "Username") {
                                backend["username"] = reader.readElementText();
                            } else if (reader.name() == "Password") {
                                backend["password"] = reader.readElementText();
                            }
                        }
                    }
                }
                backendsList.append(backend);
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
