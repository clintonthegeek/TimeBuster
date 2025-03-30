#ifndef CAL_H
#define CAL_H

#include <QObject>
#include <QAbstractTableModel>
#include "calendaritem.h"
#include <QSharedPointer>

class Collection;

class Cal : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)

public:
    explicit Cal(const QString &id, const QString &name, Collection *parent = nullptr);
    ~Cal() override = default;

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    QString calId() const { return m_calId; } // Add if not present
    Collection* parentCollection() const { return m_parent; } // New getter
    void addItem(QSharedPointer<CalendarItem> item);
    QList<QSharedPointer<CalendarItem>> items() const { return m_items; }
    void updateItem(const QSharedPointer<CalendarItem> &item);
    void removeItem(const QSharedPointer<CalendarItem> &item);

    // QAbstractTableModel
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QString m_id;
    QString m_name;
    QString m_calId; // Set in constructor
    QList<QSharedPointer<CalendarItem>> m_items;
    Collection* m_parent; // New member to store parent explicitly
};

#endif // CAL_H
