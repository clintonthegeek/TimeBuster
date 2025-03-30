#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QMap>
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

    // Nested ChangeResolver
    class ChangeResolver {
    public:
        explicit ChangeResolver(SessionManager* parent) : m_session(parent) {}

        // Returns true if resolved (applied or discarded), false if user must intervene
        bool resolveUnappliedEdit(const QSharedPointer<CalendarItem>& item, const QString& newSummary);

    private:
        SessionManager* m_session; // Access to m_deltaChanges, etc.
    };

    ChangeResolver* resolver() { return &m_resolver; }

private:
    void saveToFile(const QString &collectionId);
    QString deltaFilePath(const QString &collectionId) const;

    CollectionController *m_collectionController;
    QMap<QString, QList<DeltaChange>> m_deltaChanges;
    ChangeResolver m_resolver{this}; // Instance
};

QDataStream &operator<<(QDataStream &out, const DeltaChange &delta); // New: Serialization
QDataStream &operator>>(QDataStream &in, DeltaChange &delta);       // New: Deserialization

#endif // SESSIONMANAGER_H
