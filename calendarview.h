#ifndef CALENDARVIEW_H
#define CALENDARVIEW_H

#include <QWidget>
#include <QTableView>
#include "cal.h"

class CalendarView : public QWidget
{
    Q_OBJECT

public:
    explicit CalendarView(Cal *cal, QWidget *parent = nullptr);
    ~CalendarView() override;

    Cal *model() const { return calModel; }
    QModelIndex currentIndex() const { return tableView->currentIndex(); }
    QSharedPointer<CalendarItem> selectedItem() const; // New method

public slots:
    void refresh(); // Already implemented, now a slot
//    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

signals:
    void selectionChanged(); // For selection changes

private:
    QTableView *tableView;
    Cal *calModel; // Not owned
};

#endif // CALENDARVIEW_H
