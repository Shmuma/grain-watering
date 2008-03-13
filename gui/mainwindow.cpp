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
    connect (startButton, SIGNAL (clicked ()), this, SLOT (startButtonClicked ()));
    connect (stopButton,  SIGNAL (clicked ()), this, SLOT (stopButtonClicked ()));
    connect (pauseButton, SIGNAL (clicked ()), this, SLOT (pauseButtonClicked ()));

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
    //    autoLogEditor->hide ();

    // daemon state signals
    connect (&_daemon, SIGNAL (connectedChanged (bool)), this, SLOT (connectedChanged (bool)));
    connect (&_daemon, SIGNAL (hardwareConnected ()), this, SLOT (daemonHardwareConnected ()));
    connect (&_daemon, SIGNAL (textArrived (const QString&)), this, SLOT (daemonTextReceived (const QString&)));
    connect (&_daemon, SIGNAL (autoTextArrived (const QString&)), this, SLOT (daemonAutoTextReceived (const QString&)));
    connect (&_daemon, SIGNAL (commandSent (const QString&)), this, SLOT (daemonCommandSent (const QString&)));
    connect (&_daemon, SIGNAL (stagesActivityChanged (bool,bool,bool,bool)), this, SLOT (daemonStagesActivityChanged (bool,bool,bool,bool)));
    connect (&_daemon, SIGNAL (grainFlowGot (int, int)), this, SLOT (daemonGrainFlowGot (int, int)));
    connect (&_daemon, SIGNAL (autoModeTickGot (bool, int)), this, SLOT (daemonAutoModeTickGot (bool, int)));
    connect (&_daemon, SIGNAL (autoModeGot (bool, bool)), this, SLOT (daemonAutoModeGot (bool, bool)));
    connect (&_daemon, SIGNAL (autoModeStarted ()), this, SLOT (daemonAutoModeStarted ()));
    connect (&_daemon, SIGNAL (autoModeStopped ()), this, SLOT (daemonAutoModeStopped ()));
    connect (&_daemon, SIGNAL (autoModeToggled (bool)), this, SLOT (daemonAutoModeToggled (bool)));
    connect (&_daemon, SIGNAL (metaStateGot (int, QMap<int, QList<int> >)), this, SLOT (daemonMetaStateGot (int, QMap<int, QList<int> >)));
    connect (&_daemon, SIGNAL (waterStarted (int)), this, SLOT (daemonWaterStarted (int)));
    connect (&_daemon, SIGNAL (waterStopped (int)), this, SLOT (daemonWaterStopped (int)));

    connect (checkStateButton, SIGNAL (pressed ()), this, SLOT (checkStateButtonPressed ()));
    connect (checkWaterButton, SIGNAL (pressed ()), this, SLOT (checkWaterButtonPressed ()));
    connect (checkGrainSensorsButton, SIGNAL (pressed ()), this, SLOT (checkGrainSensorsButtonPressed ()));
    connect (stateRefreshButton, SIGNAL (clicked ()), this, SLOT (stateRefreshButtonClicked ()));
    connect (applyCheckWaterButton, SIGNAL (clicked ()), this, SLOT (applyCheckWaterButtonClicked ()));
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


// hardware connection got, fetch daemon state
void MainWindow::daemonHardwareConnected ()
{
    _daemon.getAutoMode ();
}


void MainWindow::daemonTextReceived (const QString& msg)
{
    consoleEditor->append (msg);
    consoleEditor->moveCursor (QTextCursor::End);
}


void MainWindow::daemonAutoTextReceived (const QString& msg)
{
    autoLogEditor->append (msg);
    autoLogEditor->moveCursor (QTextCursor::End);
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

    checkWaterStage1Check->setEnabled (s1);
    checkWaterStage2Check->setEnabled (s2);
    checkWaterStage3Check->setEnabled (s3);
    checkWaterStage4Check->setEnabled (s4);
}


void MainWindow::daemonGrainFlowGot (int stage, int value)
{
    getStageControl (stage)->setGrainFlow (value);
}


void MainWindow::startButtonClicked ()
{
    _daemon.startAutoMode ();
}


void MainWindow::stopButtonClicked ()
{
    _daemon.stopAutoMode ();
}


void MainWindow::pauseButtonClicked ()
{
    _daemon.toggleAutoMode ();
}


