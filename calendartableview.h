#ifndef CALENDARTABLEVIEW_H
#define CALENDARTABLEVIEW_H

#include "viewinterface.h"
#include <QTableView>

class CalendarTableView : public ViewInterface {
    Q_OBJECT
public:
    explicit CalendarTableView(Cal* cal, QWidget* parent = nullptr);
    ~CalendarTableView() override;

    void setActiveCal(Cal* cal) override;
    void refresh() override;

    // Legacy method for compatibility
    QSharedPointer<CalendarItem> selectedItem() const;

private slots:
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    QTableView* m_tableView;
};

#endif // CALENDARTABLEVIEW_H
