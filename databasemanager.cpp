#include "databasemanager.h"
#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>

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

    return true;
}
