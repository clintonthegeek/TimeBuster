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
    // Ensure single connection
    if (!connect(collectionManager, &CollectionManager::collectionAdded, this, &MainWindow::onCollectionAdded)) {
        qDebug() << "MainWindow: Failed to connect collectionAdded signal";
    }
    qDebug() << "MainWindow: Initialized";
    // In constructor, add log feedback
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
    subWindow->setWindowTitle(cal->name()); //should be a pretty "display name"
    ui->mdiArea->addSubWindow(subWindow);
    subWindow->resize(400, 300);
    subWindow->show();
}

void MainWindow::addLocalCollection()
{
    qDebug() << "MainWindow: Adding local collection";
    Collection *col = new Collection("local0", "Local Collection", this);
    Cal *cal = new Cal("cal1", "Test Calendar", col);
    col->addCal(cal); // Assuming Collection has addCal, not addCalendar
    collectionManager->addCollection("local0", nullptr);
    qDebug() << "MainWindow: Local collection added with cal1";
}

// Update addRemoteCollection
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
    const auto backends = collectionManager->backends();
    if (!backends.contains(collection->id())) {
        qDebug() << "MainWindow: No backends for collection" << collection->id();
        return;
    }

    QSet<QString> fetchedCalendars;
    for (Cal *cal : collection->calendars()) {
        qDebug() << "MainWindow: Processing calendar" << cal->id() << cal->name();
        if (fetchedCalendars.contains(cal->id())) {
            qDebug() << "MainWindow: Skipping duplicate calendar" << cal->id();
            continue;
        }
        fetchedCalendars.insert(cal->id());
        addCalendarView(cal);
        for (SyncBackend *backend : backends[collection->id()]) {
            if (CalDAVBackend *caldav = qobject_cast<CalDAVBackend*>(backend)) {
                qDebug() << "MainWindow: Triggering fetchItems for" << cal->id();
                caldav->fetchItems(cal->id());
                break;
            }
        }
    }
    ui->logTextEdit->append("Collection fetched with " + QString::number(collection->calendars().size()) + " calendars");
}
