# Add common source files here
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
           com_interface/comdev.h \
           com_interface/comOperate.h \
           com_interface/com_interface_extern.h

SOURCES += \
           com_interface/comdev.cpp \
           com_interface/comOperate.cpp
