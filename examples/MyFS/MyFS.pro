#-------------------------------------------------
#
# Project created by QtCreator 2015-02-05T20:22:43
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DEFINES += "_FILE_OFFSET_BITS=64"
LIBS += -lfuse

TARGET = MyFS
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    sfuse/simplifier.cpp \
    sfuse/qsimplefuse.cpp \
    myfs.cpp

HEADERS  += mainwindow.h \
    sfuse/simplifier.h \
    sfuse/qsimplefuse.h \
    myfs.h

FORMS    += mainwindow.ui
