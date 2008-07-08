#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stagecontrol.h"
#include "logger.h"
#include "tableform.h"

#include <QtCore>
#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>


// --------------------------------------------------
// MainWindow
// --------------------------------------------------
MainWindow::MainWindow ()
    : QWidget (0),
      _switchingToolButtons (false),
      _settingsChanged (false),
      _daemon ("localhost", 12345),
      _currentSettingsStage (0),
      _access (AL_Default),
      _calibrateStage (0),
      _cleaningWatchdogTimer (0)
{
    setupUi (this);

    // fire timer every second to update display's clocks
    _clockTimer = startTimer (1000);
    refreshScreenClock ();

    mainStackWidget->setCurrentIndex (0);

    // logger
    Logger::instance ()->setMinSeverity (Logger::Debug);
    connect (Logger::instance (), SIGNAL (message (Logger::severity_t, const QString&)), 
             this, SLOT (loggerMessage (Logger::severity_t, const QString&)));

    // connect tool buttons
    connect (configButton, SIGNAL(toggled(bool)), this, SLOT(configButtonToggled(bool)));
    connect (modeButton, SIGNAL(toggled(bool)), this, SLOT(modeButtonToggled(bool)));
    connect (checkButton, SIGNAL(toggled(bool)), this, SLOT(checkButtonToggled(bool)));
    connect (paramsButton, SIGNAL(toggled(bool)), this, SLOT(paramsButtonToggled(bool)));
    connect (sensorsButton, SIGNAL(toggled(bool)), this, SLOT(sensorsButtonToggled(bool)));
    connect (cleanButton, SIGNAL(toggled(bool)), this, SLOT(cleanButtonToggled(bool)));

    connect (globalStopButton,  SIGNAL (clicked ()), this, SLOT (globalStopButtonClicked ()));

    _filterCleaning = false;

    for (int i = 0; i < 4; i++) {
        connect (getStageControl (i), SIGNAL (startPressed (int)), this, SLOT (startButtonClicked (int)));
        connect (getStageControl (i), SIGNAL (stopPressed (int)), this, SLOT (stopButtonClicked (int)));
        connect (getStageControl (i), SIGNAL (targetHumidityUpdated (int, double)), this, SLOT (stageTargetHumidityUpdated (int, double)));
        connect (getStageControl (i), SIGNAL (targetWaterFlowUpdated (int, double)), this, SLOT (stageTargetWaterFlowUpdated (int, double)));
        _stageCleaning[i] = false;
    }

    // connect active stages checkboxes
    connect (applyStagesButton, SIGNAL (clicked ()), this, SLOT (applyStagesButtonClicked ()));

    // hide settings panel
    setSettingsPanelVisible (false);

    // tune stage controls
    stageControl1->setNumber (0);
    stageControl2->setNumber (1);
    stageControl3->setNumber (2);
    stageControl4->setNumber (3);

    // connect button
    connect (connectButton, SIGNAL (doubleClicked ()), this, SLOT (connectButtonClicked ()));

    // view button
    QMenu* connectMenu = new QMenu (this);
    QActionGroup* actionGroup = new QActionGroup (this);
    QAction* action;

    _stages_view = action = new QAction (tr ("Stages"), this);
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

    calibrateGroupBox->hide ();

    // daemon state signals
    connect (&_daemon, SIGNAL (connectedChanged (bool)), this, SLOT (connectedChanged (bool)));
    connect (&_daemon, SIGNAL (hardwareConnected ()), this, SLOT (daemonHardwareConnected ()));
    connect (&_daemon, SIGNAL (textArrived (const QString&)), this, SLOT (daemonTextReceived (const QString&)));
    connect (&_daemon, SIGNAL (autoTextArrived (const QString&)), this, SLOT (daemonAutoTextReceived (const QString&)));
    connect (&_daemon, SIGNAL (commandSent (const QString&)), this, SLOT (daemonCommandSent (const QString&)));
    connect (&_daemon, SIGNAL (stagesActivityChanged (bool,bool,bool,bool)), this, SLOT (daemonStagesActivityChanged (bool,bool,bool,bool)));
    connect (&_daemon, SIGNAL (grainFlowGot (int, int)), this, SLOT (daemonGrainFlowGot (int, int)));
    connect (&_daemon, SIGNAL (stageStarted (int)), this, SLOT (daemonStageStarted (int)));
    connect (&_daemon, SIGNAL (stageStopped (int)), this, SLOT (daemonStageStopped (int)));
    connect (&_daemon, SIGNAL (metaStateGot (double, QMap<int, QList<double> >)), this, SLOT (daemonMetaStateGot (double, QMap<int, QList<double> >)));
    connect (&_daemon, SIGNAL (waterStarted (int)), this, SLOT (daemonWaterStarted (int)));
    connect (&_daemon, SIGNAL (waterStopped (int)), this, SLOT (daemonWaterStopped (int)));
    connect (&_daemon, SIGNAL (grainSensorsPresenceGot (bool)), this, SLOT (daemonGrainSensorsPresenceGot (bool)));
    connect (&_daemon, SIGNAL (grainPresenceGot (int, bool)), this, SLOT (daemonGrainPresenceGot (int, bool)));
    connect (&_daemon, SIGNAL (cleanRequested ()), this, SLOT (daemonCleanRequested ()));
    connect (&_daemon, SIGNAL (cleanStarted ()), this, SLOT (daemonCleanStarted ()));
    connect (&_daemon, SIGNAL (cleanFinished ()), this, SLOT (daemonCleanFinished ()));

    // daemon check loop events
    connect (&_daemon, SIGNAL (stageRunningUpdated (int, bool)), this, SLOT (daemonStageRunningUpdated (int, bool)));
    connect (&_daemon, SIGNAL (waterPressureUpdated (double)), this, SLOT (daemonWaterPressureUpdated (double)));
    connect (&_daemon, SIGNAL (grainPresentUpdated (int, bool)), this, SLOT (daemonGrainPresentUpdated (int, bool)));
    connect (&_daemon, SIGNAL (waterFlowUpdated (int, double)), this, SLOT (daemonWaterFlowUpdated (int, double)));
    connect (&_daemon, SIGNAL (grainFlowUpdated (int, double)), this, SLOT (daemonGrainFlowUpdated (int, double)));
    connect (&_daemon, SIGNAL (grainHumidityUpdated (int, double)), this, SLOT (daemonGrainHumidityUpdated (int, double)));
    connect (&_daemon, SIGNAL (grainTemperatureUpdated (int, double)), this, SLOT (daemonGrainTemperatureUpdated (int, double)));
    connect (&_daemon, SIGNAL (grainNatureUpdated (int, double)), this, SLOT (daemonGrainNatureUpdated (int, double)));
    connect (&_daemon, SIGNAL (calculatedHumidityUpdated (int, double)), this, SLOT (daemonCalculatedHumidityUpdated (int, double)));
    connect (&_daemon, SIGNAL (targetFlowUpdated (int, double)), this, SLOT (daemonTargetFlowUpdated (int, double)));
    connect (&_daemon, SIGNAL (targetSettingUpdated (int, double)), this, SLOT (daemonTargetSettingUpdated (int, double)));
    connect (&_daemon, SIGNAL (settingsGot ()), this, SLOT (daemonSettingsGot ()));
    connect (&_daemon, SIGNAL (calibrateReply (int, const QString&, double)), this, SLOT (daemonCalibrateReply (int, const QString&, double)));
    connect (&_daemon, SIGNAL (bsuPoweredUpdated (int, bool)), this, SLOT (daemonBsuPoweredUpdated (int, bool)));
    connect (&_daemon, SIGNAL (grainLowUpdated (int, bool)), this, SLOT (daemonGrainLowUpdated (int, bool)));
    connect (&_daemon, SIGNAL (autoModeError (error_kind_t, const QString& )), this, SLOT (daemonAutoModeError (error_kind_t, const QString& )));
    connect (&_daemon, SIGNAL (gotCleanState (bool, bool, bool, bool, bool)), this, SLOT (daemonGotCleanState (bool, bool, bool, bool, bool)));
    connect (&_daemon, SIGNAL (gotCleanResult (bool[4], bool[4])), this, SLOT (daemonGotCleanResult (bool[4], bool[4])));
    connect (&_daemon, SIGNAL (minPressureGot (double)), this, SLOT (daemonMinPressureGot (double)));

    connect (checkStateButton, SIGNAL (pressed ()), this, SLOT (checkStateButtonPressed ()));
    connect (checkWaterButton, SIGNAL (pressed ()), this, SLOT (checkWaterButtonPressed ()));
    connect (checkGrainSensorsButton, SIGNAL (pressed ()), this, SLOT (checkGrainSensorsButtonPressed ()));
    connect (stateRefreshButton, SIGNAL (clicked ()), this, SLOT (stateRefreshButtonClicked ()));
    connect (applyCheckWaterButton, SIGNAL (clicked ()), this, SLOT (applyCheckWaterButtonClicked ()));
    connect (grainSensorsEnabledCheck, SIGNAL (toggled (bool)), this, SLOT (grainSensorsEnabledChecked (bool)));
    connect (checkGrainStageApplyButton, SIGNAL (clicked ()), this, SLOT (checkGrainStageApplyClicked ()));

    // console events
    connect (consoleSendButton, SIGNAL (clicked ()), this, SLOT (consoleSendButtonClicked ()));
    connect (sendFileButton, SIGNAL (clicked ()), this, SLOT (sendFileButtonClicked ()));

    // settings
    connect (settingsStageComboBox, SIGNAL (activated (int)), this, SLOT (settingsStageComboActivated (int)));
    connect (applySettingsButton, SIGNAL (clicked ()), this, SLOT (applySettingsButtonClicked ()));

    connect (settingsTargetHumidityEdit, SIGNAL (textEdited (const QString&)), this, SLOT (settingsValueEdited (const QString&)));
    connect (settingsHumidityCoeffEdit, SIGNAL (textEdited (const QString&)), this, SLOT (settingsValueEdited (const QString&)));
    connect (settingsMinGrainFlowEdit, SIGNAL (textEdited (const QString&)), this, SLOT (settingsValueEdited (const QString&)));
    connect (settingsWaterFlowKEdit, SIGNAL (textEdited (const QString&)), this, SLOT (settingsValueEdited (const QString&)));
    connect (settingsMaxWaterFlowEdit, SIGNAL (textEdited (const QString&)), this, SLOT (settingsValueEdited (const QString&)));
    connect (settingsMinWaterFlowEdit, SIGNAL (textEdited (const QString&)), this, SLOT (settingsValueEdited (const QString&)));

    connect (settingsHumidityTableButton, SIGNAL (clicked ()), this, SLOT (settingsHumidityTableClicked ()));
    connect (settingsGrainFlowTableButton, SIGNAL (clicked ()), this, SLOT (settingsGrainFlowTableClicked ()));
    connect (settingsGrainNatureTableButton, SIGNAL (clicked ()), this, SLOT (settingsGrainNatureTableClicked ()));
    connect (settingsGrainTempTableButton, SIGNAL (clicked ()), this, SLOT (settingsGrainTempTableClicked ()));
    connect (settingsGrainNatureCoeffTableButton, SIGNAL (clicked ()), this, SLOT (settingsGrainNatureCoeffTableClicked ()));
    connect (settingsAdvancedGroupBox, SIGNAL (toggled (bool)), this, SLOT (settingsAdvancedGroupBoxChecked (bool)));

    connect (stageSensorsApplyButton, SIGNAL (clicked ()), this, SLOT (stageSensorsApplyButtonClicked ()));
    connect (&_daemon, SIGNAL (tempCoefGot (double, double)), this, SLOT (tempCoefGot (double, double)));
    connect (tempSensorsGroupbox, SIGNAL (toggled (bool)), this, SLOT (tempSensorsGroupboxChecked (bool)));

    connect (calibrateS1Button, SIGNAL (toggled (bool)), this, SLOT (calibrateS1Checked (bool)));
    connect (calibrateS2Button, SIGNAL (toggled (bool)), this, SLOT (calibrateS2Checked (bool)));
    connect (calibrateS3Button, SIGNAL (toggled (bool)), this, SLOT (calibrateS3Checked (bool)));
    connect (calibrateS4Button, SIGNAL (toggled (bool)), this, SLOT (calibrateS4Checked (bool)));

    connect (calibrateButton, SIGNAL (clicked ()), this, SLOT (calibrateButtonClicked ()));

    connect (stageModesApplyButton, SIGNAL (clicked ()), this, SLOT (stageModesApplyButtonClicked ()));

    // history
    historyKindCombo->clear ();
    historyKindCombo->addItem (tr ("Grain flow"), HK_GrainFlow);
    historyKindCombo->addItem (tr ("Grain temperature"), HK_GrainTemp);
    historyKindCombo->addItem (tr ("Grain nature"), HK_GrainNature);
    historyKindCombo->addItem (tr ("Water pressure"), HK_WaterPress);
    historyKindCombo->addItem (tr ("Grain humidity"), HK_GrainHumidity);
    historyKindCombo->addItem (tr ("Water flow"), HK_WaterFlow);
    historyKindCombo->addItem (tr ("Setting"), HK_Setting);

    connect (historyStageCombo, SIGNAL (activated (int)), this, SLOT (historyItemChanged (int)));
    connect (refreshHistoryButton, SIGNAL (clicked ()), this, SLOT (refreshHistoryButtonClicked ()));
    connect (historyPeriodCombo, SIGNAL (activated (int)), this, SLOT (historyPeriodComboChanged (int)));
    connect (&_daemon, SIGNAL (historyGot (const QList < QPair <uint, double> >&, history_stage_t, history_kind_t)), 
             this, SLOT (historyGot (const QList < QPair <uint, double> >&, history_stage_t, history_kind_t)));
    connect (&_daemon, SIGNAL (eventsGot (const QList < QPair <uint, QString> >&)), 
             this, SLOT (eventsGot (const QList < QPair <uint, QString> >&)));
    historyPeriodComboChanged (0);

    plot->setAxisScaleDraw (QwtPlot::xBottom, new PlotScaleDraw (false));

    // cleaning
    connect (cleanFilterButton, SIGNAL (clicked ()), this, SLOT (cleanFilterButtonClicked ()));
    connect (cleanStagesButton, SIGNAL (clicked ()), this, SLOT (cleanStagesButtonClicked ()));
    connect (drainWaterButton, SIGNAL (clicked ()), this, SLOT (drainWaterButtonClicked ()));
    connect (&_daemon, SIGNAL (drainStarted ()), this, SLOT (daemonDrainStarted ()));
    connect (&_daemon, SIGNAL (drainFinished ()), this, SLOT (daemonDrainFinished ()));
}


