#include "cal.h"
#include "collection.h"
#include <QDebug>

Cal::Cal(const QString &id, const QString &name, Collection *parent)
    : QAbstractTableModel(parent), m_id(id), m_name(name)
{
    qDebug() << "Cal: Created with id" << m_id << "name" << m_name;
}

void Cal::addItem(QSharedPointer<CalendarItem> item)
{
    if (!item) {
        qDebug() << "Cal: Cannot add null item to" << m_id;
        return;
    }
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    endInsertRows();
    //qDebug() << "Cal: Added item" << item->id() << "to" << m_id;
}

void Cal::updateItem(const QSharedPointer<CalendarItem> &item)
{
    if (!item) {
        qDebug() << "Cal: Cannot update with null item in" << m_id;
        return;
    }
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->id() == item->id()) {
            m_items[i] = item;
            emit dataChanged(index(i, 0), index(i, columnCount() - 1));
            qDebug() << "Cal: Updated item" << item->id() << "in" << m_id;
            return;
        }
    }
    qDebug() << "Cal: Item" << item->id() << "not found for update in" << m_id;
}

void Cal::removeItem(const QSharedPointer<CalendarItem> &item)
{
    if (!item) {
        qDebug() << "Cal: Cannot remove null item from" << m_id;
        return;
    }
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->id() == item->id()) {
            beginRemoveRows(QModelIndex(), i, i);
            m_items.removeAt(i);
            endRemoveRows();
            qDebug() << "Cal: Removed item" << item->id() << "from" << m_id;
            return;
        }
    }
    qDebug() << "Cal: Item" << item->id() << "not found for removal in" << m_id;
}

QModelIndex Cal::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent) || parent.isValid()) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

QModelIndex Cal::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int Cal::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

int Cal::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return 4; // Type, Summary, Start, End/Due
}

QVariant Cal::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) return QVariant();
    CalendarItem *item = m_items[index.row()].data();
    switch (index.column()) {
    case 0: return item->type();
    case 1: return item->data(Qt::DisplayRole); // Summary
    case 2: return item->data(Qt::UserRole);    // Start
    case 3: return item->data(Qt::UserRole + 1); // End or Due
    default: return QVariant();
    }
}

QVariant Cal::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return QVariant();
    switch (section) {
    case 0: return "Type";
    case 1: return "Summary";
    case 2: return "Start";
    case 3: return "End/Due";
    default: return QVariant();
    }
}
