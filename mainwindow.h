#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include <QStandardItemModel>
#include <QTreeView>
#include <QCloseEvent>
#include "collectioncontroller.h"
#include "credentialsdialog.h"
#include "syncbackend.h"
#include "sessionmanager.h"
#include "editpane.h"


class CollectionManager; // Already there
class Cal;
class Collection;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void addLocalCollection();
    void addRemoteCollection();
    void addLocalBackend(); // Renamed from attachActiveToLocal()

    void syncCollections();
    void onCollectionAdded(Collection *collection);
    void onOpenCollection(); // New slot
    void onCloseCollection(); // New slot to close the collection
    void onItemAdded(Cal *cal, QSharedPointer<CalendarItem> item);
    void onCalendarLoaded(Cal *cal);
    void onSaveCollection();
    void onAddLocalBackend();

    void onSelectionChanged(); // New slot
    void onCommitChanges(); // New slot

    void onCalendarAdded(Cal *cal); // New slot
    void onAllSyncsCompleted(const QString &collectionId); // New slot

    void onTreeClicked(const QModelIndex &index);

private:
    void addCalendarView(Cal *cal);
    void onItemSelected(const QList<QSharedPointer<CalendarItem>>& items); // New slot
    void onCalendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items);
    void onSubWindowActivated(QMdiSubWindow *window);
    bool isCollectionTransient(const QString &collectionId) const;

    EditPane* editPane; // New member
    QStandardItemModel *collectionModel; // Tree model for collectionTree
    QMap<QString, QMdiSubWindow*> calToSubWindow; // Map calendar IDs to subwindows
    CredentialsDialog *credentialsDialog;
    Ui::MainWindow *ui;
    CollectionController *collectionController;
    SessionManager *sessionManager; // New member
    Collection *activeCollection;
    QString activeCal;
    QSharedPointer<CalendarItem> currentItem; // New: Track the selected item
};

#endif // MAINWINDOW_H