void MainWindow::setSettingsPanelVisible (bool visible)
{
    if (visible) {
        settingsPanel->show ();
        stageControl3->hide ();
        stageControl4->hide ();
    }
    else {
        settingsPanel->hide ();
        stageControl3->show ();
        stageControl4->show ();
    }
}


void MainWindow::timerEvent (QTimerEvent* e)
{
    if (e->timerId () == _clockTimer)
        refreshScreenClock ();

    // request cleaning state
    if (e->timerId () == _cleaningWatchdogTimer)
        _daemon.getCleanState ();
}


void MainWindow::refreshScreenClock ()
{
    dateAndTimeLabel->setText (QDateTime::currentDateTime ().toString ("<b>hh:mm:ss</b>"));
}


void MainWindow::configButtonToggled (bool on)
{
    if (on) {
        _switchingToolButtons = true;
        modeButton->setChecked (false);
        checkButton->setChecked (false);
        paramsButton->setChecked (false);
        sensorsButton->setChecked (false);
        cleanButton->setChecked (false);
        _switchingToolButtons = false;
	stackedWidget->setCurrentIndex (0);
        setSettingsPanelVisible (true);
    }
    else
        if (!_switchingToolButtons)
            setSettingsPanelVisible (false);
}


void MainWindow::modeButtonToggled (bool on)
{
    if (on) {
        _switchingToolButtons = true;
        configButton->setChecked (false);
        checkButton->setChecked (false);
        paramsButton->setChecked (false);
        sensorsButton->setChecked (false);
        cleanButton->setChecked (false);
        _switchingToolButtons = false;
	stackedWidget->setCurrentIndex (1);
        setSettingsPanelVisible (true);
    }
    else
        if (!_switchingToolButtons)
            setSettingsPanelVisible (false);
}


