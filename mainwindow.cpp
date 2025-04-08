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
#include <QLabel>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), credentialsDialog(new CredentialsDialog(this)),
    collectionController(new CollectionController(this)), sessionManager(new SessionManager(collectionController, this)),
    activeCollection(nullptr), activeCal(QString()), editPane(new EditPane(nullptr, this)), currentItem(nullptr)
{
    ui->setupUi(this);


    // Initialize the tree model
    collectionModel = new QStandardItemModel(this);
    QTreeView *collectionTree = ui->propertiesDock->findChild<QTreeView*>("collectionTree");
    if (!collectionTree) {
        qFatal("Failed to find collectionTree in propertiesDock");
    }
    collectionTree->setModel(collectionModel);
    collectionTree->setHeaderHidden(true);

    // Set up context menu for the tree
    collectionTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(collectionTree, &QTreeView::customContextMenuRequested, this, [this, collectionTree](const QPoint &pos) {
        QModelIndex index = collectionTree->indexAt(pos);
        if (!index.isValid()) return;

        QStandardItem *item = collectionModel->itemFromIndex(index);
        if (item->parent()) return; // Only show context menu for the root item (collection)

        QMenu contextMenu(this);
        QAction *closeAction = contextMenu.addAction("Close Collection");
        connect(closeAction, &QAction::triggered, this, &MainWindow::onCloseCollection); // Updated

        contextMenu.exec(collectionTree->viewport()->mapToGlobal(pos));
    });

    // Connect tree clicks to open/focus subwindows
    connect(collectionTree, &QTreeView::clicked, this, &MainWindow::onTreeClicked);

    // Dock EditPane using widget()

    ui->editDock->setWidget(editPane->widget());

    // Tabify stageDock and propertiesDock
    tabifyDockWidget(ui->logDock, ui->stageDock); // Use 'this' implicitly

    // Set minimum widths for docks
    ui->editDock->setMinimumWidth(200);
    ui->stageDock->setMinimumWidth(200);
    ui->propertiesDock->setMinimumWidth(200);
    ui->logDock->setMinimumHeight(100);

    // Connect EditPane’s itemModified to SessionManager
    connect(editPane, &EditPane::itemModified, sessionManager,
            [this](const QList<QSharedPointer<CalendarItem>>& items) {
                for (const auto& item : items) {
                    sessionManager->queueDeltaChange(activeCal, item, "modify");
                    ui->logTextEdit->append("Staged change for " + item->id());
                }
            });

    // Connect changesStaged to refresh views
    connect(sessionManager, &SessionManager::changesStaged, this, [this]() {
        for (QMdiSubWindow *window : ui->mdiArea->subWindowList()) {
            if (CalendarTableView *view = qobject_cast<CalendarTableView*>(window->widget())) {
                view->refresh();
                qDebug() << "MainWindow: Refreshed view for" << view->activeCal()->id();
            }
        }
    });

    // Status bar setup
    QLabel *statusLabel = new QLabel("Ready", this);
    QProgressBar *totalProgressBar = new QProgressBar(this);
    QProgressBar *currentProgressBar = new QProgressBar(this);
    ui->statusBar->addWidget(statusLabel, 1);

    QWidget *totalProgressWidget = new QWidget(this);
    QHBoxLayout *totalProgressLayout = new QHBoxLayout(totalProgressWidget);
    QLabel *totalProgressLabel = new QLabel("Total Progress:", this);
    totalProgressBar->setMinimumWidth(100);
    totalProgressLayout->addWidget(totalProgressLabel);
    totalProgressLayout->addWidget(totalProgressBar);
    totalProgressLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *currentProgressWidget = new QWidget(this);
    QHBoxLayout *currentProgressLayout = new QHBoxLayout(currentProgressWidget);
    QLabel *currentProgressLabel = new QLabel("Current Task:", this);
    currentProgressBar->setMinimumWidth(100);
    currentProgressLayout->addWidget(currentProgressLabel);
    currentProgressLayout->addWidget(currentProgressBar);
    currentProgressLayout->setContentsMargins(0, 0, 0, 0);

    ui->statusBar->addPermanentWidget(totalProgressWidget);
    ui->statusBar->addPermanentWidget(currentProgressWidget);

    // Connect menu actions
    connect(ui->actionNewLocalCollection, &QAction::triggered, this, &MainWindow::addLocalCollection);
    connect(ui->actionNewRemoteCollection, &QAction::triggered, this, &MainWindow::addRemoteCollection);
    connect(ui->actionSyncCollections, &QAction::triggered, this, &MainWindow::syncCollections);
    connect(ui->actionSaveCollection, &QAction::triggered, this, &MainWindow::onSaveCollection);
    connect(collectionController, &CollectionController::collectionAdded, this, &MainWindow::onCollectionAdded);
    connect(collectionController, &CollectionController::calendarAdded, this, &MainWindow::onCalendarAdded);
    connect(collectionController, &CollectionController::calendarsLoaded, this, &MainWindow::onCalendarsLoaded);
    connect(collectionController, &CollectionController::itemsLoaded, this, &MainWindow::onItemsLoaded);
    connect(collectionController, &CollectionController::allSyncsCompleted, this, &MainWindow::onAllSyncsCompleted);
    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);
    connect(ui->actionOpenCollection, &QAction::triggered, this, &MainWindow::onOpenCollection);
    connect(ui->actionAddLocalBackend, &QAction::triggered, this, &MainWindow::onAddLocalBackend);
    connect(ui->actionCommitChanges, &QAction::triggered, this, &MainWindow::onCommitChanges);
    connect(ui->actionCloseCollection, &QAction::triggered, this, &MainWindow::onCloseCollection); // Updated

    // Connect CalendarTableView selections dynamically
    connect(collectionController, &CollectionController::calendarAdded, this, [this](Cal *cal) {
        for (QMdiSubWindow *window : ui->mdiArea->subWindowList()) {
            if (CalendarTableView *view = qobject_cast<CalendarTableView*>(window->widget())) {
                if (view->activeCal() == cal) {
                    connect(view, &CalendarTableView::itemSelected, editPane, &EditPane::updateSelection);
                    qDebug() << "MainWindow: Connected itemSelected for" << cal->id();
                }
            }
        }
    });

    qDebug() << "MainWindow: Initialized";
}
MainWindow::~MainWindow()
{
    qDebug() << "MainWindow: Destroying MainWindow";
    delete collectionController; // Ensure controller is deleted before UI
    delete credentialsDialog;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    onCloseCollection();
    event->accept();
}

