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
    connect (applyStagesButton, SIGNAL (clicked ()), this, SLOT (applyStagesButtonClicked ()));

    // hide settings panel
    settingsPanel->hide ();

    // tune stage controls
    stageControl1->setNumber (1);
    stageControl2->setNumber (2);
    stageControl3->setNumber (3);
    stageControl4->setNumber (4);

    // connect button
    connect (connectButton, SIGNAL (doubleClicked ()), this, SLOT (connectButtonClicked ()));
    connect (consoleSendButton, SIGNAL (clicked ()), this, SLOT (consoleSendButtonClicked ()));

    // view button
    QMenu* connectMenu = new QMenu (this);
    QActionGroup* actionGroup = new QActionGroup (this);
    QAction* action;

    action = new QAction (tr ("Stages"), this);
    action->setCheckable (true);
    action->setChecked (true);
    action->setShortcut (tr ("Ctrl+S"));
    actionGroup->addAction (action);
    connectMenu->addAction (action);
    connect (action, SIGNAL (triggered ()), this, SLOT (switchToStagesView ()));

    action = new QAction (tr ("History"), this);
    action->setCheckable (true);
    action->setShortcut (tr ("Ctrl+H"));
    actionGroup->addAction (action);
    connectMenu->addAction (action);
    connect (action, SIGNAL (triggered ()), this, SLOT (switchToHistoryView ()));

    action = new QAction (tr ("Console"), this);
    action->setCheckable (true);
    action->setShortcut (tr ("Ctrl+C"));
    actionGroup->addAction (action);
    connectMenu->addAction (action);
    connect (action, SIGNAL (triggered ()), this, SLOT (switchToConsoleView ()));

    viewButton->setMenu (connectMenu);

    // daemon state signals
    connect (&_daemon, SIGNAL (connectedChanged (bool)), this, SLOT (connectedChanged (bool)));
    connect (&_daemon, SIGNAL (textArrived (const QString&)), this, SLOT (daemonTextReceived (const QString&)));
    connect (&_daemon, SIGNAL (commandSent (const QString&)), this, SLOT (daemonCommandSent (const QString&)));
    connect (&_daemon, SIGNAL (stagesActivityChanged (bool,bool,bool,bool)), this, SLOT (daemonStagesActivityChanged (bool,bool,bool,bool)));
    connect (&_daemon, SIGNAL (grainFlowGot (int, int)), this, SLOT (daemonGrainFlowGot (int, int)));
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


void MainWindow::loggerMessage (Logger::severity_t, const QString& msg)
{
    logListView->addItem (msg);
}


StageControl* MainWindow::getStageControl (int stage) const
{
    switch (stage) {
    case 1:
        return stageControl1;
    case 2:
        return stageControl2;
    case 3:
        return stageControl3;
    case 4:
        return stageControl4;
    default:
        return NULL;
    }
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


void MainWindow::daemonTextReceived (const QString& msg)
{
    consoleEditor->append (msg);
    consoleEditor->moveCursor (QTextCursor::End);
}


void MainWindow::daemonCommandSent (const QString& msg)
{
    consoleEditor->insertPlainText (msg);
    consoleEditor->moveCursor (QTextCursor::End);
}


void MainWindow::consoleSendButtonClicked ()
{
    if (!consoleCommandInput->text ().isEmpty ()) {
        if (consoleCommandInput->text () == "clear") {
            consoleEditor->clear ();
            consoleCommandInput->clear ();
        }
        else {
            _daemon.sendRawCommand (consoleCommandInput->text ());
            consoleCommandInput->clear ();
        }
    }
}


void MainWindow::switchToStagesView ()
{
    mainStackWidget->setCurrentIndex (0);
}


void MainWindow::switchToHistoryView ()
{
    mainStackWidget->setCurrentIndex (1);
}


void MainWindow::switchToConsoleView ()
{
    mainStackWidget->setCurrentIndex (2);
}


void MainWindow::applyStagesButtonClicked ()
{
    _daemon.setStages (stage1ActiveCheckBox->isChecked (), stage2ActiveCheckBox->isChecked (), 
                       stage3ActiveCheckBox->isChecked (), stage4ActiveCheckBox->isChecked ());
}


void MainWindow::daemonStagesActivityChanged (bool s1, bool s2, bool s3, bool s4)
{
    stageControl1->setEnabled (s1);
    stageControl2->setEnabled (s2);
    stageControl3->setEnabled (s3);
    stageControl4->setEnabled (s4);
}


void MainWindow::daemonGrainFlowGot (int stage, int value)
{
    getStageControl (stage)->setGrainFlow (value);
}
