#ifndef EDITPANE_H
#define EDITPANE_H

#include "viewinterface.h"
#include <QLineEdit>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QCheckBox>
#include <QFormLayout>

class EditPane : public ViewInterface {
    Q_OBJECT
public:
    explicit EditPane(Collection* collection, QWidget* parent = nullptr);
    ~EditPane() override;

    void setActiveCal(Cal* cal) override;
    void refresh() override;
    void setCollection(Collection* collection) override;

public slots:
    void updateSelection(const QList<QSharedPointer<CalendarItem>>& items);

private slots:
    void onApplyClicked();
    void onAllDayToggled(bool checked);

private:
    bool summariesDiffer() const;
    QFormLayout* m_layout;
    QLineEdit* m_summaryEdit;
    QDateTimeEdit* m_dtStartEdit;
    QDateTimeEdit* m_dtEndDueEdit; // Dual-purpose: dtEnd for events, due for todos
    QComboBox* m_categoriesCombo;
    QTextEdit* m_descriptionEdit;
    QCheckBox* m_allDayCheck;
    QPushButton* m_applyButton;
    QList<QSharedPointer<CalendarItem>> m_items;
};

#endif // EDITPANE_H