void MainWindow::checkButtonToggled (bool on)
{
    if (on) {
        _switchingToolButtons = true;
        configButton->setChecked (false);
        modeButton->setChecked (false);
        paramsButton->setChecked (false);
        sensorsButton->setChecked (false);
        cleanButton->setChecked (false);
        _switchingToolButtons = false;
	stackedWidget->setCurrentIndex (2);
        setSettingsPanelVisible (true);
    }
    else
        if (!_switchingToolButtons)
            setSettingsPanelVisible (false);
}


void MainWindow::paramsButtonToggled (bool on)
{
    if (on) {
        _switchingToolButtons = true;
        configButton->setChecked (false);
        checkButton->setChecked (false);
        modeButton->setChecked (false);
        sensorsButton->setChecked (false);
        cleanButton->setChecked (false);
        _switchingToolButtons = false;
	stackedWidget->setCurrentIndex (3);
        setSettingsPanelVisible (true);
    }
    else
        if (!_switchingToolButtons)
            setSettingsPanelVisible (false);
}


void MainWindow::sensorsButtonToggled (bool on)
{
    if (on) {
        _switchingToolButtons = true;
        configButton->setChecked (false);
        modeButton->setChecked (false);
        checkButton->setChecked (false);
        paramsButton->setChecked (false);
        cleanButton->setChecked (false);
        _switchingToolButtons = false;
	stackedWidget->setCurrentIndex (4);
        setSettingsPanelVisible (true);
    }
    else
        if (!_switchingToolButtons)
            setSettingsPanelVisible (false);
}


void MainWindow::cleanButtonToggled (bool on)
{
    if (on) {
        _switchingToolButtons = true;
        configButton->setChecked (false);
        modeButton->setChecked (false);
        checkButton->setChecked (false);
        paramsButton->setChecked (false);
        sensorsButton->setChecked (false);
        _switchingToolButtons = false;
	stackedWidget->setCurrentIndex (5);
        setSettingsPanelVisible (true);
    }
    else
        if (!_switchingToolButtons)
            setSettingsPanelVisible (false);
}


void MainWindow::loggerMessage (Logger::severity_t severity, const QString& msg)
{
    QListWidgetItem* item = new QListWidgetItem (QDateTime::currentDateTime ().toString ("hh:mm:ss ") + msg);
    QBrush br;

    switch (severity) {
    case Logger::Debug:
        br = QBrush (Qt::gray);
        break;
    case Logger::Information:
        br = QBrush (Qt::black);
        break;
    case Logger::Warning:
        br = QBrush (Qt::blue);
        break;
    case Logger::Error:
        br = QBrush (Qt::red);
        break;
    }

    item->setForeground (br);
    logListView->insertItem (0, item);
    _daemon.logMessage (msg);
}


