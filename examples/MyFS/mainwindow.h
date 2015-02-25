#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MyFS;

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void myExit();
private:
    void umount();
private slots:
    void on_dirChange_pressed();
    void on_sfMount_pressed();
    void on_fileload_pressed();
    void on_sfUMount_pressed();
    void on_filenew_pressed();
private:
    Ui::MainWindow *ui;
    QString mountDir, filename;
    MyFS *fs;
};

#endif // MAINWINDOW_H
