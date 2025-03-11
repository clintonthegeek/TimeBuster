#include "calendarview.h"
#include <QVBoxLayout>

CalendarView::CalendarView(Cal *cal, QWidget *parent)
    : QWidget(parent), calModel(cal)
{
    tableView = new QTableView(this);
    tableView->setModel(calModel);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(tableView);
    setLayout(layout);

    // Basic sizing - adjustable later
    resize(400, 300);
}

CalendarView::~CalendarView()
{
    // tableView is a child, deleted by Qt
    // calModel is not owned, no delete here
}
