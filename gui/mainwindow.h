#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QWidget>
#include "ui_mainwindow.h"


class MainWindow : public QWidget, private Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow ();
};


#endif
