#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calendarview.h"
#include "collectionmanager.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include "credentialsdialog.h"
#include "collectioninfowidget.h"
#include "QFileDialog"
#include "QFileInfo"
#include "QDockWidget"
#include "QWidget"
#include "QInputDialog"
#include "sessionmanager.h"
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), credentialsDialog(new CredentialsDialog(this)),
    collectionManager(new CollectionManager(this)), infoWidget(new CollectionInfoWidget(this)),
    infoDock(new QDockWidget("Collection Info", this)), activeCollection(nullptr), activeCal(nullptr), sessionManager(nullptr)
{
    ui->setupUi(this);

    connect(ui->actionAddLocalCollection, &QAction::triggered, this, &MainWindow::addLocalCollection);
    connect(ui->actionAddRemoteCollection, &QAction::triggered, this, &MainWindow::addRemoteCollection);
    connect(ui->actionCreateLocalFromRemote, &QAction::triggered, this, &MainWindow::createLocalFromRemote);
    connect(ui->actionSyncCollections, &QAction::triggered, this, &MainWindow::syncCollections);
    connect(ui->actionAttachToLocal, &QAction::triggered, this, &MainWindow::attachToLocal);
    connect(ui->actionOpenLocal, &QAction::triggered, this, &MainWindow::openLocal);
    connect(ui->actionEditItem, &QAction::triggered, this, &MainWindow::onEditItem);
    connect(ui->actionSaveChanges, &QAction::triggered, this, &MainWindow::onSaveChanges);

    connect(collectionManager, &CollectionManager::collectionAdded, this, &MainWindow::onCollectionAdded);

    // Connect QMdiArea to track active subwindow changes
    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, [this](QMdiSubWindow *subWindow) {
        if (subWindow) {
            CalendarView *view = qobject_cast<CalendarView*>(subWindow->widget());
            if (view) {
                activeCal = view->model();
                qDebug() << "MainWindow: Set activeCal to" << activeCal->name();
            }
        } else {
            activeCal = nullptr;
            qDebug() << "MainWindow: Cleared activeCal";
        }
    });

    infoDock->setWidget(infoWidget);
    addDockWidget(Qt::LeftDockWidgetArea, infoDock);
    qDebug() << "MainWindow: Initialized";
}

MainWindow::~MainWindow()
{
    // Clear active pointers to prevent accessing deleted objects
    activeCal = nullptr;
    activeCollection = nullptr;
    if (sessionManager) {
        sessionManager->saveCache(); // Ensure changes are saved
        delete sessionManager;
    }
    delete ui; // This will delete collectionManager and other children
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
}

void MainWindow::onSaveChanges()
{
    if (!activeCollection || !sessionManager) {
        ui->logTextEdit->append("No active collection or session manager!");
        return;
    }

    for (SyncBackend *backend : collectionManager->backends()[activeCollection->id()]) {
        if (LocalBackend *local = qobject_cast<LocalBackend*>(backend)) {
            sessionManager->commitChanges(local);
            ui->logTextEdit->append("Changes committed to local storage.");
            break;
        }
    }
}

void MainWindow::initializeSessionManager()
{
    if (activeCollection && !activeCollection->id().isEmpty()) {
        qDebug() << "MainWindow: Initializing SessionManager with" << activeCollection->id();
        if (sessionManager) {
            delete sessionManager;
        }
        sessionManager = new SessionManager(activeCollection->id(), this);
        sessionManager->setCollectionManager(collectionManager); // Pass the correct CollectionManager
        sessionManager->loadCache();
        connect(sessionManager, &SessionManager::changesApplied, this, &MainWindow::onChangesApplied);
        for (SyncBackend *backend : collectionManager->backends()[activeCollection->id()]) {
            sessionManager->applyToBackend(backend);
        }
    } else {
        qDebug() << "MainWindow: No valid collection ID to initialize SessionManager";
    }
}

void MainWindow::onChangesApplied()
{
    // Refresh all CalendarView instances
    for (QMdiSubWindow *subWindow : ui->mdiArea->subWindowList()) {
        CalendarView *view = qobject_cast<CalendarView*>(subWindow->widget());
        if (view) {
            view->refresh(); // Use public method
            qDebug() << "MainWindow: Refreshed CalendarView for" << view->model()->name();
        }
    }
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

    localBackend->storeCalendars(activeCollection->id(), activeCollection->calendars());

    for (Cal *cal : activeCollection->calendars()) {
        localBackend->storeItems(cal, cal->items());
    }
    ui->logTextEdit->append("Attached local backend to " + activeCollection->name() + " at " + kalbPath);
    updateCollectionInfo();
    activeCollection->setId(kalbPath); // Update to the .kalb path
    initializeSessionManager(); // Reinitialize with new path
}

