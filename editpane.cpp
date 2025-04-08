#include "editpane.h"
#include <QDebug>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>
#include <QDateTimeEdit>
#include <QScrollBar>

EditPane::EditPane(Collection *collection, QWidget *parent)
    : ViewInterface(collection, parent), m_items()
{
    m_widget = new QWidget(parent);
    m_layout = new QVBoxLayout(m_widget);
    m_propertiesTable = new QTableWidget(m_widget);
    m_applyButton = new QPushButton("Apply", m_widget);

    m_propertiesTable->setColumnCount(2);
    m_propertiesTable->setHorizontalHeaderLabels({"Property", "Value"});
    m_propertiesTable->horizontalHeader()->setStretchLastSection(true);
    m_propertiesTable->verticalHeader()->hide();
    m_propertiesTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_propertiesTable->setEditTriggers(QAbstractItemView::AllEditTriggers);

    // Set column resizing and disable horizontal scrolling
    m_propertiesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_propertiesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_propertiesTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_propertiesTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Enable word wrapping and set delegate
    m_propertiesTable->setWordWrap(true);
    m_wrappingDelegate = new WrappingDelegate(m_propertiesTable);
    m_propertiesTable->setItemDelegateForColumn(1, m_wrappingDelegate);

    m_layout->addWidget(m_propertiesTable);
    m_layout->addWidget(m_applyButton);

    connect(m_applyButton, &QPushButton::clicked, this, &EditPane::onApplyClicked);
    connect(m_propertiesTable, &QTableWidget::cellChanged, this, &EditPane::onCellChanged);

    refresh();
    qDebug() << "EditPane: Initialized";
}

EditPane::~EditPane()
{
    delete m_wrappingDelegate;
    qDebug() << "EditPane: Destroyed";
}

