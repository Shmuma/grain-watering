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

protected slots:
    void configButtonToggled (bool on);
    void checkButtonToggled (bool on);
    void paramsButtonToggled (bool on);
    void sensorsButtonToggled (bool on);

    // active state checkbox 
    void applyStagesButtonClicked ();

    // logger
    void loggerMessage (Logger::severity_t sev, const QString& msg);

    // connect button clicked
    void connectButtonClicked ();
    void consoleSendButtonClicked ();
    void switchToStagesView ();
    void switchToHistoryView ();
    void switchToConsoleView ();

    // daemon signals
    void connectedChanged (bool value);
    void daemonTextReceived (const QString& msg);
    void daemonCommandSent (const QString& msg);
    void daemonStagesActivityChanged (bool s1, bool s2, bool s3, bool s4);

public:
    MainWindow ();
};


#endif