void MainWindow::updateCollectionInfo()
{
    if (!activeCollection) {
        infoWidget->setCollection(nullptr);
        return;
    }

    infoWidget->setCollection(activeCollection);
    QTreeWidget *tree = infoWidget->findChild<QTreeWidget*>();
    QTreeWidgetItem *backendsItem = tree->topLevelItem(0)->child(0);
    backendsItem->takeChildren();

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
            QList<CalendarMetadata> calendars = local->fetchCalendars(collectionId);
            for (const CalendarMetadata &meta : calendars) {
                Cal *cal = new Cal(meta.id, meta.name, col);
                QList<CalendarItem*> items = local->fetchItems(cal);
                for (CalendarItem *item : items) {
                    cal->addItem(item);
                }
                if (!items.isEmpty()) {
                    col->addCal(cal);
                } else {
                    qDebug() << "MainWindow: No items loaded for calendar" << meta.name;
                    delete cal;
                }
            }
        }
    }

    if (col->calendars().isEmpty()) {
        ui->logTextEdit->append("No local calendars or items found in " + kalbPath);
    } else {
        collectionManager->collectionAdded(col);
        ui->logTextEdit->append("Opened " + kalbPath + " with " + QString::number(col->calendars().size()) + " calendars");
        activeCollection = col;
        activeCollection->setId(kalbPath); // Update to the .kalb path
        updateCollectionInfo();
        initializeSessionManager(); // Initialize with new path
    }
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
    activeCollection->setId(QDir::currentPath() + "/local0.kalb"); // Set default .kalb path
    initializeSessionManager(); // Initialize with new path
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
    initializeSessionManager(); // Initialize with collection ID
}

void MainWindow::onEditItem()
{
    if (!activeCollection) {
        ui->logTextEdit->append("No active collection to edit!");
        return;
    }

    if (!activeCal) {
        ui->logTextEdit->append("No active calendar selected!");
        return;
    }

    // Find the CalendarView for the activeCal
    CalendarView *view = nullptr;
    for (QMdiSubWindow *subWindow : ui->mdiArea->subWindowList()) {
        CalendarView *candidate = qobject_cast<CalendarView*>(subWindow->widget());
        if (candidate && candidate->model() == activeCal) {
            view = candidate;
            break;
        }
    }

    if (!view) {
        ui->logTextEdit->append("No calendar view found for active calendar!");
        return;
    }

    // Get the current selection
    QModelIndex current = view->currentIndex();
    if (!current.isValid()) {
        ui->logTextEdit->append("No item selected for editing!");
        return;
    }

    int row = current.row();
    bool ok;
    QString newSummary = QInputDialog::getText(this, "Edit Item", "Enter new summary:",
                                               QLineEdit::Normal, activeCal->items().at(row)->data(Qt::DisplayRole).toString(), &ok);
    if (!ok || newSummary.isEmpty()) {
        ui->logTextEdit->append("Edit canceled or empty summary!");
        return;
    }

    if (activeCal->updateItem(row, newSummary)) {
        // Stage the change using SessionManager
        if (sessionManager) {
            Change change;
            change.calId = activeCal->id();
            change.itemId = activeCal->items().at(row)->id();
            change.operation = "UPDATE";
            KCalendarCore::ICalFormat format;
            KCalendarCore::MemoryCalendar::Ptr tempCalendar(new KCalendarCore::MemoryCalendar(QTimeZone("UTC")));
            tempCalendar->addIncidence(activeCal->items().at(row)->incidence());
            change.data = format.toString(tempCalendar);
            change.timestamp = QDateTime::currentDateTime();
            sessionManager->stageChange(change);
            qDebug() << "MainWindow: Staged change for item" << change.itemId;
        } else {
            qDebug() << "MainWindow: No SessionManager available to stage change";
        }
    } else {
        ui->logTextEdit->append("Failed to update item at row " + QString::number(row));
    }
}
