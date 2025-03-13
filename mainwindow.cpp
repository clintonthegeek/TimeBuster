#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calendarview.h"
#include "collectionmanager.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), credentialsDialog(new CredentialsDialog(this)),
    collectionManager(new CollectionManager(this)), activeCollection(nullptr)
{
    ui->setupUi(this);

    connect(ui->actionAddLocalCollection, &QAction::triggered, this, &MainWindow::addLocalCollection);
    connect(ui->actionAddRemoteCollection, &QAction::triggered, this, &MainWindow::addRemoteCollection);
    connect(ui->actionCreateLocalFromRemote, &QAction::triggered, this, &MainWindow::createLocalFromRemote);
    connect(ui->actionSyncCollections, &QAction::triggered, this, &MainWindow::syncCollections);
    connect(collectionManager, &CollectionManager::collectionAdded, this, &MainWindow::onCollectionAdded);

    qDebug() << "MainWindow: Initialized";
}

MainWindow::~MainWindow()
{
    delete ui; // Deletes collectionManager and credentialsDialog as children
}

void MainWindow::addCalendarView(Cal *cal)
{
    CalendarView *view = new CalendarView(cal, this);
    QMdiSubWindow *subWindow = new QMdiSubWindow(this);
    subWindow->setWidget(view);
    subWindow->setWindowTitle(cal->name());
    ui->mdiArea->addSubWindow(subWindow);
    subWindow->resize(400, 300);
    subWindow->show();
    activeCal = cal->id(); // Store ID instead of pointer
    qDebug() << "MainWindow: Set activeCal to" << activeCal;
}

void MainWindow::addLocalCollection()
{
    qDebug() << "MainWindow: Adding local collection";
    Collection *col = new Collection("local0", "Local Collection", this);
    Cal *cal = new Cal("cal1", "Test Calendar", col);
    col->addCal(cal);
    collectionManager->addCollection("local0", nullptr);
    qDebug() << "MainWindow: Local collection added with cal1";
    activeCollection = col;
}

void MainWindow::addRemoteCollection()
{
    if (credentialsDialog->exec() == QDialog::Accepted) {
        ui->logTextEdit->append("Fetching remote calendars...");
        CalDAVBackend *backend = new CalDAVBackend(
            credentialsDialog->serverUrl(),
            credentialsDialog->username(),
            credentialsDialog->password(),
            this
            );
        collectionManager->addCollection("Remote Collection", backend);
        ui->logTextEdit->append("Added remote collection - awaiting fetch");
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
    qDebug() << "MainWindow: onCollectionAdded for" << collection->id();
    activeCollection = collection;
    const auto backends = collectionManager->backends();
    if (!backends.contains(collection->id())) {
        qDebug() << "MainWindow: No backends for collection" << collection->id();
        return;
    }

    QSet<QString> fetchedCalendars;
    for (Cal *cal : collection->calendars()) {
        qDebug() << "MainWindow: Processing calendar" << cal->id() << cal->name();
        if (fetchedCalendars.contains(cal->id())) continue;
        fetchedCalendars.insert(cal->id());
        addCalendarView(cal);
        // Remove fetchItems call - handled by CollectionManager now
    }
    ui->logTextEdit->append("Collection fetched with " + QString::number(collection->calendars().size()) + " calendars");
}
