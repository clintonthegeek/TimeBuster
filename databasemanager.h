#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>
#include <QDateTime>  // For timestamps

class DatabaseManager
{
public:
    static bool initializeDatabase(const QString &collectionId, const QString &dbPath, QSqlDatabase &db);
    static bool createBaseSchema(QSqlDatabase &db);

    // New functions for version metadata
    // Note: dbPath here is the directory where the .db file (e.g., timebuster.db) is stored.
    static bool updateVersionIdentifier(const QString &dbPath, const QString &itemId, const QString &backendId, const QString &versionIdentifier, const QDateTime &lastSyncTimestamp);
    static QString getVersionIdentifier(const QString &dbPath, const QString &itemId, const QString &backendId);
};

#endif // DATABASEMANAGER_H
