#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>

class CollectionManager; // Already there
class Cal;
class Collection;

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
    void onCollectionAdded(Collection *collection); // New slot

private:
    void addCalendarView(Cal *cal);

    Ui::MainWindow *ui;
    CollectionManager *collectionManager; // Now fully used
};

#endif // MAINWINDOW_H
