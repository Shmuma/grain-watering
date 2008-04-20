#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stagecontrol.h"
#include "logger.h"
#include "tableform.h"

#include <QtCore>
#include <QtGui>


// --------------------------------------------------
// MainWindow
// --------------------------------------------------
MainWindow::MainWindow ()
    : QWidget (0),
      _switchingToolButtons (false),
      _settingsChanged (false),
      _daemon ("localhost", 12345),
      _currentSettingsStage (0),
      _access (AL_Default)
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

    connect (globalStopButton,  SIGNAL (clicked ()), this, SLOT (globalStopButtonClicked ()));

    for (int i = 0; i < 4; i++) {
        connect (getStageControl (i), SIGNAL (startPressed (int)), this, SLOT (startButtonClicked (int)));
        connect (getStageControl (i), SIGNAL (stopPressed (int)), this, SLOT (stopButtonClicked (int)));
        connect (getStageControl (i), SIGNAL (pausePressed (int, bool)), this, SLOT (pauseButtonClicked (int, bool)));
    }

    // connect active stages checkboxes
    connect (applyStagesButton, SIGNAL (clicked ()), this, SLOT (applyStagesButtonClicked ()));

    // hide settings panel
    settingsPanel->hide ();

    // tune stage controls
    stageControl1->setNumber (0);
    stageControl2->setNumber (1);
    stageControl3->setNumber (2);
    stageControl4->setNumber (3);
    stageControl1->hide ();
    stageControl2->hide ();
    stageControl3->hide ();
    stageControl4->hide ();

    // connect button
    connect (connectButton, SIGNAL (doubleClicked ()), this, SLOT (connectButtonClicked ()));

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
    connect (&_daemon, SIGNAL (hardwareConnected ()), this, SLOT (daemonHardwareConnected ()));
    connect (&_daemon, SIGNAL (textArrived (const QString&)), this, SLOT (daemonTextReceived (const QString&)));
    connect (&_daemon, SIGNAL (autoTextArrived (const QString&)), this, SLOT (daemonAutoTextReceived (const QString&)));
    connect (&_daemon, SIGNAL (commandSent (const QString&)), this, SLOT (daemonCommandSent (const QString&)));
    connect (&_daemon, SIGNAL (stagesActivityChanged (bool,bool,bool,bool)), this, SLOT (daemonStagesActivityChanged (bool,bool,bool,bool)));
    connect (&_daemon, SIGNAL (grainFlowGot (int, int)), this, SLOT (daemonGrainFlowGot (int, int)));
    connect (&_daemon, SIGNAL (autoModeTickGot (bool, double)), this, SLOT (daemonAutoModeTickGot (bool, double)));
    connect (&_daemon, SIGNAL (autoModeGot (int, bool, bool)), this, SLOT (daemonAutoModeGot (int, bool, bool)));
    connect (&_daemon, SIGNAL (autoModeStarted (int)), this, SLOT (daemonAutoModeStarted (int)));
    connect (&_daemon, SIGNAL (autoModeStopped (int)), this, SLOT (daemonAutoModeStopped (int)));
    connect (&_daemon, SIGNAL (autoModeToggled (int, bool)), this, SLOT (daemonAutoModeToggled (int, bool)));
    connect (&_daemon, SIGNAL (metaStateGot (double, QMap<int, QList<double> >)), this, SLOT (daemonMetaStateGot (double, QMap<int, QList<double> >)));
    connect (&_daemon, SIGNAL (waterStarted (int)), this, SLOT (daemonWaterStarted (int)));
    connect (&_daemon, SIGNAL (waterStopped (int)), this, SLOT (daemonWaterStopped (int)));
    connect (&_daemon, SIGNAL (grainSensorsPresenceGot (bool)), this, SLOT (daemonGrainSensorsPresenceGot (bool)));
    connect (&_daemon, SIGNAL (grainPresenceGot (int, bool)), this, SLOT (daemonGrainPresenceGot (int, bool)));

    // daemon check loop events
    connect (&_daemon, SIGNAL (waterPressureUpdated (double)), this, SLOT (daemonWaterPressureUpdated (double)));
    connect (&_daemon, SIGNAL (grainPresentUpdated (int, bool)), this, SLOT (daemonGrainPresentUpdated (int, bool)));
    connect (&_daemon, SIGNAL (waterFlowUpdated (int, double)), this, SLOT (daemonWaterFlowUpdated (int, double)));
    connect (&_daemon, SIGNAL (grainFlowUpdated (int, double)), this, SLOT (daemonGrainFlowUpdated (int, double)));
    connect (&_daemon, SIGNAL (grainHumidityUpdated (int, double)), this, SLOT (daemonGrainHumidityUpdated (int, double)));
    connect (&_daemon, SIGNAL (grainTemperatureUpdated (int, double)), this, SLOT (daemonGrainTemperatureUpdated (int, double)));
    connect (&_daemon, SIGNAL (grainNatureUpdated (int, double)), this, SLOT (daemonGrainNatureUpdated (int, double)));
    connect (&_daemon, SIGNAL (settingsGot ()), this, SLOT (daemonSettingsGot ()));

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

    QPushButton* btns[] = {configButton, checkButton, paramsButton, sensorsButton};

    for (uint i = 0; i < sizeof (btns) / sizeof (btns[0]); i++) {
        btns[i]->setEnabled (value);
        btns[i]->setChecked (false);
    }

    if (!value)
        daemonStagesActivityChanged (false, false, false, false);
}


