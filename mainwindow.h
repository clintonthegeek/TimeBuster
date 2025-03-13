#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include "credentialsdialog.h"
#include "syncbackend.h"
#include "collection.h" // Added to define Collection fully

class CollectionManager;
class Cal;
// class Collection; // No longer needed as a forward declaration

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

private:
    void addCalendarView(Cal *cal);
    CredentialsDialog *credentialsDialog; // Owned
    Ui::MainWindow *ui;
    CollectionManager *collectionManager; // Owned
    QString activeCal; // Now a calId instead of Cal*
    Collection *activeCollection; // Still a pointer for now
};

#endif // MAINWINDOW_H
