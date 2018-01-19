#-------------------------------------------------
#
# Project created by QtCreator 2017-05-23T21:24:35
#
#-------------------------------------------------

QT       += core gui serialport widgets winextras multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = newline-2nd
TEMPLATE = app

include(com_interface/com_interface.pri)
include(ops_operate/ops_operate.pri)
include(Pub/pub.pri)
INCLUDEPATH += Gui


#############################################################################
##目前对应的版本，即 HHT_RELEASE，HHT_GERMANY
###以下是各个版本的描述###
## HHT_RELEASE 对应对外发布版本，
## HHT_GERMANY 对应德国固件发布版本,
#############################################################################
#############################################################################
###注：编译不同版本时，只需修改以下这个宏定义即可###
DEFINES += HHT_GERMANY
#############################################################################

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
    Helper/fontscaleratio.cpp \
    Gui/pubusbdialog.cpp

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
    Helper/fontscaleratio.h \
    Gui/pubusbdialog.h

FORMS    += maindialog.ui \
    Gui/aboutdialog.ui \
    Gui/frmmessagebox.ui \
    Gui/hintdialog.ui \
    Gui/uploadwidget.ui \
    Gui/montagedialog.ui \
    Gui/pubusbdialog.ui

RESOURCES += \
    resource.qrc
RC_FILE += app.rc

DISTFILES += \
    Resource/images/alert.png \
    Resource/images/warning.png
