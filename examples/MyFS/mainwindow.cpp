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
    ui->fileBox->setEnabled(true);
    ui->dirBox->setEnabled(true);
    ui->sfMount->setEnabled(true);
}

void MainWindow::on_dirChange_pressed()
{
    ui->dirstatus->setText(tr("Not set."));
    mountDir = QFileDialog::getExistingDirectory(this, tr("Choose your mount point:"));
    if (!mountDir.isEmpty())
        ui->dirstatus->setText(mountDir);
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
    /* One can delete the file afterwards, but since this is only an example, we will just do one more check */
    if (!QFile::exists(filename))
    {
        QMessageBox::warning(this, tr("Error"), tr("The container file does not exist anymore!"));
        return;
    }
    fs = new MyFS(mountDir, filename);
    if (!fs->checkStatus())
    {
        delete fs;
        fs = NULL;
        QMessageBox::warning(this, tr("Error"), tr("The mount operation failed (enable debug option to see the details)."));
        return;
    }
    ui->sfMount->setEnabled(false);
    ui->fileBox->setEnabled(false);
    ui->dirBox->setEnabled(false);
    ui->sfUMount->setEnabled(true);
}

void MainWindow::on_fileload_pressed()
{
    ui->filestatus->setText(tr("Not set."));
    filename = QFileDialog::getOpenFileName(this, tr("Select the container file:"), QString(), tr("Container files (*.sfexample)"));
    if (!filename.isEmpty())
        ui->filestatus->setText(filename);
}

void MainWindow::on_sfUMount_pressed()
{
    umount();
}

void MainWindow::on_filenew_pressed()
{
    ui->filestatus->setText(tr("Not set."));
    filename = QFileDialog::getSaveFileName(this, tr("Choose a new container file:"), QString(), tr("Container files (*.sfexample)"));
    if (filename.isEmpty())
        return;
    MyFS::createNewFilesystem(filename);
    ui->filestatus->setText(filename);
}
