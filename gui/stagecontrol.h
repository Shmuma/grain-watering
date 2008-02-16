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
    { _flow = flow; };
    int grainFlow () const
    { return _flow; };

    void setHumidity (int humidity)
    { _humidity = humidity; };
    int humidity () const
    { return _humidity; };

    void setNature (int nature)
    { _nature = nature; };
    int nature () const
    { return _nature; };

    void setTemperature (int temp)
    { _temp = temp; };
    int temperature () const
    { return _temp; };
    
private:
    int _number;
    bool _enabled;
    long _flow;
    long _humidity;
    long _nature;
    long _temp;
};

#endif
