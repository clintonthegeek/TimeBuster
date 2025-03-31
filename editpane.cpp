#include "editpane.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QLabel>

EditPane::EditPane(Collection *collection, QWidget *parent)
    : ViewInterface(collection, parent), m_items()
{
    m_widget = new QWidget(parent); // Parent is MainWindow or dock
    m_layout = new QFormLayout(m_widget);
    m_summaryEdit = new QLineEdit(m_widget);
    m_dtStartEdit = new KDateTimeEdit(m_widget);
    m_dtEndDueEdit = new KDateTimeEdit(m_widget);
    m_categoriesCombo = new QComboBox(m_widget);
    m_descriptionEdit = new QTextEdit(m_widget);
    m_allDayCheck = new QCheckBox("All Day", m_widget);
    m_applyButton = new QPushButton("Apply", m_widget);

    m_layout->addRow("Summary:", m_summaryEdit);
    m_layout->addRow("Start:", m_dtStartEdit);
    m_layout->addRow("End/Due:", m_dtEndDueEdit);
    m_layout->addRow("Categories:", m_categoriesCombo);
    m_layout->addRow("Description:", m_descriptionEdit);
    m_layout->addRow(m_allDayCheck);
    m_layout->addRow(m_applyButton);

    m_dtEndDueEdit->setVisible(false);
    m_categoriesCombo->setVisible(false);
    m_descriptionEdit->setVisible(false);

    connect(m_allDayCheck, &QCheckBox::toggled, this, &EditPane::onAllDayToggled);
    connect(m_applyButton, &QPushButton::clicked, this, &EditPane::onApplyClicked);

    onAllDayToggled(false);
    qDebug() << "EditPane: Initialized";
}

EditPane::~EditPane()
{
    // delete m_widget; // Removed—Qt owns it via parenting
    qDebug() << "EditPane: Destroyed";
}

void EditPane::setActiveCal(Cal *cal)
{
    if (m_activeCal != cal) {
        m_activeCal = cal;
        // Don’t clear m_items or call refresh() here
        emit calChanged(cal);
        qDebug() << "EditPane: Set activeCal to" << (cal ? cal->id() : "null");
    }
}

void EditPane::setCollection(Collection *collection)
{
    ViewInterface::setCollection(collection);
    m_items.clear();
    refresh();
    qDebug() << "EditPane: Cleared selection on collection change";
}

void EditPane::refresh()
{
    if (m_items.isEmpty()) {
        m_summaryEdit->clear();
        m_dtStartEdit->setDateTime(QDateTime());
        m_dtEndDueEdit->setDateTime(QDateTime());
        m_categoriesCombo->clear();
        m_descriptionEdit->clear();
        m_allDayCheck->setChecked(false);
        m_dtEndDueEdit->setVisible(false);
    }
    qDebug() << "EditPane: Refreshed with" << m_items.size() << "items";
}

void EditPane::updateSelection(const QList<QSharedPointer<CalendarItem>> &items)
{
    m_items = items;
    if (items.isEmpty()) {
        m_summaryEdit->clear();
        m_dtStartEdit->setDateTime(QDateTime());
        m_dtEndDueEdit->setDateTime(QDateTime()); // Fixed
        m_categoriesCombo->clear();
        m_descriptionEdit->clear();
        m_allDayCheck->setChecked(false);
        m_dtEndDueEdit->setVisible(false);
    } else if (items.size() == 1) {
        auto item = items.first();
        m_summaryEdit->setText(item->incidence()->summary());
        m_dtStartEdit->setDateTime(item->dtStart());
        m_allDayCheck->setChecked(item->allDay());
        QDateTime endOrDue = item->dtEndOrDue();
        m_dtEndDueEdit->setDateTime(endOrDue); // Fixed
        m_dtEndDueEdit->setVisible(endOrDue.isValid());
        m_categoriesCombo->clear();
        m_categoriesCombo->addItems(item->categories());
        m_descriptionEdit->setText(item->description());
        onAllDayToggled(m_allDayCheck->isChecked());
    }
    qDebug() << "EditPane: Updated selection with" << items.size() << "items";
}

void EditPane::onApplyClicked()
{
    if (m_items.isEmpty()) return;
    for (auto &item : m_items) {
        item->incidence()->setSummary(m_summaryEdit->text());
        item->setDtStart(m_dtStartEdit->dateTime());
        item->setDtEndOrDue(m_dtEndDueEdit->dateTime()); // Fixed
        item->setCategories(m_categoriesCombo->currentText().split(",", Qt::SkipEmptyParts));
        item->setDescription(m_descriptionEdit->toPlainText());
        item->setAllDay(m_allDayCheck->isChecked());
        item->setDirty(true);
    }
    emit itemModified(m_items);
    qDebug() << "EditPane: Applied batch edit to" << m_items.size() << "items";
}

void EditPane::onAllDayToggled(bool checked)
{
    if (checked) {
        m_dtStartEdit->setOptions(KDateTimeEdit::ShowDate);
        m_dtEndDueEdit->setOptions(KDateTimeEdit::ShowDate);
    } else {
        m_dtStartEdit->setOptions(KDateTimeEdit::ShowDate | KDateTimeEdit::ShowTime);
        m_dtEndDueEdit->setOptions(KDateTimeEdit::ShowDate | KDateTimeEdit::ShowTime);
    }
    qDebug() << "EditPane: All Day toggled to" << checked;
}

bool EditPane::summariesDiffer() const
{
    if (m_items.size() <= 1) return false;
    QString firstSummary = m_items[0]->incidence()->summary();
    for (int i = 1; i < m_items.size(); ++i) {
        if (m_items[i]->incidence()->summary() != firstSummary) return true;
    }
    return false;
}
