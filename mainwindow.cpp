#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calendarview.h"
#include "collectioncontroller.h"
#include "localbackend.h"
#include "caldavbackend.h"
#include "configmanager.h"
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QProgressBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), credentialsDialog(new CredentialsDialog(this)),
    collectionController(new CollectionController(this)), activeCollection(nullptr)
{
    ui->setupUi(this);

    // Status bar setup (adding as per previous discussion)
    QLabel *statusLabel = new QLabel("Ready", this);
    QProgressBar *totalProgressBar = new QProgressBar(this);
    QProgressBar *currentProgressBar = new QProgressBar(this);
    ui->statusBar->addWidget(statusLabel, 1);
    ui->statusBar->addPermanentWidget(totalProgressBar);
    ui->statusBar->addPermanentWidget(currentProgressBar);

    // Connect menu actions
    connect(ui->actionNewLocalCollection, &QAction::triggered, this, &MainWindow::addLocalCollection);
    connect(ui->actionNewRemoteCollection, &QAction::triggered, this, &MainWindow::addRemoteCollection);
    connect(ui->actionSyncCollections, &QAction::triggered, this, &MainWindow::syncCollections);
    connect(collectionController, &CollectionController::collectionAdded, this, &MainWindow::onCollectionAdded);
    connect(collectionController, &CollectionController::calendarAdded, this, &MainWindow::addCalendarView);
    connect(collectionController, &CollectionController::calendarsLoaded, this, &MainWindow::onCalendarsLoaded);
    connect(collectionController, &CollectionController::itemsLoaded, this, &MainWindow::onItemsLoaded);
    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);
    connect(ui->actionOpenCollection, &QAction::triggered, this, &MainWindow::onOpenCollection);

    // Setup Add Backend submenu
    QMenu *addBackendMenu = new QMenu(this);
    QAction *addLocalAction = addBackendMenu->addAction("🚧 Add &Local");
    QAction *addRemoteAction = addBackendMenu->addAction("Add &Remote");
    ui->actionAddBackend->setMenu(addBackendMenu);
    connect(addLocalAction, &QAction::triggered, this, &MainWindow::addLocalBackend);
    // addRemoteAction left unconnected for now (future 🚧)

    qDebug() << "MainWindow: Initialized";
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow: Destroying MainWindow";
    delete collectionController; // Ensure controller is deleted before UI
    delete credentialsDialog;
    delete ui;
}

void MainWindow::addCalendarView(Cal *cal)
{
    CalendarView *view = new CalendarView(cal, this);
    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(view);
    subWindow->setWindowTitle(cal->name()); // Add this
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->resize(400, 300);
    subWindow->show();
    qDebug() << "MainWindow: Added subwindow for" << cal->id() << "titled" << cal->name();
}

void MainWindow::addLocalCollection()
{
    qDebug() << "MainWindow: Adding local collection";
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Local Collection Directory"));
    if (dir.isEmpty()) {
        ui->logTextEdit->append("Local collection creation canceled");
        return;
    }
    Collection *col = new Collection("local0", "Local Collection", this); // ID will be set by controller
    collectionController->addCollection("Local Collection", new LocalBackend(dir, this));
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

void MainWindow::addLocalBackend()
{
    if (!activeCollection) {
        ui->logTextEdit->append("No active collection to attach a backend to");
        qDebug() << "MainWindow: No active collection for addLocalBackend";
        return;
    }

    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Local Backend Directory"));
    if (dir.isEmpty()) {
        ui->logTextEdit->append("Add Local Backend canceled");
        qDebug() << "MainWindow: Add Local Backend canceled by user";
        return;
    }

    LocalBackend *localBackend = new LocalBackend(dir, this);
    collectionController->attachLocalBackend(activeCollection->id(), localBackend);
    ui->logTextEdit->append("Attached local backend at " + dir + " to collection " + activeCollection->name());
    qDebug() << "MainWindow: Attached LocalBackend to" << activeCollection->id() << "at" << dir;
}

void MainWindow::syncCollections()
{
    ui->logTextEdit->append("Sync Collections triggered (stub)");
}

void MainWindow::onItemAdded(Cal *cal, QSharedPointer<CalendarItem> item)
{
    for (QMdiSubWindow *window : ui->mdiArea->subWindowList()) {
        if (CalendarView *view = qobject_cast<CalendarView*>(window->widget())) {
            if (view->model()->id() == cal->id()) {
                qDebug() << "MainWindow: Updating view for" << cal->id() << "with item" << item->id();
                view->refresh(); // Temporary—Stage 2 will refine this
                break;
            }
        }
    }
}

void MainWindow::onCalendarLoaded(Cal *cal)
{
    qDebug() << "MainWindow: Calendar loaded:" << cal->id();
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
    qDebug() << "MainWindow: onItemsLoaded for" << cal->id() << "with" << items.size() << "items";
    for (QMdiSubWindow *window : ui->mdiArea->subWindowList()) {
        if (CalendarView *view = qobject_cast<CalendarView*>(window->widget())) {
            if (view->model()->id() == cal->id()) {
                view->refresh();
                qDebug() << "MainWindow: Refreshed view for" << cal->name();
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
            // Don’t clear activeCal here—m_calMap should be reliable now
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
