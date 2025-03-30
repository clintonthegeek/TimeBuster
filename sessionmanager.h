#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QMap>
#include <QDateTime>
#include "syncbackend.h"
#include "collectioncontroller.h"
#include "collection.h"
#include "deltachange.h"

class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(CollectionController *controller, QObject *parent = nullptr);
    void queueDeltaChange(const QString &calId, const QSharedPointer<CalendarItem> &item, DeltaChange::Type change);
    void applyDeltaChanges();
    void loadStagedChanges(const QString &collectionId);
    void undoLastCommit();
    void redoLastUndo();

    // Nested ChangeResolver
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
        QList<DeltaChange> changes;
    };
    QList<Commit> history() const { return m_history; }

private:
    void saveToFile(const QString &collectionId);
    void saveHistory(const QString &collectionId);
    QString deltaFilePath(const QString &collectionId) const;
    QString historyFilePath(const QString &collectionId) const;


    CollectionController *m_collectionController;
    QMap<QString, QList<DeltaChange>> m_deltaChanges;
    QList<Commit> m_history;
    QList<Commit> m_undoStack; // For redo
    ChangeResolver m_resolver{this}; // Instance
};

QDataStream &operator<<(QDataStream &out, const DeltaChange &delta); // New: Serialization
QDataStream &operator>>(QDataStream &in, DeltaChange &delta);       // New: Deserialization

QDataStream &operator<<(QDataStream &out, const SessionManager::Commit &commit);
QDataStream &operator>>(QDataStream &in, SessionManager::Commit &commit);

#endif // SESSIONMANAGER_H