StageControl* MainWindow::getStageControl (int stage) const
{
    switch (stage) {
    case 0:
        return stageControl1;
    case 1:
        return stageControl2;
    case 2:
        return stageControl3;
    case 3:
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
    if (_daemon.connected ()) {
        _daemon.disconnect ();
        Logger::instance ()->log (Logger::Information, tr ("Disconnecting from daemon"));
    }
    else {
        _daemon.connect ();
        Logger::instance ()->log (Logger::Information, tr ("Connecting to daemon"));
    }
}


// daemon signals handlers
void MainWindow::connectedChanged (bool value)
{
    // toggle connect button
    connectButton->setChecked (value);

    QPushButton* btns[] = {configButton, checkButton, modeButton, paramsButton, sensorsButton, cleanButton};

    for (uint i = 0; i < sizeof (btns) / sizeof (btns[0]); i++) {
        btns[i]->setEnabled (value);
        btns[i]->setChecked (false);
    }

    if (!value)
        daemonStagesActivityChanged (false, false, false, false);
    Logger::instance ()->log (Logger::Information, value ? tr ("Connected to daemon, obtaining state") : tr ("Disconnected from daemon"));
}


// hardware connection got, fetch daemon state
void MainWindow::daemonHardwareConnected ()
{
    // enabled stages
    _daemon.getStages ();
    _daemon.getMinPressure ();

    // grain sensors presense
    _daemon.requestTempCoef ();
    _daemon.isGrainSensorsPresent ();
    _daemon.requestSettings ();
    _daemon.getCleanState ();
    _daemon.checkTick ();
    Logger::instance ()->log (Logger::Information, tr ("Connection to controller established"));
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
            _daemon.sendRawCommand (consoleCommandInput->text () + "\n");
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
    if (haveAccess (AL_Admin))
        mainStackWidget->setCurrentIndex (2);
    else
        _stages_view->setChecked (true);
}


void MainWindow::applyStagesButtonClicked ()
{
    double minPress;
    bool ok;

    minPress = minPressureEdit->text ().toDouble (&ok);
    
    if (!ok) {
        QMessageBox::warning (this, tr ("Invalid value"), tr ("minimal pressure is invalid"));
        return;
    }

    _daemon.setMinPressure (minPress);
    _daemon.setStages (stage1ActiveCheckBox->isChecked (), stage2ActiveCheckBox->isChecked (), 
                       stage3ActiveCheckBox->isChecked (), stage4ActiveCheckBox->isChecked ());
    _daemon.requestSettings ();

    QString stg = tr ("stage"), msg, en = tr ("enabled", "stage"), dis = tr ("disabled", "stage");
    
    msg = tr ("Changing stages activity: ");

    msg += stg + QString (" 1 ") + (stage1ActiveCheckBox->isChecked () ? en : dis) + QString (", ");
    msg += stg + QString (" 2 ") + (stage2ActiveCheckBox->isChecked () ? en : dis) + QString (", ");
    msg += stg + QString (" 3 ") + (stage3ActiveCheckBox->isChecked () ? en : dis) + QString (", ");
    msg += stg + QString (" 4 ") + (stage4ActiveCheckBox->isChecked () ? en : dis);

    Logger::instance ()->log (Logger::Information, msg);

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

    checkGrainStage1Check->setEnabled (s1);
    checkGrainStage2Check->setEnabled (s2);
    checkGrainStage3Check->setEnabled (s3);
    checkGrainStage4Check->setEnabled (s4);

    stage1ActiveCheckBox->setChecked (s1);
    stage2ActiveCheckBox->setChecked (s2);
    stage3ActiveCheckBox->setChecked (s3);
    stage4ActiveCheckBox->setChecked (s4);

    // settings combo box
    settingsStageComboBox->clear ();
    if (s1)
        settingsStageComboBox->addItem (tr ("Stage 1"));
    if (s2)
        settingsStageComboBox->addItem (tr ("Stage 2"));
    if (s3)
        settingsStageComboBox->addItem (tr ("Stage 3"));
    if (s4)
        settingsStageComboBox->addItem (tr ("Stage 4"));
    settingsStageComboBox->setCurrentIndex (0);

    // sensors page
    stage1SensorsCheckbox->setEnabled (s1);
    stage1SensorsCheckbox->setChecked (s1);
    stage2SensorsCheckbox->setEnabled (s2);
    stage2SensorsCheckbox->setChecked (s2);
    stage3SensorsCheckbox->setEnabled (s3);
    stage3SensorsCheckbox->setChecked (s3);
    stage4SensorsCheckbox->setEnabled (s4);
    stage4SensorsCheckbox->setChecked (s4);

    calibrateS1Button->setEnabled (s1);
    calibrateS2Button->setEnabled (s2);
    calibrateS3Button->setEnabled (s3);
    calibrateS4Button->setEnabled (s4);

    // stage modes
    s1modeLabel->setEnabled (s1);
    s1modeCombo->setEnabled (s1);
    s2modeLabel->setEnabled (s2);
    s2modeCombo->setEnabled (s2);
    s3modeLabel->setEnabled (s3);
    s3modeCombo->setEnabled (s3);
    s4modeLabel->setEnabled (s4);
    s4modeCombo->setEnabled (s4);

    bool anyStageActive = s1 | s2 | s3 | s4;

    QPushButton* btns[] = {checkButton, modeButton, paramsButton, sensorsButton};

    for (uint i = 0; i < sizeof (btns) / sizeof (btns[0]); i++) {
        btns[i]->setEnabled (anyStageActive);
        btns[i]->setChecked (false);
    }

    // history combo box
    historyStageCombo->clear ();
    
    if (s1)
        historyStageCombo->addItem (tr ("Stage 1"), HS_Stage1);
    if (s2)
        historyStageCombo->addItem (tr ("Stage 2"), HS_Stage2);
    if (s3)
        historyStageCombo->addItem (tr ("Stage 3"), HS_Stage3);
    if (s4)
        historyStageCombo->addItem (tr ("Stage 4"), HS_Stage4);

    historyStageCombo->addItem (tr ("Events"), HS_Events);
    historyStageCombo->addItem (tr ("Cleanings"), HS_Cleanings);

    // clean
    cleanS1Check->setEnabled (s1);
    cleanS2Check->setEnabled (s2);
    cleanS3Check->setEnabled (s3);
    cleanS4Check->setEnabled (s4);

    drainS1Check->setEnabled (s1);
    drainS2Check->setEnabled (s2);
    drainS3Check->setEnabled (s3);
    drainS4Check->setEnabled (s4);
}


void MainWindow::daemonGrainFlowGot (int stage, int value)
{
    getStageControl (stage)->setGrainFlow (value);
}


void MainWindow::startButtonClicked (int stage)
{
    _daemon.startStage (stage);
    Logger::instance ()->log (Logger::Information, tr ("Request start of stage %1").arg (stage+1));
}


void MainWindow::stopButtonClicked (int stage)
{
    _daemon.stopStage (stage);
    Logger::instance ()->log (Logger::Information, tr ("Request stop of stage %1").arg (stage+1));
}


void MainWindow::globalStopButtonClicked ()
{
    for (int i = 0; i < 4; i++)
        if (getStageControl (i)->running ()) {
            _daemon.stopStage (i);
            Logger::instance ()->log (Logger::Information, tr ("Request stop of stage %1").arg (i+1));
        }
    globalStopButton->setEnabled (false);
}


void MainWindow::daemonMetaStateGot (double water_pres, QMap<int, QList<double> > vals)
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

	int s = vals[i].size ();

        QTreeWidgetItem* item = new QTreeWidgetItem (stateTreeWidget);
        item->setExpanded (true);
        item->setText (0, tr ("Stage %1").arg (i));
        
	if (s > 0) {
		n = new QTreeWidgetItem (item);
        	n->setText (0, tr ("Water flow"));
        	n->setText (1, QString::number (vals[i][0]));
        }

	if (s > 1) {
        	n = new QTreeWidgetItem (item);
        	n->setText (0, tr ("Grain flow"));
        	n->setText (1, QString::number (vals[i][1]));
        }

	if (s > 2) {
        	n = new QTreeWidgetItem (item);
        	n->setText (0, tr ("Grain humidity"));
        	n->setText (1, QString::number (vals[i][2]));
        }

	if (s > 3) {
        	n = new QTreeWidgetItem (item);
        	n->setText (0, tr ("Grain temp."));
        	n->setText (1, QString::number (vals[i][3]));
        }

	if (s > 4) {
        	n = new QTreeWidgetItem (item);
        	n->setText (0, tr ("Grain nature"));
        	n->setText (1, QString::number (vals[i][4]));
	}
    }
}


void MainWindow::daemonWaterStarted (int stage)
{
    Logger::instance ()->log (Logger::Information, tr ("Water started on %1 stage").arg (stage+1));
    getStageControl (stage)->setWaterPresent (true);
}


void MainWindow::daemonWaterStopped (int stage)
{
    Logger::instance ()->log (Logger::Information, tr ("Water stopped on %1 stage").arg (stage+1));
    getStageControl (stage)->setWaterPresent (false);
}


void MainWindow::stateRefreshButtonClicked ()
{
    _daemon.refreshState ();
    Logger::instance ()->log (Logger::Information, tr ("Fetching state for active stages"));
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
    QCheckBox* boxes[] = { checkWaterStage1Check, checkWaterStage2Check, checkWaterStage3Check, checkWaterStage4Check };

    for (int i = 0; i < 4; i++)
        if (_daemon.isStageEnabled (i) && !getStageControl (i)->running ())
            if (boxes[i]->isChecked ()) {
                _daemon.startWater (i);
            }
            else
                _daemon.stopWater (i);
}


void MainWindow::sendFileButtonClicked ()
{
    QString file = QFileDialog::getOpenFileName (this, tr ("Open script"), QString (), tr ("Scripts (*.txt)"));

    QFile f (file);
    QTextStream s (&f);

    if (file.isNull ())
        return;

    if (!f.open (QIODevice::ReadOnly)) 
        Logger::instance ()->log (Logger::Error, tr ("Cannot open script file '%1' for reading.").arg (file));
    else {
        while (!s.atEnd ())
            _daemon.sendRawCommand (s.readLine () + "\n");
    }
}


void MainWindow::daemonGrainSensorsPresenceGot (bool value)
{
    grainSensorsEnabledCheck->setChecked (value);
    checkGrainSensorsButton->setEnabled (value);
    if (!value && checkGrainSensorsButton->isChecked ()) {
        checkStateButton->setChecked (true);
        checkStackedWidget->setCurrentIndex (0);
    }
}


void MainWindow::grainSensorsEnabledChecked (bool val)
{
    _daemon.setGrainSensorsEnabled (val);
    Logger::instance ()->log (Logger::Information, tr ("%1 grain sensors").arg (val ? tr ("Enabling", "grain sensors") : tr ("Disabling", "grain sensors")));   
}


void MainWindow::checkGrainStageApplyClicked ()
{
    if (checkGrainStage1Check->isChecked ())
        _daemon.isGrainPresent (0);
    if (checkGrainStage2Check->isChecked ())
        _daemon.isGrainPresent (1);
    if (checkGrainStage3Check->isChecked ())
        _daemon.isGrainPresent (2);
    if (checkGrainStage4Check->isChecked ())
        _daemon.isGrainPresent (3);

    checkGrainStage1Label->setText (QString ());
    checkGrainStage2Label->setText (QString ());
    checkGrainStage3Label->setText (QString ());
    checkGrainStage4Label->setText (QString ());
    Logger::instance ()->log (Logger::Information, tr ("Checking grain presense in selected stages"));   
}


