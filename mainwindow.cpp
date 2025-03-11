#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calendarview.h"
#include "collectionmanager.h" // Added
#include "localbackend.h"
#include "caldavbackend.h"
#include "credentialsdialog.h"
#include "collectioninfowidget.h"
#include "QFileDialog"
#include "QFileInfo"
#include "QDockWidget"
#include "QWidget"


// Update constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), credentialsDialog(new CredentialsDialog(this)),
    collectionManager(new CollectionManager(this)), infoWidget(new CollectionInfoWidget(this)),
    infoDock(new QDockWidget("Collection Info", this)), activeCollection(nullptr)
{

    ui->setupUi(this);
    collectionManager = new CollectionManager(this);

    connect(ui->actionAddLocalCollection, &QAction::triggered, this, &MainWindow::addLocalCollection);
    connect(ui->actionAddRemoteCollection, &QAction::triggered, this, &MainWindow::addRemoteCollection);
    connect(ui->actionCreateLocalFromRemote, &QAction::triggered, this, &MainWindow::createLocalFromRemote);
    connect(ui->actionSyncCollections, &QAction::triggered, this, &MainWindow::syncCollections);
    connect(ui->actionAttachToLocal, &QAction::triggered, this, &MainWindow::attachToLocal); // New
    connect(ui->actionOpenLocal, &QAction::triggered, this, &MainWindow::openLocal); // New

    connect(collectionManager, &CollectionManager::collectionAdded, this, &MainWindow::onCollectionAdded);

    infoDock->setWidget(infoWidget); // Set before adding
    addDockWidget(Qt::LeftDockWidgetArea, infoDock); // Add to layout
    qDebug() << "MainWindow: Initialized";

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

void MainWindow::attachToLocal()
{
    if (!activeCollection) {
        ui->logTextEdit->append("No remote collection to attach to!");
        return;
    }

    QString kalbPath = QFileDialog::getSaveFileName(this, "Save .kalb File", "", "*.kalb");
    if (kalbPath.isEmpty()) return;

    QString rootPath = QFileInfo(kalbPath).absolutePath();
    LocalBackend *localBackend = new LocalBackend(rootPath, this);
    collectionManager->addBackend(activeCollection->id(), localBackend);
    collectionManager->saveBackendConfig(activeCollection->id(), kalbPath);

    for (Cal *cal : activeCollection->calendars()) {
        localBackend->storeItems(cal, cal->items());
    }
    ui->logTextEdit->append("Attached local backend to " + activeCollection->name() + " at " + kalbPath);
    updateCollectionInfo();
}

void MainWindow::updateCollectionInfo()
{
    if (!activeCollection) {
        infoWidget->setCollection(nullptr);
        return;
    }

    infoWidget->setCollection(activeCollection);
    QTreeWidget *tree = infoWidget->findChild<QTreeWidget*>();
    QTreeWidgetItem *backendsItem = tree->topLevelItem(0)->child(0); // "Backends"
    backendsItem->takeChildren(); // Clear placeholder

    const auto backends = collectionManager->backends()[activeCollection->id()];
    for (SyncBackend *backend : backends) {
        QTreeWidgetItem *backendItem = new QTreeWidgetItem(backendsItem);
        if (LocalBackend *local = qobject_cast<LocalBackend*>(backend)) {
            backendItem->setText(0, "Local");
            backendItem->setText(1, local->rootPath());
        } else if (CalDAVBackend *caldav = qobject_cast<CalDAVBackend*>(backend)) {
            backendItem->setText(0, "CalDAV");
            backendItem->setText(1, caldav->serverUrl());
        }
    }
}

void MainWindow::openLocal()
{
    QString kalbPath = QFileDialog::getOpenFileName(this, "Open .kalb File", "", "*.kalb");
    if (kalbPath.isEmpty()) return;

    QString collectionId = QFileInfo(kalbPath).baseName();
    QList<SyncBackend*> backends = collectionManager->loadBackendConfig(collectionId, kalbPath);
    if (backends.isEmpty()) {
        ui->logTextEdit->append("No valid backends found in " + kalbPath);
        return;
    }

    collectionManager->setConfigBasePath(QFileInfo(kalbPath).absolutePath());
    Collection *col = new Collection(collectionId, "Loaded Collection", this);
    collectionManager->addCollection("Loaded Collection");
    collectionManager->setBackends(collectionId, backends);

    for (SyncBackend *backend : backends) {
        if (LocalBackend *local = qobject_cast<LocalBackend*>(backend)) {
            for (Cal *cal : activeCollection ? activeCollection->calendars() : QList<Cal*>()) {
                QList<CalendarItem*> items = local->fetchItems(cal);
                Cal *localCal = new Cal(cal->id(), cal->name(), col);
                for (CalendarItem *item : items) {
                    localCal->addItem(item);
                }
                col->addCal(localCal);
            }
        }
    }

    collectionManager->collectionAdded(col);
    ui->logTextEdit->append("Opened " + kalbPath);
    activeCollection = col;
    updateCollectionInfo();
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
    if (!activeCollection) {
        ui->logTextEdit->append("No collection to sync!");
        return;
    }

    const QString &collectionId = activeCollection->id();
    QList<SyncBackend*> backends = collectionManager->backends()[collectionId];
    for (SyncBackend *backend : backends) {
        if (CalDAVBackend *caldav = qobject_cast<CalDAVBackend*>(backend)) {
            qDebug() << "MainWindow: Triggering sync for CalDAV backend in" << collectionId;
            connect(caldav, &CalDAVBackend::calendarsFetched, collectionManager, &CollectionManager::onCalendarsFetched);
            connect(caldav, &CalDAVBackend::itemsFetched, collectionManager, &CollectionManager::onItemsFetched);
            caldav->fetchCalendars(collectionId);
            ui->logTextEdit->append("Syncing remote data for " + activeCollection->name());
            return;
        }
    }
    ui->logTextEdit->append("No remote backend to sync for " + activeCollection->name());
}

void MainWindow::onCollectionAdded(Collection *collection)
{
    qDebug() << "MainWindow: onCollectionAdded for" << collection->id();
    activeCollection = collection;
    updateCollectionInfo();

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
        for (SyncBackend *backend : backends[collection->id()]) {
            if (CalDAVBackend *caldav = qobject_cast<CalDAVBackend*>(backend)) {
                qDebug() << "MainWindow: Triggering fetchItems for" << cal->id();
                caldav->fetchItems(cal);
                break;
            }
        }
    }
    ui->logTextEdit->append("Collection fetched with " + QString::number(collection->calendars().size()) + " calendars");
}
