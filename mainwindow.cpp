#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calendarview.h"
#include "collectionmanager.h" // Added
#include "localbackend.h"
#include "caldavbackend.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    collectionManager = new CollectionManager(this); // Initialize it

    // Connect menu actions to slots
    connect(ui->actionAddLocalCollection, &QAction::triggered, this, &MainWindow::addLocalCollection);
    connect(ui->actionAddRemoteCollection, &QAction::triggered, this, &MainWindow::addRemoteCollection);
    connect(ui->actionCreateLocalFromRemote, &QAction::triggered, this, &MainWindow::createLocalFromRemote);
    connect(ui->actionSyncCollections, &QAction::triggered, this, &MainWindow::syncCollections);

    // Connect CollectionManager signal
    connect(collectionManager, &CollectionManager::collectionAdded, this, &MainWindow::onCollectionAdded);

    // Remove your test code - handled by addLocalCollection now
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

void MainWindow::addRemoteCollection()
{
    CalDAVBackend *backend = new CalDAVBackend("http://fake.cal.dav", "user", "pass", this); // Fake creds
    collectionManager->addCollection("Remote Collection", backend);
    ui->logTextEdit->append("Added remote collection with CalDAVBackend");
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
