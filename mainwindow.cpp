#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calendarview.h"
#include "collectioncontroller.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include "configmanager.h" // Add this include to access ConfigManager
#include <QDebug>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), credentialsDialog(new CredentialsDialog(this)),
    collectionController(new CollectionController(this)), activeCollection(nullptr)
{
    ui->setupUi(this);

    connect(ui->actionAddLocalCollection, &QAction::triggered, this, &MainWindow::addLocalCollection);
    connect(ui->actionAddRemoteCollection, &QAction::triggered, this, &MainWindow::addRemoteCollection);
    connect(ui->actionCreateLocalFromRemote, &QAction::triggered, this, &MainWindow::attachActiveToLocal);
    connect(ui->actionSyncCollections, &QAction::triggered, this, &MainWindow::syncCollections);
    connect(collectionController, &CollectionController::collectionAdded, this, &MainWindow::onCollectionAdded);
    connect(collectionController, &CollectionController::calendarsLoaded, this, &MainWindow::onCalendarsLoaded);
    connect(collectionController, &CollectionController::itemsLoaded, this, &MainWindow::onItemsLoaded);
    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);

    // Add Open Collection action
    connect(ui->actionOpenCollection, &QAction::triggered, this, &MainWindow::onOpenCollection);

    qDebug() << "MainWindow: Initialized";
}

MainWindow::~MainWindow()
{
    delete ui;
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
    activeCal = cal->id();
    qDebug() << "MainWindow: Set activeCal to" << activeCal;
    if (collectionController->getCal(activeCal)) {
        qDebug() << "MainWindow: Focused on" << activeCal;
    }
}

void MainWindow::addLocalCollection()
{
    qDebug() << "MainWindow: Adding local collection";
    Collection *col = new Collection("local0", "Local Collection", this);
    collectionController->addCollection("Local Collection", new LocalBackend("local_storage", this));
    qDebug() << "MainWindow: Local collection added";
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
        collectionController->addCollection("Remote Collection", backend);
        ui->logTextEdit->append("Added remote collection - awaiting fetch");
    } else {
        ui->logTextEdit->append("Remote collection creation canceled");
    }
}

void MainWindow::attachActiveToLocal()
{
    if (!activeCollection) {
        ui->logTextEdit->append("No active collection to attach");
        qDebug() << "MainWindow: No active collection for attachActiveToLocal";
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Create Local From Remote"),
        QDir::currentPath(),
        tr("KALB Files (*.kalb)")
        );

    if (filePath.isEmpty()) {
        ui->logTextEdit->append("Create Local From Remote canceled");
        qDebug() << "MainWindow: Create Local From Remote canceled by user";
        return;
    }

    if (!filePath.endsWith(".kalb")) {
        filePath += ".kalb";
    }

    QFileInfo fileInfo(filePath);
    QString rootPath = fileInfo.absolutePath();

    // Create a new LocalBackend
    LocalBackend *localBackend = new LocalBackend(rootPath, this);
    collectionController->attachLocalBackend(activeCollection->id(), localBackend);

    // Get the list of backends for the collection from CollectionController
    const auto backends = collectionController->backends().value(activeCollection->id());
    if (backends.isEmpty()) {
        ui->logTextEdit->append("No backends found for collection");
        qDebug() << "MainWindow: No backends found for collection" << activeCollection->id();
        return;
    }

    // Use ConfigManager to save the collection configuration with all backends
    ConfigManager configManager(this);
    if (configManager.saveBackendConfig(activeCollection->id(), activeCollection->name(), backends, filePath)) {
        ui->logTextEdit->append("Created Local From Remote and saved to " + filePath + " at " + rootPath);
        qDebug() << "MainWindow: Saved collection" << activeCollection->id() << "with backends to" << filePath;
    } else {
        ui->logTextEdit->append("Failed to save collection to " + filePath);
        qDebug() << "MainWindow: Failed to save collection" << activeCollection->id() << "to" << filePath;
    }
}


void MainWindow::syncCollections()
{
    ui->logTextEdit->append("Sync Collections triggered (stub)");
}

void MainWindow::onCollectionAdded(Collection *collection)
{
    qDebug() << "MainWindow: onCollectionAdded for" << collection->id();
    activeCollection = collection;
    const auto backends = collectionController->backends();
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
    }
    activeCal = collection->calendars().isEmpty() ? "" : collection->calendars().first()->id();
    ui->logTextEdit->append("Collection fetched with " + QString::number(collection->calendars().size()) + " calendars");
}

void MainWindow::onCalendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars)
{
    Q_UNUSED(calendars);
    if (activeCollection && activeCollection->id() == collectionId) {
        for (Cal *cal : activeCollection->calendars()) {
            if (cal->id() == "cal1") continue;
            addCalendarView(cal);
        }
    }
}

void MainWindow::onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items)
{
    Q_UNUSED(items);
    for (QMdiSubWindow *window : ui->mdiArea->subWindowList()) {
        if (CalendarView *view = qobject_cast<CalendarView*>(window->widget())) {
            if (view->model()->id() == cal->id()) {
                view->refresh();
                break;
            }
        }
    }
}

void MainWindow::onSubWindowActivated(QMdiSubWindow *window)
{
    if (!window) {
        activeCal = "";
        qDebug() << "MainWindow: No active subwindow, cleared activeCal";
        return;
    }
    if (CalendarView *view = qobject_cast<CalendarView*>(window->widget())) {
        activeCal = view->model()->id();
        qDebug() << "MainWindow: Switched activeCal to" << activeCal;
        if (collectionController->getCal(activeCal)) {
            qDebug() << "MainWindow: Focused on" << activeCal;
        } else {
            qDebug() << "MainWindow: ActiveCal" << activeCal << "not found in m_calMap";
            activeCal = "";
        }
    }
}

void MainWindow::onOpenCollection()
{
    qDebug() << "MainWindow: Opening collection";
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Open Collection"),
        QDir::currentPath(),
        tr("KALB Files (*.kalb)")
        );

    if (filePath.isEmpty()) {
        ui->logTextEdit->append("Open collection canceled");
        qDebug() << "MainWindow: Open collection canceled by user";
        return;
    }

    if (collectionController->loadCollection(filePath)) {
        ui->logTextEdit->append("Collection loaded from " + filePath);
        qDebug() << "MainWindow: Collection loaded successfully from" << filePath;
    } else {
        ui->logTextEdit->append("Failed to load collection from " + filePath);
        qDebug() << "MainWindow: Failed to load collection from" << filePath;
    }
}