// hardware connection got, fetch daemon state
void MainWindow::daemonHardwareConnected ()
{
    // enabled stages
    _daemon.getStages ();

    // auto mode
    for (int i = 0; i < 4; i++)
        _daemon.getAutoMode (i);

    // grain sensors presense
    _daemon.isGrainSensorsPresent ();
    _daemon.requestSettings ();
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
}


void MainWindow::applyStagesButtonClicked ()
{
    _daemon.setStages (stage1ActiveCheckBox->isChecked (), stage2ActiveCheckBox->isChecked (), 
                       stage3ActiveCheckBox->isChecked (), stage4ActiveCheckBox->isChecked ());
    _daemon.requestSettings ();
}


void MainWindow::daemonStagesActivityChanged (bool s1, bool s2, bool s3, bool s4)
{
    stageControl1->setEnabled (s1);
    stageControl2->setEnabled (s2);
    stageControl3->setEnabled (s3);
    stageControl4->setEnabled (s4);

    stageControl1->setVisible (s1);
    stageControl2->setVisible (s2);
    stageControl3->setVisible (s3);
    stageControl4->setVisible (s4);

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
}


void MainWindow::daemonGrainFlowGot (int stage, int value)
{
    getStageControl (stage)->setGrainFlow (value);
}


void MainWindow::startButtonClicked (int stage)
{
    _daemon.startAutoMode (stage);
}


void MainWindow::stopButtonClicked (int stage)
{
    _daemon.stopAutoMode (stage);
}


void MainWindow::pauseButtonClicked (int stage, bool)
{
    _daemon.toggleAutoMode (stage);
}


void MainWindow::globalStopButtonClicked ()
{
    for (int i = 0; i < 4; i++)
        _daemon.stopAutoMode (i);
}


void MainWindow::daemonAutoModeTickGot (bool state, double press)
{
    Logger::instance ()->log (Logger::Debug, QString ("Auto mode tick. State: %1, Press: %2").arg (state ? "OK" : "ERROR", QString::number (press)));
}


void MainWindow::daemonAutoModeStarted (int stage)
{
    getStageControl (stage)->setState (StageControl::S_Started);
    autoLogEditor->setVisible (haveActiveStages ());
    globalStopButton->setEnabled (haveActiveStages ());
}


void MainWindow::daemonAutoModeStopped (int stage)
{
    getStageControl (stage)->setState (StageControl::S_Stopped);
    autoLogEditor->setVisible (haveActiveStages ());
    globalStopButton->setEnabled (haveActiveStages ());
}


void MainWindow::daemonAutoModeToggled (int stage, bool paused)
{
    getStageControl (stage)->setState (paused ? StageControl::S_Paused : StageControl::S_Started);
}


