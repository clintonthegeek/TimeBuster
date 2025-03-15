#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include "credentialsdialog.h"
#include "syncbackend.h"
#include "collection.h"
#include <QSharedPointer>

class CollectionController;
class Cal;

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
    void attachActiveToLocal(); // Renamed from createLocalFromRemote
    void syncCollections();
    void onCollectionAdded(Collection *collection);
    void onCalendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars); // Renamed
    void onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items); // Updated
    void onSubWindowActivated(QMdiSubWindow *window);

private:
    void addCalendarView(Cal *cal);
    CredentialsDialog *credentialsDialog;
    Ui::MainWindow *ui;
    CollectionController *collectionController;
    QString activeCal;
    Collection *activeCollection;
};

#endif // MAINWINDOW_H
