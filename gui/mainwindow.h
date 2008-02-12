#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QtCore>
#include <QtGui>
#include "ui_mainwindow.h"


class MainWindow : public QWidget, private Ui::MainWindow
{
    Q_OBJECT

protected:
    virtual void timerEvent (QTimerEvent* event);

    void refreshScreenClock ();
public:
    MainWindow ();
};


#endif
