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

    // Connect selection changes to emit our signal
    connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CalendarView::selectionChanged);

    // Basic sizing - adjustable later
    resize(400, 300);
}

CalendarView::~CalendarView()
{
    // tableView is a child, deleted by Qt
    // calModel is not owned, no delete here
}

void CalendarView::refresh()
{
    tableView->setModel(nullptr); // Detach model
    tableView->setModel(calModel); // Reattach to force refresh
    qDebug() << "CalendarView: Refreshed view for" << calModel->name();
}

QSharedPointer<CalendarItem> CalendarView::selectedItem() const
{
    QModelIndex index = tableView->currentIndex();
    if (!index.isValid() || index.row() >= calModel->items().size()) {
        return QSharedPointer<CalendarItem>(); // Null pointer if no selection
    }
    return calModel->items().at(index.row());
}

