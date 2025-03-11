#include "cal.h"

Cal::Cal(const QString &id, const QString &displayName, QObject *parent)
    : QAbstractTableModel(parent), m_id(id), m_displayName(displayName)
{
    // Stubbed dummy data
    m_items.append(new Event("event1", this));
    m_items.append(new Todo("todo1", this));
}

Cal::~Cal()
{
    qDeleteAll(m_items); // Clean up owned items
}

int Cal::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

int Cal::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return 2; // Stubbed: UID and Summary columns
}

QVariant Cal::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    CalendarItem *item = m_items.at(index.row());
    switch (index.column()) {
    case 0: return item->uid();
    case 1: return item->summary();
    default: return QVariant();
    }
}

QVariant Cal::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section) {
    case 0: return "UID";
    case 1: return "Summary";
    default: return QVariant();
    }
}

void Cal::addItem(CalendarItem *item)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    item->setParent(this); // Take ownership
    endInsertRows();
    emit itemsChanged();
}
