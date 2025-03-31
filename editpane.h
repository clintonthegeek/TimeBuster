#ifndef EDITPANE_H
#define EDITPANE_H

#include "viewinterface.h"
#include "collectioncontroller.h" // Added for CollectionController
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QCheckBox>
#include <QFormLayout>
#include <KDateTimeEdit>

class EditPane : public ViewInterface {
    Q_OBJECT
public:
explicit EditPane(Collection *collection, QWidget *parent = nullptr);
    ~EditPane() override;
    QWidget *widget() const { return m_widget; } // Add getter

    void setActiveCal(Cal *cal) override;
    void refresh() override;
    void setCollection(Collection *collection) override;

public slots:
    void updateSelection(const QList<QSharedPointer<CalendarItem>> &items);

private slots:
    void onApplyClicked();
    void onAllDayToggled(bool checked);

private:
    bool summariesDiffer() const;
    QWidget *m_widget;
    Cal *m_activeCal;
    QList<QSharedPointer<CalendarItem>> m_items;
    QFormLayout *m_layout;
    QLineEdit *m_summaryEdit;
    KDateTimeEdit *m_dtStartEdit;
    KDateTimeEdit *m_dtEndDueEdit;
    QComboBox *m_categoriesCombo;
    QTextEdit *m_descriptionEdit;
    QCheckBox *m_allDayCheck;
    QPushButton *m_applyButton;
};

#endif // EDITPANE_H
