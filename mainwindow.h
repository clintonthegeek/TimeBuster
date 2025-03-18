#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include "collectioncontroller.h"
#include "credentialsdialog.h"
#include "syncbackend.h"

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

private slots:
    void addLocalCollection();
    void addRemoteCollection();
    void addLocalBackend(); // Renamed from attachActiveToLocal()

    void syncCollections();
    void onCollectionAdded(Collection *collection);
    void onOpenCollection(); // New slot
    void onItemAdded(Cal *cal, QSharedPointer<CalendarItem> item);
    void onCalendarLoaded(Cal *cal);
    void onSaveCollection();
    void onAddLocalBackend();

private:
    void addCalendarView(Cal *cal);
    void onCalendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars);
    void onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items);
    void onSubWindowActivated(QMdiSubWindow *window);
    bool isCollectionTransient(const QString &collectionId) const;

    CredentialsDialog *credentialsDialog;
    Ui::MainWindow *ui;
    CollectionController *collectionController;
    Collection *activeCollection;
    QString activeCal;
};

#endif // MAINWINDOW_H