void MainWindow::daemonGrainPresenceGot (int stage, bool value)
{
    QString text (value ? tr ("Present") : tr ("Not present"));

    Logger::instance ()->log (value ? Logger::Information : Logger::Error, value ? tr ("There are grain in stage %1").arg (stage+1) : tr ("There are no grain in stage %1").arg (stage+1));    

    switch (stage) {
    case 0:
        checkGrainStage1Label->setText (text);
        break;
    case 1:
        checkGrainStage2Label->setText (text);
        break;
    case 2:
        checkGrainStage3Label->setText (text);
        break;
    case 3:
        checkGrainStage4Label->setText (text);
        break;
    }
}


void MainWindow::daemonWaterPressureUpdated (double val)
{
    bsuControl->setWaterPressure (val);
}


void MainWindow::daemonGrainPresentUpdated (int stage, bool present)
{
    getStageControl (stage)->setGrainState (present ? StageControl::GS_GrainPresent : StageControl::GS_GrainMissing);
}


void MainWindow::daemonWaterFlowUpdated (int stage, double val)
{
    if (val < 10 && getStageControl (stage)->running ())
        Logger::instance ()->log (Logger::Error, tr ("Water flow too low at stage %1. Stage stopped.").arg (stage+1));
    getStageControl (stage)->setWaterFlow (val);
}


void MainWindow::daemonGrainFlowUpdated (int stage, double val)
{
    getStageControl (stage)->setGrainFlow (val);
}


void MainWindow::daemonGrainHumidityUpdated (int stage, double val)
{
    getStageControl (stage)->setHumidity (val);
}


void MainWindow::daemonGrainTemperatureUpdated (int stage, double val)
{
    getStageControl (stage)->setTemperature (val);
}


void MainWindow::daemonGrainNatureUpdated (int stage, double val)
{
    getStageControl (stage)->setNature (val);
}


void MainWindow::daemonCalculatedHumidityUpdated (int stage, double val)
{
    getStageControl (stage)->setTargetHumidity (val);
}


void MainWindow::daemonTargetFlowUpdated (int stage, double val)
{
   getStageControl (stage)->setTargetWaterFlow (val);
}


void MainWindow::daemonTargetSettingUpdated (int stage, double val)
{
    getStageControl (stage)->setSetting (val);
}


void MainWindow::daemonSettingsGot ()
{
    settingsStageComboActivated (settingsStageComboBox->currentIndex ());

    if (stage1SensorsCheckbox->isEnabled ())
        stage1SensorsCheckbox->setChecked (_daemon.getSettings (0).sensors ());
    else
        stage1SensorsCheckbox->setChecked (false);

    if (stage2SensorsCheckbox->isEnabled ())
        stage2SensorsCheckbox->setChecked (_daemon.getSettings (1).sensors ());
    else
        stage2SensorsCheckbox->setChecked (false);

    if (stage3SensorsCheckbox->isEnabled ())
        stage3SensorsCheckbox->setChecked (_daemon.getSettings (2).sensors ());
    else
        stage3SensorsCheckbox->setChecked (false);

    if (stage4SensorsCheckbox->isEnabled ())
        stage4SensorsCheckbox->setChecked (_daemon.getSettings (3).sensors ());
    else
        stage4SensorsCheckbox->setChecked (false);

    s1modeCombo->setCurrentIndex (_daemon.getSettings (0).mode ());
    s2modeCombo->setCurrentIndex (_daemon.getSettings (1).mode ());
    s3modeCombo->setCurrentIndex (_daemon.getSettings (2).mode ());
    s4modeCombo->setCurrentIndex (_daemon.getSettings (3).mode ());

    for (int i = 0; i < 4; i++) {
        getStageControl (i)->setSensors (_daemon.getSettings (i).sensors ());
        getStageControl (i)->setLabel (_daemon.getSettings (i).bsuLabel ());
        getStageControl (i)->setMode (_daemon.getSettings (i).mode ());
        getStageControl (i)->setTargetHumidity (_daemon.getSettings (i).targetHumidity ());
        getStageControl (i)->setMinMaxWaterFlow (_daemon.getSettings (i).minWaterFlow (), 
                                                 _daemon.getSettings (i).maxWaterFlow ());
    }
}


bool MainWindow::haveActiveStages () const
{
    for (int i = 0; i < 4; i++)
        if (getStageControl (i)->running ())
            return true;
    return false;
}


void MainWindow::settingsStageComboActivated (int item)
{
    StageSettings sett = _daemon.getSettings (item);

    if (_settingsChanged) {
        if (QMessageBox::question (this, tr ("Settings changed warning"), 
                                   tr ("Your settings changes have not applied. Do you want to save them?"),
                                   QMessageBox::Yes | QMessageBox::No) == QMessageBox::Ok)
            saveSettingsPage (_currentSettingsStage);
        _settingsChanged = false;
    }

    settingsTargetHumidityEdit->setText (QString::number (sett.targetHumidity (), 'f', 1));
    settingsFixedHumidityEdit->setText (QString::number (sett.fixedHumidity ()));
    settingsHumidityCoeffEdit->setText (QString::number (sett.humidityCoeff ()));
    settingsMinGrainFlowEdit->setText (QString::number (sett.minGrainFlow ()));
    settingsWaterFlowKEdit->setText (QString::number (sett.waterFlowK ()));
    settingsMaxWaterFlowEdit->setText (QString::number (sett.maxWaterFlow ()));
    settingsMinWaterFlowEdit->setText (QString::number (sett.minWaterFlow ()));
    settingsWaterFormulaComboBox->setCurrentIndex (sett.waterFormula ());
    _humidityTable = sett.humidityTable ();
    _grainFlowTable = sett.grainFlowTable ();
    _grainNatureTable = sett.grainNatureTable ();
    _grainTempTable = sett.grainTempTable ();
    _grainNatureCoeffTable = sett.grainNatureCoeffTable ();
    bsuLabelEdit->setText (sett.bsuLabel ());
    _currentSettingsStage = item;
}


void MainWindow::applySettingsButtonClicked ()
{
    saveSettingsPage (settingsStageComboBox->currentIndex ());
}


void MainWindow::saveSettingsPage (int stage)
{
    StageSettings sett = _daemon.getSettings (stage);
    bool ok;
    double val;

    val = settingsTargetHumidityEdit->text ().toDouble (&ok);
    if (ok) {
        sett.setTargetHumidity (val);
        getStageControl (stage)->setTargetHumidity (val);
    }
    else {
        QMessageBox::warning (this, tr ("Value error"), tr ("Target humidity have invalid format"));
        return;
    }

    val = settingsFixedHumidityEdit->text ().toDouble (&ok);
    if (ok) {
        sett.setFixedHumidity (val);
    }
    else {
        QMessageBox::warning (this, tr ("Value error"), tr ("Fixed humidity have invalid format"));
        return;
    }

    val = settingsHumidityCoeffEdit->text ().toDouble (&ok);
    if (ok)
        sett.setHumidityCoeff (val);
    else {
        QMessageBox::warning (this, tr ("Value error"), tr ("Humidity coefficient have invalid format"));
        return;
    }

    val = settingsMinGrainFlowEdit->text ().toDouble (&ok);
    if (ok)
        sett.setMinGrainFlow (val);
    else {
        QMessageBox::warning (this, tr ("Value error"), tr ("Min grain flow have invalid format"));
        return;
    }

    val = settingsWaterFlowKEdit->text ().toDouble (&ok);
    if (ok)
        sett.setWaterFlowK (val);
    else {
        QMessageBox::warning (this, tr ("Value error"), tr ("Water flow coefficient have invalid format"));
        return;
    }

    val = settingsMaxWaterFlowEdit->text ().toDouble (&ok);
    if (ok)
        sett.setMaxWaterFlow (val);
    else {
        QMessageBox::warning (this, tr ("Value error"), tr ("Max water flow have invalid format"));
        return;
    }

    val = settingsMinWaterFlowEdit->text ().toDouble (&ok);
    if (ok)
        sett.setMinWaterFlow (val);
    else {
        QMessageBox::warning (this, tr ("Value error"), tr ("Min water flow have invalid format"));
        return;
    }

    if (bsuLabelEdit->text ().contains (',') || bsuLabelEdit->text ().contains ('=') || bsuLabelEdit->text ().contains (' ')) {
        QMessageBox::warning (this, tr ("Value error"), tr ("BSU label cannot contain spaces, comma and equal sign"));
        return;
    }
    else {
        sett.setBsuLabel (bsuLabelEdit->text ());
        getStageControl (stage)->setLabel (sett.bsuLabel ());
    }

    sett.setWaterFormula (settingsWaterFormulaComboBox->currentIndex ());
    sett.setHumidityTable (_humidityTable);
    sett.setGrainFlowTable (_grainFlowTable);
    sett.setGrainNatureTable (_grainNatureTable);
    sett.setGrainTempTable (_grainTempTable);
    sett.setGrainNatureCoeffTable (_grainNatureCoeffTable);

    sett.setValid (true);
    _daemon.setSettings (stage, sett);
    _settingsChanged = false;
    Logger::instance ()->log (Logger::Information, tr ("Your setting for stage %1 saved and now active").arg (stage+1));

    getStageControl (stage)->setMinMaxWaterFlow (sett.minWaterFlow (), sett.maxWaterFlow ());
}



