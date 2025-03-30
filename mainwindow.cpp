#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calendartableview.h"
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
    collectionController(new CollectionController(this)), sessionManager(new SessionManager(collectionController, this)),
    activeCollection(nullptr), editPane(new EditPane(nullptr, this)), currentItem(nullptr)
{
    ui->setupUi(this);

    // Dock EditPane
    ui->editDock->setWidget(editPane);

    // Connect EditPane’s itemModified to SessionManager
    connect(editPane, &EditPane::itemModified, sessionManager,
            [this](const QList<QSharedPointer<CalendarItem>>& items) {
                for (const auto& item : items) {
                sessionManager->queueDeltaChange(activeCal, item, "modify");                    ui->logTextEdit->append("Staged change for " + item->id());
                }
            });

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
    connect(collectionController, &CollectionController::calendarAdded, this, &MainWindow::onCalendarAdded); // Use this instead
    connect(collectionController, &CollectionController::calendarsLoaded, this, &MainWindow::onCalendarsLoaded);
    connect(collectionController, &CollectionController::itemsLoaded, this, &MainWindow::onItemsLoaded);
    connect(collectionController, &CollectionController::allSyncsCompleted, this, &MainWindow::onAllSyncsCompleted);
    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);
    connect(ui->actionOpenCollection, &QAction::triggered, this, &MainWindow::onOpenCollection);
    connect(ui->actionAddLocalBackend, &QAction::triggered, this, &MainWindow::onAddLocalBackend);
    connect(ui->actionCommitChanges, &QAction::triggered, this, &MainWindow::onCommitChanges); // New connection

    qDebug() << "MainWindow: Initialized";
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow: Destroying MainWindow";
    delete collectionController; // Ensure controller is deleted before UI
    delete credentialsDialog;
    delete ui;
}

void MainWindow::addCalendarView(Cal* cal)
{
    CalendarTableView* view = new CalendarTableView(cal, this);
    QMdiSubWindow* subWindow = ui->mdiArea->addSubWindow(view);
    subWindow->setWindowTitle(cal->name());
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->resize(400, 300);
    subWindow->show();

    connect(view, &CalendarTableView::itemSelected, editPane, &EditPane::updateSelection);
    connect(view, &CalendarTableView::itemModified, sessionManager,
            [this](const QList<QSharedPointer<CalendarItem>>& items) {
                for (const auto& item : items) {
                    sessionManager->queueDeltaChange(activeCal, item, "modify");
                }
            });

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
        if (CalendarTableView *view = qobject_cast<CalendarTableView*>(window->widget())) {
            if (view->activeCal()->id() == cal->id()) {
                qDebug() << "MainWindow: Updating view for" << cal->id() << "with item" << item->id();
                view->refresh();
                break;
            }
        }
    }
}

void MainWindow::onItemSelected(const QList<QSharedPointer<CalendarItem>>& items)
{
    // No longer needed—EditPane handles this via updateSelection
    // Keep as stub for now if needed for logging
    qDebug() << "MainWindow: onItemSelected with" << items.size() << "items";
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


void MainWindow::onSelectionChanged()
{
    qDebug() << "MainWindow: onSelectionChanged called, activeCal =" << activeCal;
    for (QMdiSubWindow *window : ui->mdiArea->subWindowList()) {
        if (CalendarTableView *view = qobject_cast<CalendarTableView*>(window->widget())) {
            if (view->activeCal()->id() == activeCal) {
                editPane->updateSelection({view->selectedItem()}); // Bridge to EditPane
                return;
            }
        }
    }
}


void MainWindow::onAllSyncsCompleted(const QString &collectionId)
{
    qDebug() << "MainWindow: All syncs completed for" << collectionId;
    Collection *col = collectionController->collection(collectionId);
    if (!col) {
        qDebug() << "MainWindow: No collection found for" << collectionId;
        return;
    }
    sessionManager->loadStagedChanges(collectionId); // Loads and applies deltas
    for (QMdiSubWindow *window : ui->mdiArea->subWindowList()) {
        if (CalendarTableView *view = qobject_cast<CalendarTableView*>(window->widget())) {
            view->refresh();
        }
    }
    ui->logTextEdit->append("Collection sync completed with " + QString::number(col->calendars().size()) + " calendars");
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
    editPane->setCollection(collection); // Just update collection
    ui->logTextEdit->append("Collection opened: " + collection->name());
}

void MainWindow::onCalendarsLoaded(const QString &collectionId, const QList<CalendarMetadata> &calendars)
{
    Q_UNUSED(collectionId);
    Q_UNUSED(calendars);
    // Called after calendars are loaded, but before items
}

void MainWindow::onItemsLoaded(Cal *cal, QList<QSharedPointer<CalendarItem>> items)
{
    Q_UNUSED(items);
    for (QMdiSubWindow *window : ui->mdiArea->subWindowList()) {
        if (CalendarTableView *view = qobject_cast<CalendarTableView*>(window->widget())) {
            if (view->activeCal()->id() == cal->id()) {
                view->refresh();
            }
        }
    }
}

void MainWindow::onCalendarAdded(Cal *cal) // New slot
{
    qDebug() << "MainWindow: onCalendarAdded for" << cal->id();
    addCalendarView(cal);
    if (activeCal.isEmpty()) {
        activeCal = cal->id(); // Set first calendar as active
    }
}

void MainWindow::onSubWindowActivated(QMdiSubWindow *window)
{
    if (!window) {
        activeCal = "";
        editPane->setActiveCal(nullptr);
        qDebug() << "MainWindow: No active subwindow, cleared activeCal";
        return;
    }
    if (CalendarTableView *view = qobject_cast<CalendarTableView*>(window->widget())) {
        activeCal = view->activeCal()->id();
        editPane->setActiveCal(view->activeCal());
        qDebug() << "MainWindow: Switched activeCal to" << activeCal;
        if (collectionController->getCal(activeCal)) {
            qDebug() << "MainWindow: Focused on" << activeCal;
        } else {
            qDebug() << "MainWindow: ActiveCal" << activeCal << "not found in m_calMap";
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

void MainWindow::onCommitChanges()
{
    if (!activeCollection) {
        ui->logTextEdit->append("No active collection to commit changes");
        qDebug() << "MainWindow: No active collection for commit";
        return;
    }

    sessionManager->applyDeltaChanges();
    ui->logTextEdit->append("Committed staged changes to backends");
    qDebug() << "MainWindow: Committed changes for collection" << activeCollection->id();

    for (QMdiSubWindow *window : ui->mdiArea->subWindowList()) {
        if (CalendarTableView *view = qobject_cast<CalendarTableView*>(window->widget())) {
            view->refresh();
        }
    }
}
