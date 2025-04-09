#include "databasemanager.h"
#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>

#include <QCryptographicHash> // if needed elsewhere

bool DatabaseManager::initializeDatabase(const QString &collectionId, const QString &dbPath, QSqlDatabase &db)
{
    db = QSqlDatabase::addDatabase("QSQLITE", "dbmanager_" + collectionId);
    db.setDatabaseName(dbPath + "/collection.db");
    if (!db.open()) {
        qDebug() << "DatabaseManager: Failed to open database:" << db.lastError().text();
        return false;
    }

    return createBaseSchema(db);
}

bool DatabaseManager::createBaseSchema(QSqlDatabase &db)
{
    QSqlQuery query(db);

    // Create schema_version table
    query.exec("CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY)");
    if (query.lastError().isValid()) {
        qDebug() << "DatabaseManager: Failed to create schema_version table:" << query.lastError().text();
        return false;
    }

    // Check or set initial version
    query.exec("SELECT version FROM schema_version");
    if (!query.next()) {
        query.exec("INSERT INTO schema_version (version) VALUES (1)");
        if (query.lastError().isValid()) {
            qDebug() << "DatabaseManager: Failed to initialize schema version:" << query.lastError().text();
            return false;
        }
    }

    // Create calendars table
    query.exec("CREATE TABLE IF NOT EXISTS calendars ("
               "calId TEXT PRIMARY KEY, "
               "calName TEXT NOT NULL, "
               "path TEXT)");
    if (query.lastError().isValid()) {
        qDebug() << "DatabaseManager: Failed to create calendars table:" << query.lastError().text();
        return false;
    }

    // New: Create table for version identifiers
    query.exec("CREATE TABLE IF NOT EXISTS version_identifiers ("
               "itemId TEXT NOT NULL, "
               "backendId TEXT NOT NULL, "
               "versionIdentifier TEXT, "
               "lastSyncTimestamp TEXT, "
               "PRIMARY KEY (itemId, backendId))");
    if (query.lastError().isValid()) {
        qDebug() << "DatabaseManager: Failed to create version_identifiers table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::updateVersionIdentifier(const QString &dbPath,
                                              const QString &itemId,
                                              const QString &backendId,
                                              const QString &versionIdentifier,
                                              const QDateTime &lastSyncTimestamp)
{
    // Open a SQLite connection using a unique connection name.
    QString connName = QString("dbmanager_update_%1_%2").arg(itemId, backendId);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(dbPath + "/timebuster.db");
    if (!db.open()) {
        qDebug() << "DatabaseManager: Failed to open database for update:" << db.lastError().text();
        return false;
    }
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO version_identifiers (itemId, backendId, versionIdentifier, lastSyncTimestamp) VALUES (?, ?, ?, ?)");
    query.addBindValue(itemId);
    query.addBindValue(backendId);
    query.addBindValue(versionIdentifier);
    query.addBindValue(lastSyncTimestamp.toString(Qt::ISODate));
    if (!query.exec()) {
        qDebug() << "DatabaseManager: Failed to update version identifier:" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase(connName);
        return false;
    }
    db.close();
    QSqlDatabase::removeDatabase(connName);
    return true;
}

QString DatabaseManager::getVersionIdentifier(const QString &dbPath,
                                              const QString &itemId,
                                              const QString &backendId)
{
    QString connName = QString("dbmanager_get_%1_%2").arg(itemId, backendId);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(dbPath + "/timebuster.db");
    if (!db.open()) {
        qDebug() << "DatabaseManager: Failed to open database for get:" << db.lastError().text();
        return QString();
    }
    QSqlQuery query(db);
    query.prepare("SELECT versionIdentifier FROM version_identifiers WHERE itemId = ? AND backendId = ?");
    query.addBindValue(itemId);
    query.addBindValue(backendId);
    QString storedVer;
    if (query.exec() && query.next()) {
        storedVer = query.value(0).toString();
    } else {
        qDebug() << "DatabaseManager: No version identifier found for item" << itemId;
    }
    db.close();
    QSqlDatabase::removeDatabase(connName);
    return storedVer;
}
