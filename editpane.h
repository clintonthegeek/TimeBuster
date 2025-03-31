#ifndef EDITPANE_H
#define EDITPANE_H

#include "viewinterface.h"
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSet>
#include <QStyledItemDelegate>

class EditPane : public ViewInterface {
    Q_OBJECT
public:
    explicit EditPane(Collection *collection, QWidget *parent = nullptr);
    ~EditPane() override;
    QWidget *widget() const { return m_widget; }

    void setActiveCal(Cal *cal) override;
    void refresh() override;
    void setCollection(Collection *collection) override;

public slots:
    void updateSelection(const QList<QSharedPointer<CalendarItem>> &items);

private slots:
    void onApplyClicked();
    void onCellChanged(int row, int column);

private:
    bool summariesDiffer() const;
    QWidget *m_widget;
    Cal *m_activeCal;
    QList<QSharedPointer<CalendarItem>> m_items;
    QVBoxLayout *m_layout;
    QTableWidget *m_propertiesTable;
    QPushButton *m_applyButton;
    QSet<int> m_modifiedRows; // Track edited rows

    class WrappingDelegate : public QStyledItemDelegate {
    public:
        explicit WrappingDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            opt.textElideMode = Qt::ElideNone; // Disable eliding
            QStyledItemDelegate::paint(painter, opt, index);
        }
        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            opt.textElideMode = Qt::ElideNone;
            QString text = index.data(Qt::DisplayRole).toString();
            QFontMetrics fm(opt.font);
            // Use the actual column width for wrapping
            int columnWidth = static_cast<QTableWidget*>(parent())->columnWidth(index.column());
            QRect textRect = fm.boundingRect(QRect(0, 0, columnWidth - 10, 0), Qt::TextWordWrap, text);
            QSize size = QStyledItemDelegate::sizeHint(opt, index);
            size.setHeight(textRect.height() + 10); // Add padding
            size.setWidth(columnWidth);
            return size;
        }
    };
    WrappingDelegate *m_wrappingDelegate; // Add delegate member
};

#endif // EDITPANE_H
