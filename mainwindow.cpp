#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), collectionManager(nullptr)
{
    ui->setupUi(this);

    // Connect menu actions to slots
    connect(ui->actionAddLocalCollection, &QAction::triggered, this, &MainWindow::addLocalCollection);
    connect(ui->actionAddRemoteCollection, &QAction::triggered, this, &MainWindow::addRemoteCollection);
    connect(ui->actionCreateLocalFromRemote, &QAction::triggered, this, &MainWindow::createLocalFromRemote);
    connect(ui->actionSyncCollections, &QAction::triggered, this, &MainWindow::syncCollections);
}

MainWindow::~MainWindow()
{
    delete ui;
    // collectionManager is null for now, no cleanup needed
}

void MainWindow::addLocalCollection()
{
    ui->logTextEdit->append("Add Local Collection triggered (stub)");
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
