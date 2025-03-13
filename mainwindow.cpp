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
    connect(collectionManager, &CollectionManager::calendarsFetched, this, &MainWindow::onCalendarsFetched);
    connect(collectionManager, &CollectionManager::itemsFetched, this, &MainWindow::onItemsFetched);
    connect(ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated); // New

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
    if (collectionManager->getCal(activeCal)) {
        qDebug() << "MainWindow: Focused on" << activeCal;
    }
}

void MainWindow::addLocalCollection()
{
    qDebug() << "MainWindow: Adding local collection";
    Collection *col = new Collection("local0", "Local Collection", this);
    collectionManager->addCollection("local0", new LocalBackend("/path/to/local/storage", this)); // Adjust path
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
    }
    activeCal = collection->calendars().isEmpty() ? "" : collection->calendars().first()->id();
    ui->logTextEdit->append("Collection fetched with " + QString::number(collection->calendars().size()) + " calendars");
}

void MainWindow::onCalendarsFetched(const QString &collectionId, const QList<CalendarMetadata> &calendars)
{
    Q_UNUSED(calendars);
    if (activeCollection && activeCollection->id() == collectionId) {
        for (Cal *cal : activeCollection->calendars()) {
            if (cal->id() == "cal1") continue;
            addCalendarView(cal);
        }
    }
}

void MainWindow::onItemsFetched(Cal *cal, QList<CalendarItem*> items)
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
        if (collectionManager->getCal(activeCal)) {
            qDebug() << "MainWindow: Focused on" << activeCal;
        } else {
            qDebug() << "MainWindow: ActiveCal" << activeCal << "not found in m_calMap";
            activeCal = "";
        }
    }
}
