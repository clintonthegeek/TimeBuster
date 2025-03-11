#include "collection.h"

Collection::Collection(const QString &id, const QString &name, QObject *parent)
    : QAbstractTableModel(parent), m_id(id), m_name(name)
{
    // Stubbed dummy data
    m_calendars.append(new Cal("cal1", "Test Calendar", this));
}

Collection::~Collection()
{
    qDeleteAll(m_calendars); // Clean up owned Cal objects
}

int Collection::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_calendars.size();
}

int Collection::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return 2; // Stubbed: ID and Name columns
}

QVariant Collection::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    Cal *cal = m_calendars.at(index.row());
    switch (index.column()) {
    case 0: return cal->id();
    case 1: return cal->name(); //implement displayName() once thre!
    default: return QVariant();
    }
}

Cal* Collection::calendar(const QString &id) const
{
    for (Cal *cal : m_calendars) {
        if (cal->id() == id) return cal;
    }
    return nullptr;
}

QVariant Collection::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section) {
    case 0: return "ID";
    case 1: return "Name";
    default: return QVariant();
    }
}

void Collection::addCal(Cal *cal)
{
    beginInsertRows(QModelIndex(), m_calendars.size(), m_calendars.size());
    m_calendars.append(cal);
    cal->setParent(this); // Take ownership
    endInsertRows();
    emit calendarsChanged();
}
