#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calendarview.h" // Added for CalendarView

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), collectionManager(nullptr)
{
    ui->setupUi(this);

    // Connect menu actions to slots
    connect(ui->actionAddLocalCollection, &QAction::triggered, this, &MainWindow::addLocalCollection);
    connect(ui->actionAddRemoteCollection, &QAction::triggered, this, &MainWindow::addRemoteCollection);
    connect(ui->actionCreateLocalFromRemote, &QAction::triggered, this, &MainWindow::createLocalFromRemote);
    connect(ui->actionSyncCollections, &QAction::triggered, this, &MainWindow::syncCollections);

    Cal * aCal = new Cal("Hello", "hi there", this);
    addCalendarView(aCal);
}

MainWindow::~MainWindow()
{
    delete ui;
    // collectionManager is null for now, no cleanup needed
}

void MainWindow::addCalendarView(Cal *cal)
{
    CalendarView *view = new CalendarView(cal, this);
    QMdiSubWindow *subWindow = new QMdiSubWindow(this);
    subWindow->setWidget(view);
    subWindow->setWindowTitle(cal->displayName());
    ui->mdiArea->addSubWindow(subWindow);
    subWindow->resize(400, 300); // Match CalendarView's default size
    subWindow->show();
}

void MainWindow::addLocalCollection()
{
    Cal *cal = new Cal("cal1", "Test Calendar", this);
    addCalendarView(cal);
    ui->logTextEdit->append("Added local calendar: Test Calendar");
}

void MainWindow::addRemoteCollection()
{
    ui->logTextEdit->append("Add Remote Collection triggered (stub)");
}

void MainWindow::createLocalFromRemote()
{
    ui->logTextEdit->append("Create Local from Remote triggered (stub)");
}

void MainWindow::syncCollections()
{
    ui->logTextEdit->append("Sync Collections triggered (stub)");
}
