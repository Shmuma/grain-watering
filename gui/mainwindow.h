#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QtCore>
#include <QtGui>

#include "ui_mainwindow.h"
#include "stagecontrol.h"
#include "daemon.h"
#include "logger.h"


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

protected:
    virtual void timerEvent (QTimerEvent* event);

    void refreshScreenClock ();
    StageControl* getStageControl (int stage) const;

    bool haveActiveStages () const;
    void saveSettingsPage (int stage);

    bool haveAccess (access_level_t level);

protected slots:
    void configButtonToggled (bool on);
    void checkButtonToggled (bool on);
    void paramsButtonToggled (bool on);
    void sensorsButtonToggled (bool on);

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
    void pauseButtonClicked (int stage, bool on);

    // daemon signals
    void connectedChanged (bool value);
    void daemonHardwareConnected ();
    void daemonTextReceived (const QString& msg);
    void daemonAutoTextReceived (const QString& msg);
    void daemonCommandSent (const QString& msg);
    void daemonStagesActivityChanged (bool s1, bool s2, bool s3, bool s4);
    void daemonGrainFlowGot (int stage, int value);
    void daemonAutoModeTickGot (bool state, double press);
    void daemonAutoModeStarted (int stage);
    void daemonAutoModeStopped (int stage);
    void daemonAutoModeToggled (int stage, bool paused);
    void daemonAutoModeGot (int stage, bool active, bool paused);
    void daemonMetaStateGot (double water_pres, QMap<int, QList<double> > vals);
    void daemonWaterStarted (int stage);
    void daemonWaterStopped (int stage);
    void daemonGrainSensorsPresenceGot (bool value);
    void daemonGrainPresenceGot (int stage, bool value);

    // daemon check loop
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

public:
    MainWindow ();
};


#endif
