#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include "credentialsdialog.h"
#include "syncbackend.h"
#include "sessionmanager.h" // For SessionManager

class CollectionManager;
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
    void onCollectionAdded(Collection *collection);
    void attachToLocal();
    void openLocal();
    void onEditItem();
    void onChangesApplied();

private:
    void addCalendarView(Cal *cal);
    void updateCollectionInfo();
    void initializeSessionManager();
    CredentialsDialog *credentialsDialog;

    Ui::MainWindow *ui;
    CollectionManager *collectionManager;

    CollectionInfoWidget *infoWidget;
    QDockWidget *infoDock;

    Collection *activeCollection;
    Cal *activeCal; // New member for tracking active calendar
    SessionManager *sessionManager; // Persistent SessionManager
};

#endif // MAINWINDOW_H
