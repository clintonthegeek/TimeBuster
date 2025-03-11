#ifndef COLLECTIONINFOWIDGET_H
#define COLLECTIONINFOWIDGET_H

#include <QWidget>
#include <QTreeWidget>

class Collection;

class CollectionInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CollectionInfoWidget(QWidget *parent = nullptr);
    void setCollection(Collection *collection);

private:
    QTreeWidget *treeWidget;
};

#endif // COLLECTIONINFOWIDGET_H
