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

private:
    QTableView *tableView;
    Cal *calModel; // Not owned - MainWindow or CollectionManager owns it
};

#endif // CALENDARVIEW_H
