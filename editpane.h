#ifndef EDITPANE_H
#define EDITPANE_H

#include "viewinterface.h"
#include <QLineEdit>
#include <QPushButton>

class EditPane : public ViewInterface {
    Q_OBJECT
public:
    explicit EditPane(Collection* collection, QWidget* parent = nullptr);
    ~EditPane() override;

    void setActiveCal(Cal* cal) override;
    void refresh() override;
    void setCollection(Collection* collection) override; // New

public slots:
    void updateSelection(const QList<QSharedPointer<CalendarItem>>& items);

private slots:
    void onApplyClicked();

private:
    QLineEdit* m_summaryEdit;
    QPushButton* m_applyButton;
    QList<QSharedPointer<CalendarItem>> m_items; // Current selection
};

#endif // EDITPANE_H
