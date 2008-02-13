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

protected slots:
    void configButtonToggled (bool on);
    void checkButtonToggled (bool on);
    void paramsButtonToggled (bool on);
    void sensorsButtonToggled (bool on);

public:
    MainWindow ();
};


#endif
