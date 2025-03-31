#include "editpane.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QLabel>

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
    m_dtStartEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_dtEndDueEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_categoriesCombo->setEditable(true);
    m_categoriesCombo->addItems({"Work", "Personal", "Urgent", ""});
    m_descriptionEdit->setMaximumHeight(100);

    m_layout->addRow("Summary:", m_summaryEdit);
    m_layout->addRow("Start:", m_dtStartEdit);
    m_layout->addRow("End/Due:", m_dtEndDueEdit);
    m_layout->addRow("Categories:", m_categoriesCombo);
    m_layout->addRow("Description:", m_descriptionEdit);
    m_layout->addRow("", m_allDayCheck);
    m_layout->addRow("", m_applyButton);

    connect(m_applyButton, &QPushButton::clicked, this, &EditPane::onApplyClicked);
    connect(m_allDayCheck, &QCheckBox::toggled, this, &EditPane::onAllDayToggled);

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

    QLabel* endDueLabel = qobject_cast<QLabel*>(m_layout->labelForField(m_dtEndDueEdit));
    if (endDueLabel) {
        endDueLabel->setText(allTodos ? "Due:" : "End:");
    }

    // Single item: load directly; multiple: check for divergence
    if (m_items.size() == 1) {
        const auto& item = m_items[0];
        m_summaryEdit->setText(item->incidence()->summary());
        m_dtStartEdit->setDateTime(item->dtStart().isValid() ? item->dtStart() : QDateTime::currentDateTime());
        m_dtEndDueEdit->setDateTime(item->dtEndOrDue().isValid() ? item->dtEndOrDue() : QDateTime::currentDateTime().addSecs(3600));
        m_categoriesCombo->setCurrentText(item->categories().join(", "));
        m_descriptionEdit->setText(item->description());
        m_allDayCheck->setChecked(item->allDay());
    } else {
        // Summary
        m_summaryEdit->setText(summariesDiffer() ? "<Multiple Values>" : m_items[0]->incidence()->summary());

        // dtStart
        QDateTime firstDtStart = m_items[0]->dtStart();
        bool dtStartDiffer = false;
        for (int i = 1; i < m_items.size(); ++i) {
            if (m_items[i]->dtStart() != firstDtStart) {
                dtStartDiffer = true;
                break;
            }
        }
        m_dtStartEdit->setDateTime(dtStartDiffer ? QDateTime::currentDateTime() : firstDtStart);

        // dtEndOrDue
        QDateTime firstDtEndDue = m_items[0]->dtEndOrDue();
        bool dtEndDueDiffer = false;
        for (int i = 1; i < m_items.size(); ++i) {
            if (m_items[i]->dtEndOrDue() != firstDtEndDue) {
                dtEndDueDiffer = true;
                break;
            }
        }
        m_dtEndDueEdit->setDateTime(dtEndDueDiffer ? QDateTime::currentDateTime().addSecs(3600) : firstDtEndDue);

        // Categories
        QStringList firstCats = m_items[0]->categories();
        bool catsDiffer = false;
        for (int i = 1; i < m_items.size(); ++i) {
            if (m_items[i]->categories() != firstCats) {
                catsDiffer = true;
                break;
            }
        }
        m_categoriesCombo->setCurrentText(catsDiffer ? "<Multiple Values>" : firstCats.join(", "));

        // Description
        QString firstDesc = m_items[0]->description();
        bool descDiffer = false;
        for (int i = 1; i < m_items.size(); ++i) {
            if (m_items[i]->description() != firstDesc) {
                descDiffer = true;
                break;
            }
        }
        m_descriptionEdit->setText(descDiffer ? "<Multiple Values>" : firstDesc);

        // All Day
        bool firstAllDay = m_items[0]->allDay();
        bool allDayDiffer = false;
        for (int i = 1; i < m_items.size(); ++i) {
            if (m_items[i]->allDay() != firstAllDay) {
                allDayDiffer = true;
                break;
            }
        }
        m_allDayCheck->setChecked(firstAllDay && !allDayDiffer);
    }

    onAllDayToggled(m_allDayCheck->isChecked()); // Adjust display format
    qDebug() << "EditPane: Updated selection with" << items.size() << "items";
}

void EditPane::onApplyClicked()
{
    if (m_items.isEmpty()) return;

    bool allEvents = true, allTodos = true;
    for (const auto& item : m_items) {
        if (item->type() != "Event") allEvents = false;
        if (item->type() != "Todo") allTodos = false;
    }

    for (auto& item : m_items) {
        // Only update fields that aren't "<Multiple Values>"
        if (m_summaryEdit->text() != "<Multiple Values>")
            item->incidence()->setSummary(m_summaryEdit->text());
        if (m_dtStartEdit->dateTime().isValid())
            item->setDtStart(m_dtStartEdit->dateTime());
        if (m_dtEndDueEdit->dateTime().isValid())
            item->setDtEndOrDue(m_dtEndDueEdit->dateTime());
        if (m_categoriesCombo->currentText() != "<Multiple Values>") {
            QStringList cats = m_categoriesCombo->currentText().split(", ", Qt::SkipEmptyParts);
            item->setCategories(cats);
        }
        if (m_descriptionEdit->toPlainText() != "<Multiple Values>")
            item->setDescription(m_descriptionEdit->toPlainText());
        item->setAllDay(m_allDayCheck->isChecked());
    }

    emit itemModified(m_items);
    qDebug() << "EditPane: Applied batch edit to" << m_items.size() << "items in" << (m_activeCal ? m_activeCal->id() : "null");
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
