#-------------------------------------------------
#
# Project created by QtCreator 2017-05-23T21:24:35
#
#-------------------------------------------------

QT       += core gui serialport widgets winextras multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = newline-1nd
TEMPLATE = app

include(com_interface/com_interface.pri)
include(ops_operate/ops_operate.pri)
include(Pub/pub.pri)
INCLUDEPATH += Gui

SOURCES += main.cpp\
        maindialog.cpp \
    Helper/iconhelper.cpp \
    Gui/aboutdialog.cpp \
    Gui/frmmessagebox.cpp \
    nlistwidget.cpp \
    trashwidget.cpp \
    Gui/hintdialog.cpp \
    maskwidget.cpp \
    Gui/uploadwidget.cpp \
    Gui/montagedialog.cpp \
    Helper/fontscaleratio.cpp

HEADERS  += maindialog.h \
    hhtheader.h \
    Helper/hhthelper.h \
    Helper/iconhelper.h \
    Gui/aboutdialog.h \
    Gui/frmmessagebox.h \
    nlistwidget.h \
    trashwidget.h \
    Gui/hintdialog.h \
    maskwidget.h \
    Gui/uploadwidget.h \
    Gui/montagedialog.h \
    Helper/fontscaleratio.h

FORMS    += maindialog.ui \
    Gui/aboutdialog.ui \
    Gui/frmmessagebox.ui \
    Gui/hintdialog.ui \
    Gui/uploadwidget.ui \
    Gui/montagedialog.ui

RESOURCES += \
    resource.qrc
RC_FILE += app.rc

DISTFILES += \
    Resource/images/alert.png \
    Resource/images/warning.png
