#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "myfs.h"

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), mountDir(), fs(NULL)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    if (fs) umount();
    delete ui;
}

void MainWindow::umount()
{
    if (!fs)
    {
#ifndef QT_NO_DEBUG
        qDebug() << "Unexpected umount()";
#endif
        return;
    }
    delete fs;
    fs = NULL;
    ui->sfUMount->setEnabled(false);
    ui->sfMount->setEnabled(true);
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
    if (fs)
    {
#ifndef QT_NO_DEBUG
        qDebug() << "Unexpected mount()";
#endif
        return;
    }
    if (mountDir.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"), tr("You have to specify a mount point (directory) before!"));
        return;
    }
    if (filename.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"), tr("You have to specify a container file before!"));
        return;
    }
    // TODO
}

void MainWindow::on_fileload_pressed()
{
    filename = QFileDialog::getOpenFileName(this, tr("Select the container file:"), QString(), tr("Container files (*.sfexample)"));
    if (filename.isEmpty())
    {
        ui->filestatus->setText(tr("Not set."));
    } else {
        ui->filestatus->setText(filename);
    }
}
