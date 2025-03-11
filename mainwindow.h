#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow> // Added for MDI wrapping

class CollectionManager; // Forward declaration - stub for now
class Cal; // Forward declaration - for addCalendarView

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addLocalCollection();
    void addRemoteCollection();
    void createLocalFromRemote();
    void syncCollections();

private:
    void addCalendarView(Cal *cal); // Updated to wrap in QMdiSubWindow

    Ui::MainWindow *ui;
    CollectionManager *collectionManager; // Stubbed - null for now
};

#endif // MAINWINDOW_H
