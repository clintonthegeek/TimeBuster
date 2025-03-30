#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QMap>
#include <QDateTime>
#include "syncbackend.h"
#include "collectioncontroller.h"
#include "collection.h"
#include "deltaentry.h"

class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(CollectionController *controller, QObject *parent = nullptr);
    void queueDeltaChange(const QString &calId, const QSharedPointer<CalendarItem> &item, const QString &userIntent);
    void applyDeltaChanges();
    void clearDeltaChanges(const QString &collectionId);
    void loadStagedChanges(const QString &collectionId);
    void undoLastCommit();
    void redoLastUndo();

    class ChangeResolver {
    public:
        explicit ChangeResolver(SessionManager* parent) : m_session(parent) {}
        bool resolveUnappliedEdit(Cal* cal, const QSharedPointer<CalendarItem>& item, const QString& newSummary);
    private:
        SessionManager* m_session;
    };

    ChangeResolver* resolver() { return &m_resolver; }

    struct Commit {
        QDateTime timestamp;
        QList<DeltaEntry> changes;
    };
    QList<Commit> history() const { return m_history; }

signals:
    void changesStaged(const QList<DeltaEntry> &changes);

private:
    void saveToFile(const QString &collectionId, bool cleanExit = false);
    void saveHistory(const QString &collectionId);
    QString deltaFilePath(const QString &collectionId) const;
    QString historyFilePath(const QString &collectionId) const;

    void saveDeltaEntries(const QString &collectionId);
    QList<DeltaEntry> loadDeltaEntries(const QString &collectionId);

    CollectionController *m_collectionController;
    QList<DeltaEntry> m_newDeltaChanges;
    QList<Commit> m_history;
    QList<Commit> m_undoStack;
    ChangeResolver m_resolver{this};
    QString m_sessionId;
};

QDataStream &operator<<(QDataStream &out, const SessionManager::Commit &commit);
QDataStream &operator>>(QDataStream &in, SessionManager::Commit &commit);

#endif // SESSIONMANAGER_H