void MainWindow::settingsValueEdited (const QString&)
{
    _settingsChanged = true;
}


void MainWindow::settingsHumidityTableClicked ()
{
    TableForm dlg (this, tr ("Humidity table editor"), tr ("Key (0..255) -> Humidity map"), tr ("Value"), tr ("Humidity"), true);

    dlg.setData (_humidityTable);

    if (dlg.exec () == QDialog::Rejected)
        return;

    _settingsChanged = true;
    _humidityTable = dlg.result ();
}


void MainWindow::settingsGrainFlowTableClicked ()
{
    TableForm dlg (this, tr ("Grain flow table editor"), tr ("Key (0..255) -> Grain flow map"), tr ("Value"), tr ("Grain flow"), true);

    dlg.setData (_grainFlowTable);

    if (dlg.exec () == QDialog::Rejected)
        return;

    _settingsChanged = true;
    _grainFlowTable = dlg.result ();
}


void MainWindow::settingsGrainNatureTableClicked ()
{
    TableForm dlg (this, tr ("Grain nature table editor"), tr ("Key (0..255) -> Grain nature map"), tr ("Value"), tr ("Grain nature"), true);

    dlg.setData (_grainNatureTable);

    if (dlg.exec () == QDialog::Rejected)
        return;

    _settingsChanged = true;
    _grainNatureTable = dlg.result ();
}


void MainWindow::settingsGrainTempTableClicked ()
{
    TableForm dlg (this, tr ("Grain temperature table editor"), tr ("Temperature -> Grain temperature coeff map"), 
                   tr ("Temperature"), tr ("Grain temperature coeff"), false);

    dlg.setData (_grainTempTable);

    if (dlg.exec () == QDialog::Rejected)
        return;

    _settingsChanged = true;
    _grainTempTable = dlg.result ();
}


void MainWindow::settingsGrainNatureCoeffTableClicked ()
{
    TableForm dlg (this, tr ("Grain nature coeff table editor"), tr ("Temperature -> Grain nature coeff map"), 
                   tr ("Temperature"), tr ("Grain nature coeff"), false);

    dlg.setData (_grainNatureCoeffTable);

    if (dlg.exec () == QDialog::Rejected)
        return;

    _settingsChanged = true;
    _grainNatureCoeffTable = dlg.result ();
}


bool MainWindow::haveAccess (access_level_t level)
{
    if (level <= _access)
        return true;

    // ask for password
    QString msg, user;
    
    switch (level) {
    case AL_Config:
        msg = tr ("Enter configurator password");
        user = "config";
        break;
    case AL_Admin:
        msg = tr ("Enter administrator password");
        user = "admin";
        break;
    default:
        return false;
    }

    bool ok;
    QString pass = QInputDialog::getText (this, tr ("Password dialog"), msg, QLineEdit::Password, QString (), &ok);

    if (!ok)
        return false;

    if (!_daemon.checkPass (user, pass)) {
        QMessageBox::warning (this, tr ("Access denied"), tr ("Invalid password"));
        Logger::instance ()->log (Logger::Information, tr ("Access to %1 account not granted").arg (user));
        return false;
    }
    else {
        _access = level;
        Logger::instance ()->log (Logger::Information, tr ("Access to %1 account have granted").arg (user));
        return true;
    }
}


void MainWindow::settingsAdvancedGroupBoxChecked (bool on)
{
    if (!on)
        return;

    if (!haveAccess (AL_Config))
        settingsAdvancedGroupBox->setChecked (false);
}


void MainWindow::tempSensorsGroupboxChecked (bool on)
{
    if (!on)
        return;

    if (!haveAccess (AL_Config))
        tempSensorsGroupbox->setChecked (false);
}



void MainWindow::stageSensorsApplyButtonClicked ()
{
    _daemon.setSensors (stage1SensorsCheckbox->isChecked (), stage2SensorsCheckbox->isChecked (), 
                        stage3SensorsCheckbox->isChecked (), stage4SensorsCheckbox->isChecked ());

    getStageControl (0)->setSensors (stage1SensorsCheckbox->isChecked ());
    getStageControl (1)->setSensors (stage2SensorsCheckbox->isChecked ());
    getStageControl (2)->setSensors (stage3SensorsCheckbox->isChecked ());
    getStageControl (3)->setSensors (stage4SensorsCheckbox->isChecked ());

    double k, resist;
    bool ok;

    k = tempKEdit->text ().toDouble (&ok);
    
    if (ok)
        resist = tempResistorEdit->text ().toDouble (&ok);

    if (!ok)
        QMessageBox::warning (this, tr ("Values error"), tr ("Value you've entered is not valid"));
    else
        _daemon.setTempCoef (k, resist);
    Logger::instance ()->log (Logger::Information, tr ("Sensors settings saved and now active"));
}


void MainWindow::tempCoefGot (double k, double resist)
{
    tempKEdit->setText (QString::number (k));
    tempResistorEdit->setText (QString::number (resist));
}


void MainWindow::calibrateS1Checked (bool on)
{
    if (!on)
        calibrateGroupBox->hide ();
    else {
        if (haveAccess (AL_Config)) {
            _calibrateStage = 0;
            calibrateGroupBox->show ();
            calibrateGroupBox->setTitle (tr ("Calibrate stage %1").arg (_calibrateStage+1));
            calibrateAnswerEdit->clear ();
        }
    }
}


void MainWindow::calibrateS2Checked (bool on)
{
    if (!on)
        calibrateGroupBox->hide ();
    else {
        if (haveAccess (AL_Config)) {
            _calibrateStage = 1;
            calibrateGroupBox->show ();
            calibrateGroupBox->setTitle (tr ("Calibrate stage %1").arg (_calibrateStage+1));
            calibrateAnswerEdit->clear ();
        }
    }
}


void MainWindow::calibrateS3Checked (bool on)
{
    if (!on)
        calibrateGroupBox->hide ();
    else {
        if (haveAccess (AL_Config)) {
            _calibrateStage = 2;
            calibrateGroupBox->show ();
            calibrateGroupBox->setTitle (tr ("Calibrate stage %1").arg (_calibrateStage+1));
            calibrateAnswerEdit->clear ();
        }
    }
}


void MainWindow::calibrateS4Checked (bool on)
{
    if (!on)
        calibrateGroupBox->hide ();
    else {
        if (haveAccess (AL_Config)) {
            _calibrateStage = 3;
            calibrateGroupBox->show ();
            calibrateGroupBox->setTitle (tr ("Calibrate stage %1").arg (_calibrateStage+1));
            calibrateAnswerEdit->clear ();
        }
    }
}


