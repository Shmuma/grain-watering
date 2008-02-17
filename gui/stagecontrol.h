#ifndef __STAGECONTROL_H__
#define __STAGECONTROL_H__

#include <QtGui>

class StageControl : public QWidget
{
    Q_OBJECT

protected:
    void paintEvent (QPaintEvent* event);

public:
    StageControl (QWidget* parent);

    void setNumber (int number)
    { _number = number; update (); };
    int number () const
    { return _number; };
    
    void setEnabled (bool enabled)
    { _enabled = enabled; update (); };
    bool enabled () const
    { return _enabled; };

    void setGrainFlow (int flow)
    { _flow = flow; update (); };
    int grainFlow () const
    { return _flow; };

    void setHumidity (int humidity)
    { _humidity = humidity; update (); };
    int humidity () const
    { return _humidity; };

    void setNature (int nature)
    { _nature = nature; update (); };
    int nature () const
    { return _nature; };

    void setTemperature (int temp)
    { _temp = temp; update (); };
    int temperature () const
    { return _temp; };
    
    void setWaterFlow (int waterFlow)
    { _waterFlow = waterFlow; update (); };
    int waterFlow () const
    { return _waterFlow; };

    void setTargetHumidity (int humidity)
    { _targetHumidity = humidity; update (); };
    int targetHumidity () const
    { return _targetHumidity; };
private:
    int _number;
    bool _enabled;
    int _flow;
    int _humidity;
    int _nature;
    int _temp;
    int _waterFlow;
    int _targetHumidity;
};

#endif
