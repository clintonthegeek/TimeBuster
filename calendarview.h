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
    void refresh(); // New method to refresh the view

signals:
    void selectionChanged(); // New signal for when selection changes

private:
    QTableView *tableView;
    Cal *calModel; // Not owned - MainWindow or CollectionManager owns it
};

#endif // CALENDARVIEW_H
