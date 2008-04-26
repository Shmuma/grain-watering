#ifndef __STAGECONTROL_H__
#define __STAGECONTROL_H__

#include <QtGui>


class StageControl : public QWidget
{
    Q_OBJECT

public:
    enum state_t {
        S_Stopped,
        S_Started,
        S_Paused,
    };

protected:
    void paintEvent (QPaintEvent* event);

protected slots:
    void startToggled (bool checked);
    void pauseToggled (bool checked);

signals:
    void startPressed (int stage);
    void stopPressed (int stage);
    void pausePressed (int stage, bool on);

public:
    StageControl (QWidget* parent);

    void setGrain (bool grain)
        { _grain = grain; update (); };
    bool grain () const
        { return _grain; };

    void setNumber (int number)
        { _number = number; update (); };
    int number () const
        { return _number; };
    
    void setEnabled (bool enabled)
        { _enabled = enabled; update (); };
    bool enabled () const
        { return _enabled; };

    void setGrainFlow (double flow)
        { _flow = flow; update (); };
    double grainFlow () const
        { return _flow; };

    void setHumidity (double humidity)
        { _humidity = humidity; update (); };
    double humidity () const
        { return _humidity; };

    void setNature (double nature)
        { _nature = nature; update (); };
    double nature () const
        { return _nature; };

    void setTemperature (double temp)
        { _temp = temp; update (); };
    double temperature () const
        { return _temp; };
    
    void setWaterFlow (double waterFlow)
        { _waterFlow = waterFlow; update (); };
    double waterFlow () const
        { return _waterFlow; };

    void setTargetHumidity (double humidity)
        { _targetHumidity = humidity; update (); };
    double targetHumidity () const
        { return _targetHumidity; };

    StageControl::state_t state () const
        { return _state; };
    
    void setState (state_t state)
        { _state = state; handleNewState (); update (); };

    void setSetting (double setting)
        { _setting = setting; update (); };
    double setting () const
        { return _setting; };

    void setSensors (bool sensors)
        { _sensors = sensors; update (); };
    bool sensors () const
        { return _sensors; };

    QString label () const
        { return _label; };
    void setLabel (const QString& label)
        { _label = label; };

private:
    int _number;
    bool _enabled;
    bool _grain;
    double _flow;
    double _humidity;
    double _nature;
    double _temp;
    double _waterFlow;
    double _targetHumidity;
    double _setting;
    state_t _state;
    bool _sensors;
    QString _label;

    QToolButton *_start, *_pause;
    bool _inHandleState;

    void handleNewState ();
};

#endif
