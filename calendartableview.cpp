#include "calendartableview.h"
//#include "cal.h"
#include <QVBoxLayout>
#include <QDebug>

CalendarTableView::CalendarTableView(Cal* cal, QWidget* parent)
    : ViewInterface(cal ? cal->parentCollection() : nullptr, parent), m_tableView(new QTableView(this))
{
    m_tableView->setModel(cal);
    m_activeCal = cal;

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_tableView);
    setLayout(layout);

    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CalendarTableView::onSelectionChanged);

    resize(400, 300);
    qDebug() << "CalendarTableView: Created for" << (cal ? cal->id() : "null");
}

CalendarTableView::~CalendarTableView()
{
    // m_tableView deleted by Qt as child
    qDebug() << "CalendarTableView: Destroyed";
}

void CalendarTableView::setActiveCal(Cal* cal)
{
    if (m_activeCal != cal) {
        m_activeCal = cal;
        m_tableView->setModel(cal);
        emit calChanged(cal);
        qDebug() << "CalendarTableView: Switched activeCal to" << (cal ? cal->id() : "null");
    }
}

void CalendarTableView::refresh()
{
    m_tableView->viewport()->update();
    qDebug() << "CalendarTableView: Refreshed view for" << (m_activeCal ? m_activeCal->name() : "null");
}

QSharedPointer<CalendarItem> CalendarTableView::selectedItem() const
{
    QModelIndex index = m_tableView->currentIndex();
    if (!index.isValid() || !m_activeCal || index.row() >= m_activeCal->items().size()) {
        return QSharedPointer<CalendarItem>();
    }
    return m_activeCal->items().at(index.row());
}

void CalendarTableView::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected);
    if (!m_activeCal) {
        qDebug() << "CalendarTableView: Selection changed but no activeCal";
        return;
    }

    QModelIndexList indexes = m_tableView->selectionModel()->selectedRows();
    QList<QSharedPointer<CalendarItem>> items;
    for (const QModelIndex& index : indexes) {
        if (index.row() >= 0 && index.row() < m_activeCal->items().size()) {
            items.append(m_activeCal->items().at(index.row()));
        }
    }

    emit itemSelected(items);
    qDebug() << "CalendarTableView: Emitted itemSelected for" << items.size() << "items in" << m_activeCal->id();
}
