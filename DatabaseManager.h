#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
/* Just a stub
 */
class DatabaseManager
{
public:
    static bool initializeDatabase(const QString &collectionId, const QString &rootPath, QSqlDatabase &db);
};

#endif // DATABASEMANAGER_H
