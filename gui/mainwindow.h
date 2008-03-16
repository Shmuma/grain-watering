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

private:
    bool _switchingToolButtons;
    Daemon _daemon;

protected:
    virtual void timerEvent (QTimerEvent* event);

    void refreshScreenClock ();
    StageControl* getStageControl (int stage) const;

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
    void startButtonClicked ();
    void stopButtonClicked ();
    void pauseButtonClicked ();

    // daemon signals
    void connectedChanged (bool value);
    void daemonHardwareConnected ();
    void daemonTextReceived (const QString& msg);
    void daemonAutoTextReceived (const QString& msg);
    void daemonCommandSent (const QString& msg);
    void daemonStagesActivityChanged (bool s1, bool s2, bool s3, bool s4);
    void daemonGrainFlowGot (int stage, int value);
    void daemonAutoModeTickGot (bool state, int press);
    void daemonAutoModeStarted ();
    void daemonAutoModeStopped ();
    void daemonAutoModeToggled (bool paused);
    void daemonAutoModeGot (bool active, bool paused);
    void daemonMetaStateGot (int water_pres, QMap<int, QList<int> > vals);
    void daemonWaterStarted (int stage);
    void daemonWaterStopped (int stage);

    // check page
    void checkStateButtonPressed ();
    void checkWaterButtonPressed ();
    void checkGrainSensorsButtonPressed ();
    void applyCheckWaterButtonClicked ();
    void stateRefreshButtonClicked ();

    // console
    void sendFileButtonClicked ();

public:
    MainWindow ();
};


#endif
