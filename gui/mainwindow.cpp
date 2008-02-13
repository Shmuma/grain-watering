#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QtGui>


// --------------------------------------------------
// MainWindow
// --------------------------------------------------
MainWindow::MainWindow ()
    : QWidget (0)
{
    setupUi (this);

    // fire timer every second to update display's clocks
    startTimer (1000);
    refreshScreenClock ();

    // connect tool buttons
    connect (configButton, SIGNAL(toggled(bool)), this, SLOT(configButtonToggled(bool)));
    connect (checkButton, SIGNAL(toggled(bool)), this, SLOT(checkButtonToggled(bool)));
    connect (paramsButton, SIGNAL(toggled(bool)), this, SLOT(paramsButtonToggled(bool)));
    connect (sensorsButton, SIGNAL(toggled(bool)), this, SLOT(sensorsButtonToggled(bool)));

    // hide settings panel
    settingsPanel->hide ();
}


void MainWindow::timerEvent (QTimerEvent*)
{
    refreshScreenClock ();
}


void MainWindow::refreshScreenClock ()
{
    dateAndTimeLabel->setText (QDateTime::currentDateTime ().toString ("<b>hh:mm:ss</b>"));
}


void MainWindow::configButtonToggled (bool on)
{
    checkButton->setChecked (false);
    paramsButton->setChecked (false);
    sensorsButton->setChecked (false);

    if (on) {
	stackedWidget->setCurrentIndex (0);
	settingsPanel->show ();
    }
    else
	settingsPanel->hide ();
}


void MainWindow::checkButtonToggled (bool on)
{
    configButton->setChecked (false);
    paramsButton->setChecked (false);
    sensorsButton->setChecked (false);

    if (on) {
	stackedWidget->setCurrentIndex (1);
	settingsPanel->show ();
    }
    else
	settingsPanel->hide ();
}


void MainWindow::paramsButtonToggled (bool on)
{
    checkButton->setChecked (false);
    configButton->setChecked (false);
    sensorsButton->setChecked (false);

    if (on) {
	stackedWidget->setCurrentIndex (2);
	settingsPanel->show ();
    }
    else
	settingsPanel->hide ();
}


void MainWindow::sensorsButtonToggled (bool on)
{
    checkButton->setChecked (false);
    paramsButton->setChecked (false);
    configButton->setChecked (false);

    if (on) {
	stackedWidget->setCurrentIndex (3);
	settingsPanel->show ();
    }
    else
	settingsPanel->hide ();
}