void EditPane::setActiveCal(Cal *cal)
{
    if (m_activeCal != cal) {
        m_activeCal = cal;
        emit calChanged(cal);
        qDebug() << "EditPane: Set activeCal to" << (cal ? cal->id() : "null");
        refresh(); // Update the UI based on the new m_activeCal
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
    m_propertiesTable->clearContents();
    if (m_items.isEmpty()) {
        m_propertiesTable->setRowCount(0);
        qDebug() << "EditPane: Refreshed with 0 items";
        return;
    }

    bool allEvents = true, allTodos = true;
    for (const auto &item : m_items) {
        if (item->type() != "Event") allEvents = false;
        if (item->type() != "Todo") allTodos = false;
    }

    QStringList properties = {"Summary", "Start", allTodos ? "Due" : "End", "All Day", "Categories", "Description"};
    m_propertiesTable->setRowCount(properties.size());

    for (int row = 0; row < properties.size(); ++row) {
        QTableWidgetItem *propertyItem = new QTableWidgetItem(properties[row]);
        propertyItem->setFlags(Qt::ItemIsEnabled);
        m_propertiesTable->setItem(row, 0, propertyItem);
    }

    updateSelection(m_items);
    qDebug() << "EditPane: Refreshed with" << m_items.size() << "items";
}

void EditPane::updateSelection(const QList<QSharedPointer<CalendarItem>> &items)
{
    m_items = items;
    m_modifiedRows.clear();

    // Clear only the "Value" column (column 1), preserve "Property" column (column 0)
    for (int row = 0; row < m_propertiesTable->rowCount(); ++row) {
        m_propertiesTable->removeCellWidget(row, 1);
        m_propertiesTable->setItem(row, 1, nullptr); // Clear value column
    }

    if (m_items.isEmpty()) {
        m_propertiesTable->setRowCount(0);
        qDebug() << "EditPane: Updated selection with 0 items";
        return;
    }

    bool allEvents = true, allTodos = true;
    for (const auto &item : m_items) {
        if (item->type() != "Event") allEvents = false;
        if (item->type() != "Todo") allTodos = false;
    }

    QStringList properties = {"Summary", "Start", allTodos ? "Due" : "End", "All Day", "Categories", "Description"};
    if (m_propertiesTable->rowCount() != properties.size()) {
        m_propertiesTable->setRowCount(properties.size());
        for (int row = 0; row < properties.size(); ++row) {
            QTableWidgetItem *propertyItem = new QTableWidgetItem(properties[row]);
            propertyItem->setFlags(Qt::ItemIsEnabled);
            m_propertiesTable->setItem(row, 0, propertyItem);
        }
    }

    if (m_items.size() == 1) {
        auto item = m_items.first();
        m_propertiesTable->setItem(0, 1, new QTableWidgetItem(item->incidence()->summary()));

        // Start with QDateTimeEdit
        QDateTime startDt = item->dtStart();
        if (startDt.isValid()) {
            QDateTimeEdit *startEdit = new QDateTimeEdit();
            startEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
            startEdit->setDateTime(startDt);
            m_propertiesTable->setCellWidget(1, 1, startEdit);
        } else {
            m_propertiesTable->setItem(1, 1, new QTableWidgetItem("Not Set"));
        }

        // End/Due with QDateTimeEdit
        QDateTime endDueDt = item->dtEndOrDue();
        if (endDueDt.isValid()) {
            QDateTimeEdit *endDueEdit = new QDateTimeEdit();
            endDueEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
            endDueEdit->setDateTime(endDueDt);
            m_propertiesTable->setCellWidget(2, 1, endDueEdit);
        } else {
            m_propertiesTable->setItem(2, 1, new QTableWidgetItem("Not Set"));
        }

        // All Day as checkbox
        QCheckBox *allDayCheck = new QCheckBox();
        allDayCheck->setChecked(item->allDay());
        m_propertiesTable->setCellWidget(3, 1, allDayCheck);

        // Connect All Day to toggle time visibility
        QDateTimeEdit *startEdit = qobject_cast<QDateTimeEdit*>(m_propertiesTable->cellWidget(1, 1));
        QDateTimeEdit *endDueEdit = qobject_cast<QDateTimeEdit*>(m_propertiesTable->cellWidget(2, 1));
        if (startEdit && endDueEdit) {
            connect(allDayCheck, &QCheckBox::toggled, this, [startEdit, endDueEdit](bool checked) {
                startEdit->setDisplayFormat(checked ? "yyyy-MM-dd" : "yyyy-MM-dd hh:mm");
                endDueEdit->setDisplayFormat(checked ? "yyyy-MM-dd" : "yyyy-MM-dd hh:mm");
            });
        }

        m_propertiesTable->setItem(4, 1, new QTableWidgetItem(item->categories().join(", ")));
        m_propertiesTable->setItem(5, 1, new QTableWidgetItem(item->description()));
    } else {
        m_propertiesTable->setItem(0, 1, new QTableWidgetItem(summariesDiffer() ? "<Multiple Values>" : m_items[0]->incidence()->summary()));

        // Start
        QDateTime firstDtStart = m_items[0]->dtStart();
        bool dtStartDiffer = false;
        for (int i = 1; i < m_items.size(); ++i) {
            if (m_items[i]->dtStart() != firstDtStart) {
                dtStartDiffer = true;
                break;
            }
        }
        if (dtStartDiffer || !firstDtStart.isValid()) {
            m_propertiesTable->setItem(1, 1, new QTableWidgetItem("<Multiple Values>"));
        } else {
            QDateTimeEdit *startEdit = new QDateTimeEdit();
            startEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
            startEdit->setDateTime(firstDtStart);
            m_propertiesTable->setCellWidget(1, 1, startEdit);
        }

        // End/Due
        QDateTime firstDtEndDue = m_items[0]->dtEndOrDue();
        bool dtEndDueDiffer = false;
        for (int i = 1; i < m_items.size(); ++i) {
            if (m_items[i]->dtEndOrDue() != firstDtEndDue) {
                dtEndDueDiffer = true;
                break;
            }
        }
        if (dtEndDueDiffer || !firstDtEndDue.isValid()) {
            m_propertiesTable->setItem(2, 1, new QTableWidgetItem("<Multiple Values>"));
        } else {
            QDateTimeEdit *endDueEdit = new QDateTimeEdit();
            endDueEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
            endDueEdit->setDateTime(firstDtEndDue);
            m_propertiesTable->setCellWidget(2, 1, endDueEdit);
        }

        // All Day
        bool firstAllDay = m_items[0]->allDay();
        bool allDayDiffer = false;
        for (int i = 1; i < m_items.size(); ++i) {
            if (m_items[i]->allDay() != firstAllDay) {
                allDayDiffer = true;
                break;
            }
        }
        QCheckBox *allDayCheck = new QCheckBox();
        allDayCheck->setChecked(firstAllDay && !allDayDiffer);
        allDayCheck->setEnabled(!allDayDiffer);
        m_propertiesTable->setCellWidget(3, 1, allDayCheck);

        // Connect All Day to toggle time visibility (if no differences)
        if (!dtStartDiffer && !dtEndDueDiffer && firstDtStart.isValid() && firstDtEndDue.isValid()) {
            QDateTimeEdit *startEdit = qobject_cast<QDateTimeEdit*>(m_propertiesTable->cellWidget(1, 1));
            QDateTimeEdit *endDueEdit = qobject_cast<QDateTimeEdit*>(m_propertiesTable->cellWidget(2, 1));
            if (startEdit && endDueEdit) {
                startEdit->setEnabled(!allDayDiffer);
                endDueEdit->setEnabled(!allDayDiffer);
                connect(allDayCheck, &QCheckBox::toggled, this, [startEdit, endDueEdit](bool checked) {
                    startEdit->setDisplayFormat(checked ? "yyyy-MM-dd" : "yyyy-MM-dd hh:mm");
                    endDueEdit->setDisplayFormat(checked ? "yyyy-MM-dd" : "yyyy-MM-dd hh:mm");
                });
            }
        }

        // Categories
        QStringList firstCats = m_items[0]->categories();
        bool catsDiffer = false;
        for (int i = 1; i < m_items.size(); ++i) {
            if (m_items[i]->categories() != firstCats) {
                catsDiffer = true;
                break;
            }
        }
        m_propertiesTable->setItem(4, 1, new QTableWidgetItem(catsDiffer ? "<Multiple Values>" : firstCats.join(", ")));

        // Description
        QString firstDesc = m_items[0]->description();
        bool descDiffer = false;
        for (int i = 1; i < m_items.size(); ++i) {
            if (m_items[i]->description() != firstDesc) {
                descDiffer = true;
                break;
            }
        }
        m_propertiesTable->setItem(5, 1, new QTableWidgetItem(descDiffer ? "<Multiple Values>" : firstDesc));
    }

    m_propertiesTable->resizeColumnsToContents();
    m_propertiesTable->resizeRowsToContents(); // Adjust row heights for wrapped text
    m_propertiesTable->horizontalScrollBar()->setValue(0);
    qDebug() << "EditPane: Updated selection with" << items.size() << "items";
}

void EditPane::onApplyClicked()
{
    if (m_items.isEmpty()) {
        qDebug() << "EditPane: No items to apply changes to";
        return;
    }

    for (auto &item : m_items) {
        if (m_modifiedRows.contains(0)) {
            QTableWidgetItem *summaryItem = m_propertiesTable->item(0, 1);
            if (summaryItem && summaryItem->text() != "<Multiple Values>")
                item->incidence()->setSummary(summaryItem->text());
        }

        if (m_modifiedRows.contains(1)) {
            QDateTimeEdit *startEdit = qobject_cast<QDateTimeEdit*>(m_propertiesTable->cellWidget(1, 1));
            if (startEdit) {
                item->setDtStart(startEdit->dateTime());
            } else {
                QTableWidgetItem *dtStartItem = m_propertiesTable->item(1, 1);
                if (dtStartItem && dtStartItem->text() != "<Multiple Values>" && dtStartItem->text() != "Not Set")
                    item->setDtStart(QDateTime::fromString(dtStartItem->text(), "yyyy-MM-dd hh:mm"));
            }
        }

        if (m_modifiedRows.contains(2)) {
            QDateTimeEdit *endDueEdit = qobject_cast<QDateTimeEdit*>(m_propertiesTable->cellWidget(2, 1));
            if (endDueEdit) {
                item->setDtEndOrDue(endDueEdit->dateTime());
            } else {
                QTableWidgetItem *dtEndDueItem = m_propertiesTable->item(2, 1);
                if (dtEndDueItem && dtEndDueItem->text() != "<Multiple Values>" && dtEndDueItem->text() != "Not Set")
                    item->setDtEndOrDue(QDateTime::fromString(dtEndDueItem->text(), "yyyy-MM-dd hh:mm"));
            }
        }

        if (m_modifiedRows.contains(3)) {
            QCheckBox *allDayCheck = qobject_cast<QCheckBox*>(m_propertiesTable->cellWidget(3, 1));
            if (allDayCheck)
                item->setAllDay(allDayCheck->isChecked());
        }

        if (m_modifiedRows.contains(4)) {
            QTableWidgetItem *categoriesItem = m_propertiesTable->item(4, 1);
            if (categoriesItem && categoriesItem->text() != "<Multiple Values>")
                item->setCategories(categoriesItem->text().split(", ", Qt::SkipEmptyParts));
        }

        if (m_modifiedRows.contains(5)) {
            QTableWidgetItem *descriptionItem = m_propertiesTable->item(5, 1);
            if (descriptionItem && descriptionItem->text() != "<Multiple Values>")
                item->setDescription(descriptionItem->text());
        }

        item->setDirty(true);
    }

    emit itemModified(m_items);
    m_modifiedRows.clear();
    qDebug() << "EditPane: Applied batch edit to" << m_items.size() << "items";
}

void EditPane::onCellChanged(int row, int column)
{
    if (column == 1) {
        m_modifiedRows.insert(row);
    }
    qDebug() << "EditPane: Cell changed at row" << row << "column" << column;
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
