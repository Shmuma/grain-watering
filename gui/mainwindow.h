#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QtCore>
#include <QtGui>

#include "ui_mainwindow.h"
#include "stagecontrol.h"
#include "controller.h"
#include "daemon.h"
#include "logger.h"


class MainWindow : public QWidget, private Ui::MainWindow
{
    Q_OBJECT

private:
    bool _switchingToolButtons;
    ControllerState _state;
    Daemon _daemon;

protected:
    virtual void timerEvent (QTimerEvent* event);

    void refreshScreenClock ();

protected slots:
    void configButtonToggled (bool on);
    void checkButtonToggled (bool on);
    void paramsButtonToggled (bool on);
    void sensorsButtonToggled (bool on);

    // active state checkbox 
    void stage1ActiveCheckBoxToggled (bool on);
    void stage2ActiveCheckBoxToggled (bool on);
    void stage3ActiveCheckBoxToggled (bool on);
    void stage4ActiveCheckBoxToggled (bool on);

    // logger
    void loggerMessage (Logger::severity_t sev, const QString& msg);

    // state slots
    void stageEnabledChanged (int stages, bool enabled);

    // connect button clicked
    void connectButtonClicked ();
    void consoleSendButtonClicked ();
    void switchToStagesView ();
    void switchToHistoryView ();
    void switchToConsoleView ();

    // daemon signals
    void connectedChanged (bool value);
    void daemonTextReceived (const QString& msg);
public:
    MainWindow ();
};


#endif
