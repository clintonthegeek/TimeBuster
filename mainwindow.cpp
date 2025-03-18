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
#include <QMessageBox>

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
    connect(ui->actionSaveCollection, &QAction::triggered, this, &MainWindow::onSaveCollection);
    connect(collectionController, &CollectionController::collectionAdded, this, &MainWindow::onCollectionAdded);
    connect(collectionController, &CollectionController::calendarAdded, this, &MainWindow::addCalendarView);
    connect(collectionController, &CollectionController::calendarsLoaded, this, &MainWindow::onCalendarsLoaded);
    connect(collectionController, &CollectionController::itemsLoaded, this, &MainWindow::onItemsLoaded);
    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);
    connect(ui->actionOpenCollection, &QAction::triggered, this, &MainWindow::onOpenCollection);
    connect(ui->actionAddLocalBackend, &QAction::triggered, this, &MainWindow::onAddLocalBackend);



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
    collectionController->loadCollection("Local Collection", new LocalBackend(dir, this));
    qDebug() << "MainWindow: Local collection loaded";
}

void MainWindow::addRemoteCollection()
{
    if (credentialsDialog->exec() == QDialog::Accepted) {
        ui->logTextEdit->append("Creating transient remote collection...");
        CalDAVBackend *backend = new CalDAVBackend(
            credentialsDialog->serverUrl(),
            credentialsDialog->username(),
            credentialsDialog->password(),
            this
            );
        collectionController->loadCollection("Remote Collection", backend, true);
        ui->logTextEdit->append("Transient remote collection created");
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

bool MainWindow::isCollectionTransient(const QString &collectionId) const
{
    return collectionController->isTransient(collectionId);
}

void MainWindow::onAddLocalBackend()
{
    if (!activeCollection) {
        ui->logTextEdit->append("No active collection to add backend to");
        return;
    }

    QString collectionId = activeCollection->id();
    if (isCollectionTransient(collectionId)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Save Collection"),
            tr("This collection is transient and must be saved before adding a local backend. Save now?"),
            QMessageBox::Yes | QMessageBox::No
            );
        if (reply == QMessageBox::Yes) {
            QString filePath = QFileDialog::getSaveFileName(this, tr("Save Collection"), QDir::currentPath(), tr("KALB Files (*.kalb)"));
            if (filePath.isEmpty()) {
                ui->logTextEdit->append("Save canceled, local backend not added");
                return;
            }
            if (!collectionController->saveCollection(collectionId, filePath)) {
                ui->logTextEdit->append("Failed to save collection, local backend not added");
                return;
            }
            ui->logTextEdit->append("Collection saved to " + filePath);
        } else {
            ui->logTextEdit->append("Local backend addition canceled for transient collection");
            return;
        }
    }

    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Local Backend Directory"), QDir::currentPath());
    if (dir.isEmpty()) {
        ui->logTextEdit->append("Local backend addition canceled");
        return;
    }
    LocalBackend *backend = new LocalBackend(dir, this);
    collectionController->attachLocalBackend(collectionId, backend);
    ui->logTextEdit->append("Local backend added at " + dir);
}

void MainWindow::onSaveCollection()
{
    if (!activeCollection) {
        ui->logTextEdit->append("No active collection to save");
        return;
    }
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Collection"), QDir::currentPath(), tr("KALB Files (*.kalb)"));
    if (filePath.isEmpty()) {
        ui->logTextEdit->append("Save collection canceled");
        return;
    }
    if (collectionController->saveCollection(activeCollection->id(), filePath)) {
        ui->logTextEdit->append("Collection saved to " + filePath);
    } else {
        ui->logTextEdit->append("Failed to save collection to " + filePath);
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
    collectionController->loadCollection("Loaded Collection", nullptr, false, filePath);
    ui->logTextEdit->append("Collection loaded from " + filePath);
    qDebug() << "MainWindow: Collection load requested from" << filePath;
}