void MainWindow::daemonAutoModeTickGot (bool state, int press)
{
    Logger::instance ()->log (Logger::Debug, QString ("Auto mode tick. State: %1, Press: %2").arg (state ? "OK" : "ERROR", QString::number (press)));
}


void MainWindow::daemonAutoModeStarted ()
{
    startButton->setChecked (true);
    stopButton->setChecked (false);
    autoLogEditor->show ();
}


void MainWindow::daemonAutoModeStopped ()
{
    startButton->setChecked (false);
    stopButton->setChecked (true);
    autoLogEditor->hide ();
}


void MainWindow::daemonAutoModeToggled (bool paused)
{
    pauseButton->setChecked (paused);
}


void MainWindow::daemonAutoModeGot (bool active, bool paused)
{
    if (active) {
        startButton->setChecked (true);
        stopButton->setChecked (false);
        autoLogEditor->show ();
    }
    else {
        startButton->setChecked (false);
        stopButton->setChecked (true);
        autoLogEditor->hide ();
    }

    pauseButton->setChecked (paused);
}


void MainWindow::daemonMetaStateGot (int water_pres, QMap<int, QList<int> > vals)
{
    stateTreeWidget->clear ();
    QTreeWidgetItem* glob = new QTreeWidgetItem (stateTreeWidget);
    glob->setText (0, tr ("Global"));
             glob->setExpanded (true);

    QTreeWidgetItem* n = new QTreeWidgetItem (glob);
    n->setText (0, tr ("Water pressure"));
    n->setText (1, QString::number (water_pres));

    for (int i = 1; i <= 4; i++) {
        if (vals.find (i) == vals.end ())
            continue;
        QTreeWidgetItem* item = new QTreeWidgetItem (stateTreeWidget);
        item->setExpanded (true);
        item->setText (0, tr ("Stage %1").arg (i));
        
        n = new QTreeWidgetItem (item);
        n->setText (0, tr ("Water flow"));
        n->setText (1, QString::number (vals[i][0]));
            
        n = new QTreeWidgetItem (item);
        n->setText (0, tr ("Grain flow"));
        n->setText (1, QString::number (vals[i][1]));
            
        n = new QTreeWidgetItem (item);
        n->setText (0, tr ("Grain humidity"));
        n->setText (1, QString::number (vals[i][2]));
            
        n = new QTreeWidgetItem (item);
        n->setText (0, tr ("Grain temp."));
        n->setText (1, QString::number (vals[i][3]));
            
        n = new QTreeWidgetItem (item);
        n->setText (0, tr ("Grain nature"));
        n->setText (1, QString::number (vals[i][4]));
    }
}


void MainWindow::daemonWaterStarted (int stage)
{
    Logger::instance ()->log (Logger::Information, tr ("Water started on %1 stage").arg (stage));
}


void MainWindow::daemonWaterStopped (int stage)
{
    Logger::instance ()->log (Logger::Information, tr ("Water stopped on %1 stage").arg (stage));
}


void MainWindow::stateRefreshButtonClicked ()
{
    _daemon.refreshState ();
}


void MainWindow::checkStateButtonPressed ()
{
    checkStackedWidget->setCurrentIndex (0);
}


void MainWindow::checkWaterButtonPressed ()
{
    checkStackedWidget->setCurrentIndex (1);
}


void MainWindow::checkGrainSensorsButtonPressed ()
{
    checkStackedWidget->setCurrentIndex (2);
}


void MainWindow::applyCheckWaterButtonClicked ()
{
    if (_daemon.isStageEnabled (1))
        if (checkWaterStage1Check->isChecked ())
            _daemon.startWater (1);
        else
            _daemon.stopWater (1);

    if (_daemon.isStageEnabled (2))
        if (checkWaterStage2Check->isChecked ())
            _daemon.startWater (2);
        else
            _daemon.stopWater (3);

    if (_daemon.isStageEnabled (3))
        if (checkWaterStage3Check->isChecked ())
            _daemon.startWater (3);
        else
            _daemon.stopWater (3);

    if (_daemon.isStageEnabled (4))
        if (checkWaterStage4Check->isChecked ())
            _daemon.startWater (4);
        else
            _daemon.stopWater (4);
}