void MainWindow::calibrateButtonClicked ()
{
    QString sensorKey;

    switch (calibrateSensorComboBox->currentIndex ()) {
    case 0:                     // humidity
        sensorKey = "hum";
        break;
    case 1:                     // grain flow
        sensorKey = "gf";
        break;
    case 2:                     // nature sensor
        sensorKey = "nat";
        break;
    default:
        return;
    }

    _daemon.calibrate (_calibrateStage, sensorKey);
    Logger::instance ()->log (Logger::Information, tr ("Calibration of sensor '%1' on stage %2 started").
                              arg (calibrateSensorComboBox->currentText (), QString::number (_calibrateStage+1)));
}


void MainWindow::daemonCalibrateReply (int stage, const QString&, double val)
{
    Logger::instance ()->log (Logger::Information, tr ("Calibration of sensor '%1' on stage %2 finished. Value = %3").
                              arg (calibrateSensorComboBox->currentText (), QString::number (stage+1), QString::number (val)));
    calibrateAnswerEdit->setText (QString::number (val));
}


void MainWindow::stageModesApplyButtonClicked ()
{
    QString modes[] = { tr ("Auto"), tr ("Semi-auto"), tr ("Fixed humidity") };

    _daemon.setStageModes ((StageSettings::mode_t)s1modeCombo->currentIndex (), (StageSettings::mode_t)s2modeCombo->currentIndex (), 
                           (StageSettings::mode_t)s3modeCombo->currentIndex (), (StageSettings::mode_t)s4modeCombo->currentIndex ());
    Logger::instance ()->log (Logger::Information, tr ("Request new stages modes: %1, %2, %3, %4")
                              .arg (modes[s1modeCombo->currentIndex ()])
                              .arg (modes[s2modeCombo->currentIndex ()])
                              .arg (modes[s3modeCombo->currentIndex ()])
                              .arg (modes[s4modeCombo->currentIndex ()]));
    stageControl1->setMode ((StageSettings::mode_t)s1modeCombo->currentIndex ());
    stageControl2->setMode ((StageSettings::mode_t)s2modeCombo->currentIndex ());
    stageControl3->setMode ((StageSettings::mode_t)s3modeCombo->currentIndex ());
    stageControl4->setMode ((StageSettings::mode_t)s4modeCombo->currentIndex ());
}


void MainWindow::historyPeriodComboChanged (int)
{
    graphResetButtonClicked ();
}


void MainWindow::refreshHistoryButtonClicked ()
{
    history_stage_t stage = (history_stage_t)historyStageCombo->itemData (historyStageCombo->currentIndex ()).toInt ();
    history_kind_t kind = (history_kind_t)historyKindCombo->itemData (historyKindCombo->currentIndex ()).toInt ();
    QDateTime from = historyFromDateEdit->dateTime ();
    QDateTime to;
    int period = historyPeriodCombo->currentIndex ();
    bool need_date = false;
    
    switch (period) {
    case 0:
        to = from.addSecs (60*60);
        break;
    case 1:
        to = from.addDays (1);
        break;
    case 2:
        to = from.addDays (7);
        need_date = true;
        break;
    case 3:
        to = from.addDays (30);
        need_date = true;
        break;
    case 4:
        to = from.addYears (1);
        need_date = true;
        break;
    }

    PlotScaleDraw* sd = static_cast<PlotScaleDraw*> (plot->axisScaleDraw (QwtPlot::xBottom));

    if (sd)
        sd->setShowDate (need_date);

    historyTable->clear ();

    switch (stage) {
    case HS_Events:
        _daemon.requestEvents (from.toTime_t (), to.toTime_t (), false);
        break;
    case HS_Cleanings:
        _daemon.requestEvents (from.toTime_t (), to.toTime_t (), true);
        break;
    default:
        _daemon.requestHistory (stage, kind, from.toTime_t (), to.toTime_t ());
        break;
    }
}


void MainWindow::graphResetButtonClicked ()
{
    QDateTime from = QDateTime::currentDateTime ();

    switch (historyPeriodCombo->currentIndex ()) {
    case 0:
        from = from.addSecs (-60*60);
        break;
    case 1:
        from = from.addDays (-1);
        break;
    case 2:
        from = from.addDays (-7);
        break;
    case 3:
        from = from.addDays (-30);
        break;
    case 4:
        from = from.addYears (-1);
        break;
    }

    historyFromDateEdit->setDateTime (from);
}



