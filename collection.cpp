#include "collection.h"
#include <QDebug>

Collection::Collection(const QString &id, const QString &name, QObject *parent)
    : QAbstractTableModel(parent), m_id(id), m_name(name)
{
    // Stubbed dummy data
    //addCal(new Cal("cal1", "Test Calendar", this));
}

Collection::~Collection()
{
    // QSharedPointer handles deletion automatically when ref count hits 0
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

    Cal *cal = m_calendars.at(index.row()).data();
    switch (index.column()) {
    case 0: return cal->id();
    case 1: return cal->name();
    default: return QVariant();
    }
}

void Collection::clearCalendars()
{
    m_calendars.clear();
    emit calendarsChanged();
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
    if (!cal) return;
    beginInsertRows(QModelIndex(), m_calendars.size(), m_calendars.size());
    m_calendars.append(QSharedPointer<Cal>(cal));
    endInsertRows();
    emit calendarsChanged();
    qDebug() << "Collection: Added calendar" << cal->id() << "to" << m_id;
}

QList<Cal*> Collection::calendars() const
{
    QList<Cal*> rawPointers;
    for (const auto &cal : m_calendars) {
        rawPointers.append(cal.data());
    }
    return rawPointers;
}
