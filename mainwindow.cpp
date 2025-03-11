#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calendarview.h"
#include "collectionmanager.h" // Added
#include "localbackend.h"
#include "caldavbackend.h"


// Update constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), credentialsDialog(new CredentialsDialog(this))
{
    ui->setupUi(this);
    collectionManager = new CollectionManager(this);

    connect(ui->actionAddLocalCollection, &QAction::triggered, this, &MainWindow::addLocalCollection);
    connect(ui->actionAddRemoteCollection, &QAction::triggered, this, &MainWindow::addRemoteCollection);
    connect(ui->actionCreateLocalFromRemote, &QAction::triggered, this, &MainWindow::createLocalFromRemote);
    connect(ui->actionSyncCollections, &QAction::triggered, this, &MainWindow::syncCollections);
    connect(collectionManager, &CollectionManager::collectionAdded, this, &MainWindow::onCollectionAdded);
}

MainWindow::~MainWindow()
{
    delete ui; // Deletes collectionManager too, as it's a child
}

void MainWindow::addCalendarView(Cal *cal)
{
    CalendarView *view = new CalendarView(cal, this);
    QMdiSubWindow *subWindow = new QMdiSubWindow(this);
    subWindow->setWidget(view);
    subWindow->setWindowTitle(cal->displayName());
    ui->mdiArea->addSubWindow(subWindow);
    subWindow->resize(400, 300);
    subWindow->show();
}

// Update addLocalCollection
void MainWindow::addLocalCollection()
{
    LocalBackend *backend = new LocalBackend("", this); // Empty path - stubbed
    collectionManager->addCollection("Local Collection", backend);
    ui->logTextEdit->append("Added local collection with LocalBackend");
}

// Update addRemoteCollection
void MainWindow::addRemoteCollection()
{
    if (credentialsDialog->exec() == QDialog::Accepted) {
        CalDAVBackend *backend = new CalDAVBackend(
            credentialsDialog->serverUrl(),
            credentialsDialog->username(),
            credentialsDialog->password(),
            this
            );
        collectionManager->addCollection("Remote Collection", backend);
        ui->logTextEdit->append("Added remote collection with CalDAVBackend");
    } else {
        ui->logTextEdit->append("Remote collection creation canceled");
    }
}

void MainWindow::createLocalFromRemote()
{
    ui->logTextEdit->append("Create Local from Remote triggered (stub)");
}

void MainWindow::syncCollections()
{
    ui->logTextEdit->append("Sync Collections triggered (stub)");
}

void MainWindow::onCollectionAdded(Collection *collection)
{
    for (Cal *cal : collection->calendars()) {
        addCalendarView(cal);
    }
    ui->logTextEdit->append("Collection added with " + QString::number(collection->calendars().size()) + " calendars");
}
