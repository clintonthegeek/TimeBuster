#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include "credentialsdialog.h"
#include "syncbackend.h"
#include "collection.h"

class CollectionManager;
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
    void createLocalFromRemote();
    void syncCollections();
    void onCollectionAdded(Collection *collection);
    void onCalendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars); // Added
    void onItemsFetched(Cal *cal, QList<CalendarItem*> items); // Added

private:
    void addCalendarView(Cal *cal);
    CredentialsDialog *credentialsDialog;
    Ui::MainWindow *ui;
    CollectionManager *collectionManager;
    QString activeCal;
    Collection *activeCollection;
};

#endif // MAINWINDOW_H