void MainWindow::daemonAutoModeGot (int stage, bool active, bool paused)
{
    if (!_daemon.isStageEnabled (stage))
        return;

    if (active && paused)
        getStageControl (stage)->setState (StageControl::S_Paused);
    else
        if (active)
            getStageControl (stage)->setState (StageControl::S_Started);
        else
            getStageControl (stage)->setState (StageControl::S_Stopped);

    autoLogEditor->setVisible (haveActiveStages ());
    globalStopButton->setEnabled (haveActiveStages ());
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
    QCheckBox* boxes[] = { checkWaterStage1Check, checkWaterStage2Check, checkWaterStage3Check, checkWaterStage4Check };

    for (int i = 0; i < 4; i++)
        if (_daemon.isStageEnabled (i))
            if (boxes[i]->isChecked ())
                _daemon.startWater (i);
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
}


void MainWindow::daemonGrainPresenceGot (int stage, bool value)
{
    QString text (value ? tr ("Present") : tr ("Not present"));

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
    getStageControl (stage)->setGrain (present);
}


void MainWindow::daemonWaterFlowUpdated (int stage, double val)
{
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


void MainWindow::daemonSettingsGot ()
{
    settingsStageComboActivated (settingsStageComboBox->currentIndex ());
}


bool MainWindow::haveActiveStages () const
{
    for (int i = 0; i < 4; i++)
        if (getStageControl (i)->state () == StageControl::S_Started)
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

    settingsTargetHumidityEdit->setText (QString::number (sett.targetHumidity ()));
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
    if (ok)
        sett.setTargetHumidity (val);
    else {
        QMessageBox::warning (this, tr ("Value error"), tr ("Target humidity have invalid format"));
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

    sett.setWaterFormula (settingsWaterFormulaComboBox->currentIndex ());
    sett.setHumidityTable (_humidityTable);
    sett.setGrainFlowTable (_grainFlowTable);
    sett.setGrainNatureTable (_grainNatureTable);
    sett.setGrainTempTable (_grainTempTable);
    sett.setGrainNatureCoeffTable (_grainNatureCoeffTable);

    sett.setValid (true);
    _daemon.setSettings (stage, sett);
    _settingsChanged = false;
}



void MainWindow::settingsValueEdited (const QString&)
{
    _settingsChanged = true;
}


void MainWindow::settingsHumidityTableClicked ()
{
    TableForm dlg (this, tr ("Humidity table editor"), tr ("Key (hex) -> Humidity map"), tr ("Value (hex)"), tr ("Humidity"), true);

    dlg.setData (_humidityTable);

    if (dlg.exec () == QDialog::Rejected)
        return;

    _settingsChanged = true;
    _humidityTable = dlg.result ();
}


void MainWindow::settingsGrainFlowTableClicked ()
{
    TableForm dlg (this, tr ("Grain flow table editor"), tr ("Key (hex) -> Grain flow map"), tr ("Value (hex)"), tr ("Grain flow"), true);

    dlg.setData (_grainFlowTable);

    if (dlg.exec () == QDialog::Rejected)
        return;

    _settingsChanged = true;
    _grainFlowTable = dlg.result ();
}


void MainWindow::settingsGrainNatureTableClicked ()
{
    TableForm dlg (this, tr ("Grain nature table editor"), tr ("Key (hex) -> Grain nature map"), tr ("Value (hex)"), tr ("Grain nature"), true);

    dlg.setData (_grainNatureTable);

    if (dlg.exec () == QDialog::Rejected)
        return;

    _settingsChanged = true;
    _grainNatureTable = dlg.result ();
}


void MainWindow::settingsGrainTempTableClicked ()
{
    TableForm dlg (this, tr ("Grain temperature table editor"), tr ("Temperature (dec) -> Grain temperature coeff map"), 
                   tr ("Temperature"), tr ("Grain temperature coeff"), false);

    dlg.setData (_grainTempTable);

    if (dlg.exec () == QDialog::Rejected)
        return;

    _settingsChanged = true;
    _grainTempTable = dlg.result ();
}


void MainWindow::settingsGrainNatureCoeffTableClicked ()
{
    TableForm dlg (this, tr ("Grain nature coeff table editor"), tr ("Temperature (dec) -> Grain nature coeff map"), 
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
        return false;
    }
    else {
        _access = level;
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
