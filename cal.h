#ifndef CAL_H
#define CAL_H

#include <QAbstractTableModel>
#include "calendaritem.h"
#include <QList>

class Cal : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit Cal(const QString &id, const QString &displayName, QObject *parent = nullptr);
    ~Cal() override;

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QString id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    void addItem(CalendarItem *item);

signals:
    void itemsChanged();

private:
    QString m_id; // Unique across app
    QString m_displayName; // User-facing name
    QList<CalendarItem*> m_items; // Owned by Cal
};

#endif // CAL_H
