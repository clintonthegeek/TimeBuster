#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include "credentialsdialog.h"
#include "syncbackend.h"

class CollectionManager; // Already there
class Cal;
class Collection;
class CollectionInfoWidget;

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
    void onCollectionAdded(Collection *collection); // New slot
    void attachToLocal();
    void openLocal();

private:
    void addCalendarView(Cal *cal);
    void updateCollectionInfo(); // Added
    CredentialsDialog *credentialsDialog; // New - owned

    Ui::MainWindow *ui;
    CollectionManager *collectionManager; // Now fully used

    CollectionInfoWidget *infoWidget; // New
    QDockWidget *infoDock; // New - keep the pointer
    Collection *activeCollection; // New - track single active collection
};

#endif // MAINWINDOW_H
