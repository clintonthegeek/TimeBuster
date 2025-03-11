#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class CollectionManager; // Forward declaration - stub for now

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
    Ui::MainWindow *ui;
    CollectionManager *collectionManager; // Stubbed - null for now
};

#endif // MAINWINDOW_H