void MainWindow::historyGot (const QList < QPair <uint, double> >& data, history_stage_t stage, history_kind_t kind)
{
    QStringList l;
    double *x, *y;

    x = (double*)malloc (sizeof (double) * data.size ());
    y = (double*)malloc (sizeof (double) * data.size ());

    historyTable->clear ();

    for (int i = 0; i < data.size (); i++) {
        l.clear ();
        l.append (QDateTime::fromTime_t (data[i].first).toString ("dd.MM.yy hh:mm:ss"));
        l.append (QString::number (data[i].second));

        new QTreeWidgetItem (historyTable, l);
        x[i] = data[i].first;
        y[i] = data[i].second;
    }

    historyTable->resizeColumnToContents (0);

    QString title;

    switch (stage) {
    case HS_Stage1:
        title = tr ("Stage 1");
        break;
    case HS_Stage2:
        title = tr ("Stage 2");
        break;
    case HS_Stage3:
        title = tr ("Stage 3");
        break;
    case HS_Stage4:
        title = tr ("Stage 4");
        break;
    case HS_Events:
        title = tr ("Events");
        break;
    case HS_Cleanings:
        title = tr ("Clearnings");
        break;
    }

    title += ", ";

    switch (kind) {
    case HK_GrainFlow:
        title += tr ("Grain flow history");
        break;
    case HK_GrainTemp:
        title += tr ("Grain temperature history");
        break;
    case HK_GrainNature:
        title += tr ("Grain nature history");
        break;
    case HK_WaterPress:
        title += tr ("Water pressure history");
        break;
    case HK_GrainHumidity:
        title += tr ("Grain humidity history");
        break;
    case HK_WaterFlow:
        title += tr ("Water flow history");
        break;
    case HK_Setting:
        title += tr ("Setting history");
        break;
    }

    // plot
    plot->clear ();
    plot->setTitle (title);
    QwtPlotCurve* curve = new QwtPlotCurve ();
    QPen pen (Qt::blue, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    curve->setData (x, y, data.size ());
    curve->setPen (pen);
    curve->attach (plot);
    plot->replot ();
    free (x);
    free (y);
}


void MainWindow::daemonBsuPoweredUpdated (int stage, bool on)
{
    if (!on) {
        if (getStageControl (stage)->running ())
            Logger::instance ()->log (Logger::Error, tr ("BSU in stage %1 not powered, stopping stage").arg (stage+1));
        else
            Logger::instance ()->log (Logger::Warning, tr ("BSU in stage %1 not powered").arg (stage+1));
    }
}


void MainWindow::daemonGrainLowUpdated (int stage, bool on)
{
    if (on)
        if (getStageControl (stage)->running ())
            Logger::instance ()->log (Logger::Error, tr ("Grain is low in stage %1, stopping stage").arg (stage+1));
        else
            Logger::instance ()->log (Logger::Warning, tr ("Grain is low in stage %1").arg (stage+1));
    getStageControl (stage)->setGrainState (on ? StageControl::GS_GrainLow : StageControl::GS_GrainPresent);
}


void MainWindow::daemonAutoModeError (error_kind_t error, const QString&)
{
    switch (error) {
    case Err_NoAnswer:
        Logger::instance ()->log (Logger::Warning, tr ("Cannot communicate with controller"));
        break;
    case Err_NotConnected:
        Logger::instance ()->log (Logger::Warning, tr ("Daemon not connected with controller"));
        break;
    case Err_ManualMode:
        Logger::instance ()->log (Logger::Warning, tr ("System in manual mode"));
        break;
    case Err_WaterPress:
        Logger::instance ()->log (Logger::Error, tr ("Water pressure is below it's minimum. All stages are stopped."));
        break;
    }
}


void MainWindow::cleanFilterButtonClicked ()
{
    // check for working stages
    if (haveActiveStages ()) {
        Logger::instance ()->log (Logger::Warning, tr ("Filter cleaning cannot be started, because there are active stages"));
        return;
    }

    _daemon.cleanFilter ();
    _daemon.getCleanState ();
    Logger::instance ()->log (Logger::Information, tr ("Filter cleaning requested"));
}


void MainWindow::cleanStagesButtonClicked ()
{
    bool s[] = {
        cleanS1Check->isChecked (),
        cleanS2Check->isChecked (),
        cleanS3Check->isChecked (),
        cleanS4Check->isChecked (),
    };
    QString res;

    if (haveActiveStages ()) {
        Logger::instance ()->log (Logger::Warning, tr ("Cannot clean stages, because there are active stages"));
        return;
    }

    for (int i = 0; i < 4; i++) {
        if (s[i]) {
            if (getStageControl (i)->enabled ()) {
                if (!res.isEmpty ())
                    res += ", ";
                res += tr ("stage %1").arg (i+1);
            }
        }
    }

    if (! (s[0] || s[1] || s[2] || s[3]))
        return;

    _daemon.cleanStages (s[0], s[1], s[2], s[3]);
    Logger::instance ()->log (Logger::Information, tr ("Clean of %1 requested").arg (res));
    _daemon.getCleanState ();
}


void MainWindow::daemonCleanRequested ()
{
    Logger::instance ()->log (Logger::Information, tr ("Clean initiated. Wait for button."));
}


void MainWindow::daemonCleanStarted ()
{
    Logger::instance ()->log (Logger::Information, tr ("Clean started"));
}


void MainWindow::daemonCleanFinished ()
{
    Logger::instance ()->log (Logger::Information, tr ("Clean finished, requesting clean result"));
    _daemon.getCleanState ();
    _daemon.getCleanResult ();
}


void MainWindow::drainWaterButtonClicked ()
{
    if (drainS1Check->isChecked () || drainS2Check->isChecked () || drainS3Check->isChecked () || drainS4Check->isChecked ()) {
        _daemon.drainWater (drainS1Check->isChecked (), drainS2Check->isChecked (), drainS3Check->isChecked (), drainS4Check->isChecked ());
        Logger::instance ()->log (Logger::Information, tr ("Drain water of specified stages started"));
    }
}


void MainWindow::daemonDrainStarted ()
{
    Logger::instance ()->log (Logger::Information, tr ("Drain started"));
}


void MainWindow::daemonDrainFinished ()
{
    Logger::instance ()->log (Logger::Information, tr ("Drain finished"));
}


void MainWindow::daemonStageStarted (int stage)
{
    getStageControl (stage)->setRunning (true);
    getStageControl (stage)->setWaterPresent (true);
    Logger::instance ()->log (Logger::Information, tr ("Stage %1 started").arg (stage+1));
    globalStopButton->setEnabled (true);
}


void MainWindow::daemonStageStopped (int stage)
{
    getStageControl (stage)->setRunning (false);
    getStageControl (stage)->setWaterPresent (false);
    Logger::instance ()->log (Logger::Information, tr ("Stage %1 stopped").arg (stage+1));
    if (!haveActiveStages ())
        globalStopButton->setEnabled (false);
}


void MainWindow::daemonStageRunningUpdated (int stage, bool running)
{
    if (getStageControl (stage)->running () != running) {
        if (running) {
            Logger::instance ()->log (Logger::Information, tr ("Stage %1 currently at running state").arg (stage+1));
            globalStopButton->setEnabled (true);
        }
        else {
            Logger::instance ()->log (Logger::Information, tr ("Stage %1 currently at stopped state").arg (stage+1));
            if (!haveActiveStages ())
                globalStopButton->setEnabled (false);
        }
        getStageControl (stage)->setRunning (running);
    }
}


void MainWindow::stageTargetHumidityUpdated (int stage, double value)
{
    StageSettings sett = _daemon.getSettings (stage);
    sett.setTargetHumidity (value);
    _daemon.setSettings (stage, sett);

    if (stage == settingsStageComboBox->currentIndex ())
        settingsTargetHumidityEdit->setText (QString::number (value, 'f', 1));
}


void MainWindow::historyItemChanged (int item)
{
    history_stage_t stage = (history_stage_t)historyStageCombo->itemData (item).toInt ();

    historyKindCombo->setEnabled (stage != HS_Events && stage != HS_Cleanings);
    historyTabWidget->setTabEnabled (1, stage != HS_Events && stage != HS_Cleanings);
}


void MainWindow::eventsGot (const QList < QPair <uint, QString> >& data)
{
    QStringList l;

    historyTable->clear ();

    for (int i = 0; i < data.size (); i++) {
        l.clear ();
        l.append (QDateTime::fromTime_t (data[i].first).toString ("dd.MM.yy hh:mm:ss"));
        l.append (data[i].second);

        new QTreeWidgetItem (historyTable, l);
    }

    historyTable->resizeColumnToContents (0);
}


void MainWindow::daemonGotCleanState (bool filter, bool s1, bool s2, bool s3, bool s4)
{
    bsuControl->setCleaning (filter);

    if (filter || s1 || s2 || s3 || s4 ) {
        if (!_cleaningWatchdogTimer)
            _cleaningWatchdogTimer = startTimer (5000);
    }
    else {
        if (_cleaningWatchdogTimer)
            killTimer (_cleaningWatchdogTimer);
    }

    // check for initiated cleaning
    if (filter && !_filterCleaning)
        Logger::instance ()->log (Logger::Information, tr ("Filter cleaning started"));
    
    // check for finished cleaning
    if (!filter && _filterCleaning)
        Logger::instance ()->log (Logger::Information, tr ("Filter cleaning finished"));
        
    _filterCleaning = filter;
    _stageCleaning[0] = s1;
    _stageCleaning[1] = s2;
    _stageCleaning[2] = s3;
    _stageCleaning[3] = s4;

    for (int i = 0; i < 4; i++)
        getStageControl (i)->setCleaning (_stageCleaning[i]);
}


void MainWindow::daemonGotCleanResult (bool s_w[4], bool s_r[4])
{
    int i;
    QString msg;

    for (i = 0; i < 4; i++)
        if (getStageControl (i)->enabled ()) {
            if (s_w[i])
                msg = tr ("Stage %1 cleaning with water performed").arg (i+1);
            else
                msg = tr ("Stage %1 cleaning with water not performed").arg (i+1);
            Logger::instance ()->log (Logger::Information, msg);
            _daemon.logCleanResult (msg);
        }

    for (i = 0; i < 4; i++)
        if (getStageControl (i)->enabled ()) {
            if (s_r[i])
                msg = tr ("Stage %1 cleaning from Rottenberg performed").arg (i+1);
            else
                msg = tr ("Stage %1 cleaning from Rottenberg not performed").arg (i+1);
            Logger::instance ()->log (Logger::Information, msg);
            _daemon.logCleanResult (msg);
        }
}


void MainWindow::stageTargetWaterFlowUpdated (int stage, double value)
{
    _daemon.setTargetWaterFlow (stage, value);
}


void MainWindow::daemonMinPressureGot (double val)
{
    minPressureEdit->setText (QString::number (val));
}



// --------------------------------------------------
// PlotScaleDraw
// --------------------------------------------------
QwtText PlotScaleDraw::label (double v) const
{
    return QDateTime::fromTime_t ((uint)v).toString (_show_date ? "dd.MM hh:mm" : "hh:mm:ss");
}


