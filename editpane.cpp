#include "editpane.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDebug>

EditPane::EditPane(Collection* collection, QWidget* parent)
    : ViewInterface(collection, parent),
    m_summaryEdit(new QLineEdit(this)),
    m_applyButton(new QPushButton("Apply", this)),
    m_items()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QFormLayout* form = new QFormLayout;
    form->addRow(new QLabel("Summary:"), m_summaryEdit);
    layout->addLayout(form);
    layout->addWidget(m_applyButton);
    layout->addStretch(); // Push content to top
    setLayout(layout);

    connect(m_applyButton, &QPushButton::clicked, this, &EditPane::onApplyClicked);

    qDebug() << "EditPane: Created for collection" << (collection ? collection->id() : "null");
}

EditPane::~EditPane()
{
    qDebug() << "EditPane: Destroyed";
}

void EditPane::setActiveCal(Cal* cal)
{
    m_activeCal = cal; // Passive: doesn’t emit calChanged
    qDebug() << "EditPane: Set activeCal to" << (cal ? cal->id() : "null");
}

void EditPane::setCollection(Collection* collection)
{
    ViewInterface::setCollection(collection);
    m_items.clear(); // Reset selection when collection changes
    refresh();
    qDebug() << "EditPane: Cleared selection on collection change";
}

void EditPane::refresh()
{
    if (m_items.isEmpty()) {
        m_summaryEdit->clear();
        m_applyButton->setEnabled(false);
    } else if (m_items.size() == 1) {
        m_summaryEdit->setText(m_items.first()->incidence()->summary());
        m_applyButton->setEnabled(true);
    } else {
        m_summaryEdit->setText("<Multiple Items>");
        m_applyButton->setEnabled(false);
    }
    qDebug() << "EditPane: Refreshed with" << m_items.size() << "items";
}

void EditPane::updateSelection(const QList<QSharedPointer<CalendarItem>>& items)
{
    if (!m_items.isEmpty() && m_summaryEdit->isModified()) {
        qDebug() << "EditPane: Unapplied changes detected for" << m_items.first()->id();
        // Stub for ChangeResolver in Milestone 4
        // For now, just discard and proceed
    }
    m_items = items;
    refresh();
}

void EditPane::onApplyClicked()
{
    if (m_items.isEmpty() || m_items.size() > 1) {
        qDebug() << "EditPane: Apply clicked with invalid selection size" << m_items.size();
        return;
    }

    QSharedPointer<CalendarItem> item = m_items.first();
    QString newSummary = m_summaryEdit->text();
    if (newSummary != item->incidence()->summary()) {
        item->incidence()->setSummary(newSummary);
        item->setDirty(true);
        if (m_activeCal) {
            m_activeCal->updateItem(item);
            emit itemModified({item});
            qDebug() << "EditPane: Applied edit to" << item->id() << "in" << m_activeCal->id();
        } else {
            qDebug() << "EditPane: No activeCal to apply edit";
        }
    }
    m_summaryEdit->setModified(false);
}
