#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_dirChange_pressed()
{
    mountDir = QFileDialog::getExistingDirectory(this, tr("Choose your mount point:"));
    if (mountDir.isEmpty())
    {
        ui->dirstatus->setText(tr("Not set."));
    } else {
        ui->dirstatus->setText(mountDir);
    }
}

void MainWindow::on_sfMount_pressed()
{
    if (mountDir.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"), tr("You have to specify a mount point (directory) before!"));
        return;
    }
    // TODO
}
