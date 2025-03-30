#include "editpane.h"
#include "sessionmanager.h"
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
    layout->addStretch();
    setLayout(layout);

    connect(m_applyButton, &QPushButton::clicked, this, &EditPane::onApplyClicked);
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
        m_summaryEdit->setText(summariesDiffer() ? "<Multiple Items>" : m_items.first()->incidence()->summary());
        m_applyButton->setEnabled(true); // Allow batch edit
    }
    qDebug() << "EditPane: Refreshed with" << m_items.size() << "items";
}

void EditPane::updateSelection(const QList<QSharedPointer<CalendarItem>>& items)
{
    if (!m_items.isEmpty() && m_summaryEdit->isModified()) {
        SessionManager* session = qobject_cast<SessionManager*>(parent()->parent());
        for (const auto& oldItem : m_items) {
            if (session && session->resolver()->resolveUnappliedEdit(m_activeCal, oldItem, m_summaryEdit->text())) {
                qDebug() << "EditPane: Resolved unapplied edit for" << oldItem->id();
            } else {
                qDebug() << "EditPane: Failed to resolve unapplied edit—discarded";
            }
        }
    }

    m_items = items;
    refresh();
}

bool EditPane::summariesDiffer() const
{
    if (m_items.size() <= 1) return false;
    QString firstSummary = m_items.first()->incidence()->summary();
    for (const auto& item : m_items) {
        if (item->incidence()->summary() != firstSummary) return true;
    }
    return false;
}

void EditPane::onApplyClicked()
{
    if (m_items.isEmpty()) {
        qDebug() << "EditPane: Apply clicked with no selection";
        return;
    }

    QString newSummary = m_summaryEdit->text();
    QList<QSharedPointer<CalendarItem>> modifiedItems;
    for (const auto& item : m_items) {
        if (newSummary != item->incidence()->summary()) {
            item->incidence()->setSummary(newSummary);
            item->setDirty(true);
            if (m_activeCal) {
                m_activeCal->updateItem(item);
                modifiedItems.append(item);
            }
        }
    }

    if (!modifiedItems.isEmpty()) {
        emit itemModified(modifiedItems);
        qDebug() << "EditPane: Applied batch edit to" << modifiedItems.size() << "items in" << (m_activeCal ? m_activeCal->id() : "null");
    }
    m_summaryEdit->setModified(false);
}
