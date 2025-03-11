#include "cal.h"
#include "collection.h"

Cal::Cal(const QString &id, const QString &name, Collection *parent)
    : QAbstractTableModel(parent), m_id(id), m_name(name)
{
        qDebug() << "Cal: Created with id" << m_id << "name" << m_name;
}

Cal::~Cal()
{
    qDeleteAll(m_items);
}

void Cal::addItem(CalendarItem *item)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    endInsertRows();
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
    CalendarItem *item = m_items[index.row()];
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
