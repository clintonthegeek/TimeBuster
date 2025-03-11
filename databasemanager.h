#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>

class DatabaseManager
{
public:
    static bool initializeDatabase(const QString &collectionId, const QString &dbPath, QSqlDatabase &db);
    static bool createBaseSchema(QSqlDatabase &db);
};

#endif // DATABASEMANAGER_H
