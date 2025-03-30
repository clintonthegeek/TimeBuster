#include "editpane.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QLabel> // Added for casting

EditPane::EditPane(Collection* collection, QWidget* parent)
    : ViewInterface(collection, parent),
    m_layout(new QFormLayout(this)),
    m_summaryEdit(new QLineEdit(this)),
    m_dtStartEdit(new QDateTimeEdit(this)),
    m_dtEndDueEdit(new QDateTimeEdit(this)),
    m_categoriesCombo(new QComboBox(this)),
    m_descriptionEdit(new QTextEdit(this)),
    m_allDayCheck(new QCheckBox("All Day", this)),
    m_applyButton(new QPushButton("Apply", this)),
    m_items()
{
    // Setup widgets
    m_dtStartEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_dtEndDueEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_categoriesCombo->setEditable(true);
    m_categoriesCombo->addItems({"Work", "Personal", "Urgent", ""});
    m_descriptionEdit->setMaximumHeight(100);

    // Add to layout
    m_layout->addRow("Summary:", m_summaryEdit);
    m_layout->addRow("Start:", m_dtStartEdit);
    m_layout->addRow("End/Due:", m_dtEndDueEdit); // Label will be adjusted dynamically
    m_layout->addRow("Categories:", m_categoriesCombo);
    m_layout->addRow("Description:", m_descriptionEdit);
    m_layout->addRow("", m_allDayCheck);
    m_layout->addRow("", m_applyButton);

    // Connections
    connect(m_applyButton, &QPushButton::clicked, this, &EditPane::onApplyClicked);
    connect(m_allDayCheck, &QCheckBox::toggled, this, &EditPane::onAllDayToggled);

    // Initial state
    setLayout(m_layout);
    onAllDayToggled(false);
    setEnabled(false);
    qDebug() << "EditPane: Initialized";
}

EditPane::~EditPane()
{
    qDebug() << "EditPane: Destroyed";
}

void EditPane::setActiveCal(Cal* cal)
{
    if (m_activeCal != cal) {
        m_activeCal = cal;
        m_items.clear();
        refresh();
        emit calChanged(cal);
        qDebug() << "EditPane: Set activeCal to" << (cal ? cal->id() : "null");
    }
}

void EditPane::setCollection(Collection* collection)
{
    ViewInterface::setCollection(collection);
    m_items.clear();
    refresh();
    qDebug() << "EditPane: Cleared selection on collection change";
}

void EditPane::refresh()
{
    if (m_items.isEmpty()) {
        setEnabled(false);
        m_summaryEdit->clear();
        m_dtStartEdit->setDateTime(QDateTime::currentDateTime());
        m_dtEndDueEdit->setDateTime(QDateTime::currentDateTime().addSecs(3600));
        m_categoriesCombo->setCurrentText("");
        m_descriptionEdit->clear();
        m_allDayCheck->setChecked(false);
    } else {
        setEnabled(true);
        updateSelection(m_items);
    }
    qDebug() << "EditPane: Refreshed with" << m_items.size() << "items";
}

void EditPane::updateSelection(const QList<QSharedPointer<CalendarItem>>& items)
{
    m_items = items;
    if (items.isEmpty()) {
        refresh();
        return;
    }

    setEnabled(true);
    bool allEvents = true, allTodos = true;
    for (const auto& item : items) {
        if (item->type() != "Event") allEvents = false;
        if (item->type() != "Todo") allTodos = false;
    }

    // Adjust label based on type
    QLabel* endDueLabel = qobject_cast<QLabel*>(m_layout->labelForField(m_dtEndDueEdit));
    if (endDueLabel) {
        endDueLabel->setText(allTodos ? "Due:" : "End:");
    } else {
        qDebug() << "EditPane: Failed to cast end/due label to QLabel";
    }

    // TODO: Fill fields with item data (Milestone 3)
    qDebug() << "EditPane: Updated selection with" << items.size() << "items";
}

void EditPane::onApplyClicked()
{
    qDebug() << "EditPane: Apply clicked—stub";
}

void EditPane::onAllDayToggled(bool checked)
{
    m_dtStartEdit->setDisplayFormat(checked ? "yyyy-MM-dd" : "yyyy-MM-dd HH:mm");
    m_dtEndDueEdit->setDisplayFormat(checked ? "yyyy-MM-dd" : "yyyy-MM-dd HH:mm");
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
