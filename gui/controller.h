#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include <QtCore>


class StageState 
{
private:
    bool _enabled;

public:
    StageState ()
        : _enabled (false)
    {};

    bool enabled () const
    { return _enabled; };
    void setEnabled (bool enabled)
    { _enabled = enabled; };
};


class ControllerState : public QObject
{
    Q_OBJECT
private:
    StageState stages[4];

signals:
    void stageEnabledChanged (int, bool);
    
public:
    ControllerState ();

    void setStageEnabled (int stage, bool enabled); 
};

#endif
