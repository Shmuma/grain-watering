#ifndef __STAGECONTROL_H__
#define __STAGECONTROL_H__

#include <QtGui>

class StageControl : public QWidget
{
    Q_OBJECT

protected:
    void paintEvent (QPaintEvent* event);

signals:
    void startPressed ();
    void stopPressed ();
    void pausePressed (bool on);

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

    bool isStarted () const
        { return _started; };
    void start ();
    void stop ();

    bool isPaused () const
        { return _paused; };
    void pause (bool on);

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
    bool _started;
    bool _paused;

    QToolButton *_start, *_pause;
};

#endif
