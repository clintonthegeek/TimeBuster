#ifndef COLLECTION_H
#define COLLECTION_H

#include <QAbstractTableModel>
#include "cal.h"
#include <QList>
#include <QSharedPointer>

class Collection : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit Collection(const QString &id, const QString &name, QObject *parent = nullptr);
    ~Collection() override;

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    void addCal(Cal *cal); // Takes ownership via QSharedPointer
    QList<Cal*> calendars() const; // Returns raw pointers for compatibility
    void clearCalendars(); // New method to clear calendars

signals:
    void calendarsChanged();

private:
    QString m_id; // Unique across app
    QString m_name; // User-facing name
    QList<QSharedPointer<Cal>> m_calendars; // Owned by Collection
};

#endif // COLLECTION_H
