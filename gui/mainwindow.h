#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QtCore>
#include <QtGui>
#include <qwt_plot.h>
#include <qwt_scale_draw.h>

#include "ui_mainwindow.h"
#include "stagecontrol.h"
#include "daemon.h"
#include "logger.h"
#include "history.h"


class MainWindow : public QWidget, private Ui::MainWindow
{
    Q_OBJECT
public:
    enum access_level_t {
        AL_Default = 0,
        AL_Config,
        AL_Admin,
    };

private:
    bool _switchingToolButtons, _settingsChanged;
    Daemon _daemon;
    int _currentSettingsStage;
    QMap<int, double> _humidityTable;
    QMap<int, double> _grainFlowTable;
    QMap<int, double> _grainNatureTable;
    QMap<int, double> _grainTempTable;
    QMap<int, double> _grainNatureCoeffTable;
    access_level_t _access;
    QAction* _stages_view;
    int _calibrateStage;

protected:
    virtual void timerEvent (QTimerEvent* event);

    void refreshScreenClock ();
    StageControl* getStageControl (int stage) const;

    bool haveActiveStages () const;
    void saveSettingsPage (int stage);

    bool haveAccess (access_level_t level);
    void setSettingsPanelVisible (bool visible);

protected slots:
    void configButtonToggled (bool on);
    void modeButtonToggled (bool on);
    void checkButtonToggled (bool on);
    void paramsButtonToggled (bool on);
    void sensorsButtonToggled (bool on);
    void cleanButtonToggled (bool on);

    // active state checkbox 
    void applyStagesButtonClicked ();

    // logger
    void loggerMessage (Logger::severity_t sev, const QString& msg);

    // toolbar buttons
    void connectButtonClicked ();
    void consoleSendButtonClicked ();
    void switchToStagesView ();
    void switchToHistoryView ();
    void switchToConsoleView ();

    void globalStopButtonClicked ();
    void startButtonClicked (int stage);
    void stopButtonClicked (int stage);
    void stageTargetHumidityUpdated (int stage, double value);

    // daemon signals
    void connectedChanged (bool value);
    void daemonHardwareConnected ();
    void daemonTextReceived (const QString& msg);
    void daemonAutoTextReceived (const QString& msg);
    void daemonCommandSent (const QString& msg);
    void daemonStagesActivityChanged (bool s1, bool s2, bool s3, bool s4);
    void daemonGrainFlowGot (int stage, int value);
    void daemonStageStarted (int stage);
    void daemonStageStopped (int stage);
    
    void daemonMetaStateGot (double water_pres, QMap<int, QList<double> > vals);
    void daemonWaterStarted (int stage);
    void daemonWaterStopped (int stage);
    void daemonGrainSensorsPresenceGot (bool value);
    void daemonGrainPresenceGot (int stage, bool value);

    // daemon check loop
    void daemonStageRunningUpdated (int stage, bool running);
    void daemonWaterPressureUpdated (double val);
    void daemonGrainPresentUpdated (int stage, bool present);
    void daemonWaterFlowUpdated (int stage, double val);
    void daemonGrainFlowUpdated (int stage, double val);
    void daemonGrainHumidityUpdated (int stage, double val);
    void daemonGrainTemperatureUpdated (int stage, double val);
    void daemonGrainNatureUpdated (int stage, double val);
    void daemonCalculatedHumidityUpdated (int stage, double val);
    void daemonTargetFlowUpdated (int stage, double val);
    void daemonTargetSettingUpdated (int stage, double val);
    void daemonSettingsGot ();
    void daemonCalibrateReply (int stage, const QString& key, double val);
    void daemonBsuPoweredUpdated (int stage, bool on);
    void daemonWaterPresentUpdated (int stage, bool on);
    void daemonGrainLowUpdated (int stage, bool on);
    void daemonAutoModeError (bool timeout, bool manual);
    void daemonCleanFinished ();
    void daemonDrainFinished ();

    // check page
    void checkStateButtonPressed ();
    void checkWaterButtonPressed ();
    void checkGrainSensorsButtonPressed ();
    void applyCheckWaterButtonClicked ();
    void stateRefreshButtonClicked ();
    void grainSensorsEnabledChecked (bool val);
    void checkGrainStageApplyClicked ();

    // console
    void sendFileButtonClicked ();

    // settings dialog
    void settingsStageComboActivated (int item);
    void applySettingsButtonClicked ();
    void settingsValueEdited (const QString&);
    void settingsHumidityTableClicked ();
    void settingsGrainFlowTableClicked ();
    void settingsGrainNatureTableClicked ();
    void settingsGrainTempTableClicked ();
    void settingsGrainNatureCoeffTableClicked ();
    void settingsAdvancedGroupBoxChecked (bool on);

    void stageSensorsApplyButtonClicked ();
    void tempCoefGot (double k, double resist);
    void tempSensorsGroupboxChecked (bool on);

    void calibrateS1Checked (bool on);
    void calibrateS2Checked (bool on);
    void calibrateS3Checked (bool on);
    void calibrateS4Checked (bool on);

    void calibrateButtonClicked ();
    void stageModesApplyButtonClicked ();

    // history
    void historyItemChanged (int item);
    void refreshHistoryButtonClicked ();
    void historyPeriodComboChanged (int index);
    void historyGot (const QList < QPair <uint, double> >& data, history_stage_t stage, history_kind_t kind);
    void eventsGot (const QList < QPair <uint, QString> >& data);
    void graphResetButtonClicked ();

    // cleaning
    void cleanFilterButtonClicked ();
    void cleanStagesButtonClicked ();
    void drainWaterButtonClicked ();

public:
    MainWindow ();
};


class PlotScaleDraw : public QwtScaleDraw
{
private:
    bool _show_date;

public:
    PlotScaleDraw (bool show_date)
        : QwtScaleDraw (),
          _show_date (show_date) {};

    virtual QwtText label (double v) const;

    bool showDate () const
        { return _show_date; };
    void setShowDate (bool show_date)
        { _show_date = show_date; };
};


#endif
