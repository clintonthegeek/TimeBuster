#ifndef COLLECTION_H
#define COLLECTION_H

#include <QAbstractTableModel>
#include "cal.h"
#include <QList>

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
    void setId(const QString &id); // New setter
    QString name() const { return m_name; }
    void addCal(Cal *cal);
    QList<Cal*> calendars() const { return m_calendars; }
    Cal* calendar(const QString &id) const;

signals:
    void calendarsChanged();
    void idChanged(const QString &oldId, const QString &newId);

private:
    QString m_id; // Unique across app
    QString m_name; // User-facing name
    QList<Cal*> m_calendars; // Owned by Collection
};

#endif // COLLECTION_H
