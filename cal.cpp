#include "cal.h"
#include "collection.h"
#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>

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
    if (!item) {
        qDebug() << "Cal: Attempted to add null item to calendar" << m_id;
        return;
    }
    // Check for duplicates by ID
    for (const CalendarItem *existing : m_items) {
        if (existing && existing->id() == item->id()) {
            qDebug() << "Cal: Item with ID" << item->id() << "already exists in calendar" << m_id;
            return;
        }
    }
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    item->setParent(this); // Ensure ownership
    endInsertRows();
    qDebug() << "Cal: Added item" << item->id() << "to calendar" << m_id;
}

void Cal::refreshModel()
{
    beginResetModel();
    endResetModel();
    qDebug() << "Cal: Model refreshed for" << m_name;
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

bool Cal::updateItem(int row, const QString &summary)
{
    if (row < 0 || row >= m_items.size()) return false;
    CalendarItem *item = m_items[row];
    if (item->incidence()) {
        item->incidence()->setSummary(summary);
        QModelIndex topLeft = index(row, 1); // Summary column
        QModelIndex bottomRight = index(row, columnCount() - 1);
        emit dataChanged(topLeft, bottomRight);
        return true;
    }
    return false;
}

QVariant Cal::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) return QVariant();
    if (index.row() < 0 || index.row() >= m_items.size()) return QVariant(); // Add bounds check
    CalendarItem *item = m_items[index.row()];
    if (!item) return QVariant(); // Add null check
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
