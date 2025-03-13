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
    ~Cal() override;

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    void addItem(CalendarItem *item); // Takes ownership via QSharedPointer
    QList<CalendarItem*> items() const; // Returns raw pointers for compatibility

    // QAbstractTableModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    QString m_id;
    QString m_name;
    QList<QSharedPointer<CalendarItem>> m_items; // Owned by Cal
};

#endif // CAL_H
