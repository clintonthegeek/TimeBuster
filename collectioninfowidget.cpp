#include "collectioninfowidget.h"
#include "collection.h"
#include "cal.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include <QVBoxLayout>

CollectionInfoWidget::CollectionInfoWidget(QWidget *parent)
    : QWidget(parent)
{
    treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderLabels({"Property", "Value"});
    treeWidget->setColumnWidth(0, 150);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(treeWidget);
    setLayout(layout);
}

void CollectionInfoWidget::setCollection(Collection *collection)
{
    treeWidget->clear();
    if (!collection) {
        qDebug() << "CollectionInfoWidget: No collection to display";
        return;
    }

    QTreeWidgetItem *root = new QTreeWidgetItem(treeWidget);
    root->setText(0, "Collection");
    root->setText(1, collection->name() + " (" + collection->id() + ")");

    QTreeWidgetItem *backendsItem = new QTreeWidgetItem(root);
    backendsItem->setText(0, "Backends");

    // Assume MainWindow or CollectionController passes backends later
    // For now, this is a placeholder - we'll integrate with actual backends in MainWindow
    QTreeWidgetItem *backendPlaceholder = new QTreeWidgetItem(backendsItem);
    backendPlaceholder->setText(0, "Unknown");
    backendPlaceholder->setText(1, "Pending integration");

    QTreeWidgetItem *calendarsItem = new QTreeWidgetItem(root);
    calendarsItem->setText(0, "Calendars");
    for (Cal *cal : collection->calendars()) {
        QTreeWidgetItem *calItem = new QTreeWidgetItem(calendarsItem);
        calItem->setText(0, cal->name());
        calItem->setText(1, cal->id());
    }

    treeWidget->expandAll();
}