void MainWindow::addCalendarView(Cal* cal)
{
    CalendarTableView* view = new CalendarTableView(cal, this);
    QMdiSubWindow* subWindow = ui->mdiArea->addSubWindow(view);
    subWindow->setWindowTitle(cal->name());
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    subWindow->resize(400, 300);
    subWindow->show();

    // Store the subwindow in the map
    calToSubWindow[cal->id()] = subWindow;

    connect(view, &CalendarTableView::itemSelected, editPane, &EditPane::updateSelection);
    connect(view, &CalendarTableView::itemModified, sessionManager,
            [this, cal](const QList<QSharedPointer<CalendarItem>>& items) {
                for (const auto& item : items) {
                    sessionManager->queueDeltaChange(cal->id(), item, "modify");
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

void MainWindow::onTreeClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    // Check if the clicked item is a calendar (has a parent, i.e., under the collection root)
    QStandardItem *item = collectionModel->itemFromIndex(index);
    if (!item->parent()) return; // Clicked on the root (collection), not a calendar

    QString calId = item->data(Qt::UserRole).toString();
    Cal *cal = collectionController->getCal(calId);
    if (!cal) return;

    // Check if the subwindow already exists
    QMdiSubWindow *subWindow = calToSubWindow.value(calId, nullptr);
    if (subWindow) {
        // Subwindow exists, bring it to the front
        ui->mdiArea->setActiveSubWindow(subWindow);
        subWindow->show();
    } else {
        // Subwindow doesn’t exist, create a new one
        addCalendarView(cal);
    }

    // Update activeCal and focus
    activeCal = calId;
    editPane->setActiveCal(cal);
    qDebug() << "MainWindow: Switched activeCal to" << calId;
    qDebug() << "MainWindow: Focused on" << calId;
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
    sessionManager->loadStagedChanges(collectionId); // Apply deltas after ICS load
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
    editPane->setCollection(collection);
    ui->logTextEdit->append("Collection opened: " + collection->name());

    // Populate the tree model with the collection as the root
    collectionModel->clear();
    QStandardItem *rootItem = new QStandardItem(collection->name());
    rootItem->setData(collection->id(), Qt::UserRole); // Store collection ID
    collectionModel->appendRow(rootItem);

    // Expand the root item
    QTreeView *collectionTree = ui->propertiesDock->findChild<QTreeView*>("collectionTree");
    if (collectionTree) {
        collectionTree->expand(collectionModel->index(0, 0));
    }
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

void MainWindow::onCalendarAdded(Cal *cal)
{
    qDebug() << "MainWindow: onCalendarAdded for" << cal->id();

    // Add the calendar to the tree model
    QStandardItem *rootItem = collectionModel->item(0); // Collection root
    if (rootItem) {
        QStandardItem *calItem = new QStandardItem(cal->name());
        calItem->setData(cal->id(), Qt::UserRole); // Store calendar ID
        rootItem->appendRow(calItem);
    }

    // Create the subwindow after adding to the tree
    addCalendarView(cal);

    if (activeCal.isEmpty()) {
        activeCal = cal->id(); // Set first calendar as active
        editPane->setActiveCal(cal);
    }
}


void MainWindow::onSubWindowActivated(QMdiSubWindow *window)
{
    if (!activeCollection) {
        activeCal = QString();
        editPane->setActiveCal(nullptr);
        qDebug() << "MainWindow: No active collection, cleared activeCal";
        return;
    }

    if (!window) {
        activeCal = "";
        editPane->setActiveCal(nullptr);
        qDebug() << "MainWindow: No active subwindow, cleared activeCal";

        // Clear tree selection when no subwindow is active
        QTreeView *collectionTree = ui->propertiesDock->findChild<QTreeView*>("collectionTree");
        if (collectionTree) {
            collectionTree->selectionModel()->clearSelection();
        }
        return;
    }

    if (CalendarTableView *view = qobject_cast<CalendarTableView*>(window->widget())) {
        // Get the calendar ID and validate the Cal* pointer
        Cal *cal = view->activeCal();
        if (!cal) {
            activeCal = "";
            editPane->setActiveCal(nullptr);
            qDebug() << "MainWindow: No active calendar in view, cleared activeCal";
            return;
        }

        QString calId = cal->id();
        if (!collectionController->getCal(calId)) {
            activeCal = "";
            editPane->setActiveCal(nullptr);
            qDebug() << "MainWindow: ActiveCal" << calId << "not found in m_calMap, cleared activeCal";
            return;
        }

        // Calendar is valid, proceed with setting it as active
        activeCal = calId;
        editPane->setActiveCal(cal);
        qDebug() << "MainWindow: Switched activeCal to" << activeCal;
        qDebug() << "MainWindow: Focused on" << activeCal;

        // Update tree selection to match the active subwindow
        QTreeView *collectionTree = ui->propertiesDock->findChild<QTreeView*>("collectionTree");
        if (collectionTree) {
            QStandardItem *rootItem = collectionModel->item(0); // Collection root
            if (rootItem) {
                for (int i = 0; i < rootItem->rowCount(); ++i) {
                    QStandardItem *calItem = rootItem->child(i);
                    if (calItem && calItem->data(Qt::UserRole).toString() == activeCal) {
                        QModelIndex index = collectionModel->indexFromItem(calItem);
                        collectionTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
                        break;
                    }
                }
            }
        }

        // Remove closed subwindows from the map
        for (auto it = calToSubWindow.begin(); it != calToSubWindow.end(); ) {
            if (!ui->mdiArea->subWindowList().contains(it.value())) {
                it = calToSubWindow.erase(it);
            } else {
                ++it;
            }
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

void MainWindow::onCloseCollection()
{
    if (!activeCollection) {
        ui->logTextEdit->append("No active collection to close");
        return;
    }

    // Clear active calendar state to avoid dangling pointers
    activeCal = QString();
    editPane->setActiveCal(nullptr);
    editPane->setCollection(nullptr);

    // Disconnect subWindowActivated to prevent signals during closing
    disconnect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);

    // Unload the collection from CollectionController
    QString collectionId = activeCollection->id();
    collectionController->unloadCollection(collectionId);

    // Close all MDI subwindows
    ui->mdiArea->closeAllSubWindows();
    calToSubWindow.clear();

    // Clear the tree model
    collectionModel->clear();

    // Reset state
    activeCollection = nullptr;
    ui->valuePath->setText("(none)");

    ui->logTextEdit->append("Collection closed");
    qDebug() << "MainWindow: Collection closed";

    // Reconnect subWindowActivated for future use
    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);
}

void MainWindow::onCommitChanges()
{
    if (!activeCollection) {
        ui->logTextEdit->append("No active collection to commit changes");
        qDebug() << "MainWindow: No active collection for commit";
        return;
    }

    QMap<QString, QList<QSharedPointer<CalendarItem>>> itemsToCommit;
    for (Cal *cal : activeCollection->calendars()) {
        for (const QSharedPointer<CalendarItem> &item : cal->items()) {
            if (item->isDirty()) {
                itemsToCommit[cal->id()].append(item);
                qDebug() << "MainWindow: Queued dirty item" << item->id() << "from" << cal->id() << "for commit";
            }
        }
    }

    if (itemsToCommit.isEmpty()) {
        ui->logTextEdit->append("No changes to commit");
        qDebug() << "MainWindow: No dirty items to commit for" << activeCollection->id();
        return;
    }

    // Use CollectionController’s backends
    const QMap<QString, QList<SyncBackend*>> &backends = collectionController->backends();
    const QList<SyncBackend*> &collectionBackends = backends.value(activeCollection->id());
    for (SyncBackend *backend : collectionBackends) {
        for (const QString &calId : itemsToCommit.keys()) {
            Cal *cal = collectionController->getCal(calId);
            if (cal) {
                backend->storeItems(cal, itemsToCommit[calId]);
                qDebug() << "MainWindow: Committed" << itemsToCommit[calId].size() << "items to" << calId << "via backend";
                for (const QSharedPointer<CalendarItem> &item : itemsToCommit[calId]) {
                    item->setDirty(false);
                }
            }
        }
    }

    ui->logTextEdit->append("Committed staged changes to backends");
    qDebug() << "MainWindow: Committed changes for collection" << activeCollection->id();

    sessionManager->clearDeltaChanges(activeCollection->id());

    for (QMdiSubWindow *window : ui->mdiArea->subWindowList()) {
        if (CalendarTableView *view = qobject_cast<CalendarTableView*>(window->widget())) {
            view->refresh();
        }
    }
}
