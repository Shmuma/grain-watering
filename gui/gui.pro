######################################################################
# Automatically generated by qmake (2.01a) ?? ????. 6 18:15:50 2008
######################################################################

TEMPLATE = app
CONFIG += debug
QT += network
TARGET =
DEPENDPATH += .
INCLUDEPATH += .

QMAKE_CXXFLAGS_DEBUG += -O0
QMAKE_LFLAGS_DEBUG += -g

# Input
HEADERS += mainwindow.h doubleclickbutton.h stagecontrol.h daemon.h logger.h bsucontrol.h
SOURCES += main.cpp mainwindow.cpp doubleclickbutton.cpp stagecontrol.cpp daemon.cpp logger.cpp bsucontrol.cpp
FORMS += mainwindow.ui

LIBS += -L../lib/core -lcore
INCLUDEPATH += ../lib/core

TRANSLATIONS = gui_ru.ts
