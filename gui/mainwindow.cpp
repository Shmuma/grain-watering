#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stagecontrol.h"
#include "logger.h"

#include <QtCore>
#include <QtGui>


// --------------------------------------------------
// MainWindow
// --------------------------------------------------
MainWindow::MainWindow ()
    : QWidget (0),
      _switchingToolButtons (false),
      _daemon ("localhost", 12345)
{
    setupUi (this);

    // fire timer every second to update display's clocks
    startTimer (1000);
    refreshScreenClock ();

    // logger
    Logger::instance ()->setMinSeverity (Logger::Debug);
    connect (Logger::instance (), SIGNAL (message (Logger::severity_t, const QString&)), 
             this, SLOT (loggerMessage (Logger::severity_t, const QString&)));

    // connect tool buttons
    connect (configButton, SIGNAL(toggled(bool)), this, SLOT(configButtonToggled(bool)));
    connect (checkButton, SIGNAL(toggled(bool)), this, SLOT(checkButtonToggled(bool)));
    connect (paramsButton, SIGNAL(toggled(bool)), this, SLOT(paramsButtonToggled(bool)));
    connect (sensorsButton, SIGNAL(toggled(bool)), this, SLOT(sensorsButtonToggled(bool)));

    // connect active stages checkboxes
    connect (stage1ActiveCheckBox, SIGNAL(toggled(bool)), this, SLOT(stage1ActiveCheckBoxToggled(bool)));
    connect (stage2ActiveCheckBox, SIGNAL(toggled(bool)), this, SLOT(stage2ActiveCheckBoxToggled(bool)));
    connect (stage3ActiveCheckBox, SIGNAL(toggled(bool)), this, SLOT(stage3ActiveCheckBoxToggled(bool)));
    connect (stage4ActiveCheckBox, SIGNAL(toggled(bool)), this, SLOT(stage4ActiveCheckBoxToggled(bool)));

    // state signals
    connect (&_state, SIGNAL(stageEnabledChanged(int, bool)), this, SLOT(stageEnabledChanged(int, bool)));

    // hide settings panel
    settingsPanel->hide ();

    // tune stage controls
    stageControl1->setNumber (1);
    stageControl2->setNumber (2);
    stageControl3->setNumber (3);
    stageControl4->setNumber (4);

    // connect button
    connect (connectButton, SIGNAL (doubleClicked ()), this, SLOT (connectButtonClicked ()));

    // daemon state signals
    connect (&_daemon, SIGNAL (connectedChanged (bool)), this, SLOT (connectedChanged (bool)));
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
    if (on) {
        _switchingToolButtons = true;
        checkButton->setChecked (false);
        paramsButton->setChecked (false);
        sensorsButton->setChecked (false);
        _switchingToolButtons = false;
	stackedWidget->setCurrentIndex (0);
	settingsPanel->show ();
    }
    else
        if (!_switchingToolButtons)
            settingsPanel->hide ();
}


void MainWindow::checkButtonToggled (bool on)
{
    if (on) {
        _switchingToolButtons = true;
        configButton->setChecked (false);
        paramsButton->setChecked (false);
        sensorsButton->setChecked (false);
        _switchingToolButtons = false;
	stackedWidget->setCurrentIndex (1);
	settingsPanel->show ();
    }
    else
        if (!_switchingToolButtons)
            settingsPanel->hide ();
}


void MainWindow::paramsButtonToggled (bool on)
{
    if (on) {
        _switchingToolButtons = true;
        configButton->setChecked (false);
        checkButton->setChecked (false);
        sensorsButton->setChecked (false);
        _switchingToolButtons = false;
	stackedWidget->setCurrentIndex (2);
	settingsPanel->show ();
    }
    else
        if (!_switchingToolButtons)
            settingsPanel->hide ();
}


void MainWindow::sensorsButtonToggled (bool on)
{
    if (on) {
        _switchingToolButtons = true;
        configButton->setChecked (false);
        checkButton->setChecked (false);
        paramsButton->setChecked (false);
        _switchingToolButtons = false;
	stackedWidget->setCurrentIndex (3);
	settingsPanel->show ();
    }
    else
        if (!_switchingToolButtons)
            settingsPanel->hide ();
}


void MainWindow::stage1ActiveCheckBoxToggled (bool on)
{
    _state.setStageEnabled (1, on);
}


void MainWindow::stage2ActiveCheckBoxToggled (bool on)
{
    _state.setStageEnabled (2, on);
}


void MainWindow::stage3ActiveCheckBoxToggled (bool on)
{
    _state.setStageEnabled (3, on);
}


void MainWindow::stage4ActiveCheckBoxToggled (bool on)
{
    _state.setStageEnabled (4, on);
}


void MainWindow::stageEnabledChanged (int stages, bool enabled)
{
    switch (stages) {
    case 1:
        stageControl1->setEnabled (enabled);
        break;
    case 2:
        stageControl2->setEnabled (enabled);
        break;
    case 3:
        stageControl3->setEnabled (enabled);
        break;
    case 4:
        stageControl4->setEnabled (enabled);
        break;
    }
}


void MainWindow::loggerMessage (Logger::severity_t sev, const QString& msg)
{
    logListView->addItem (msg);
}


// GUI events
void MainWindow::connectButtonClicked ()
{
    // Try to connect/disconnect. Don't care about GUI update, daemon
    // sends signal about state change.
    if (_daemon.connected ())
        _daemon.disconnect ();
    else
        _daemon.connect ();
}


// daemon signals handlers
void MainWindow::connectedChanged (bool value)
{
    // toggle connect button
    connectButton->setChecked (value);
}
