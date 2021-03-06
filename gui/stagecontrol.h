#ifndef __STAGECONTROL_H__
#define __STAGECONTROL_H__

#include <QtGui>
#include <QtSvg>

#include "settings.h"


class StageControl : public QWidget
{
    Q_OBJECT

public:
    enum grainstate_t {
        GS_GrainLow,
        GS_GrainMissing,
        GS_GrainPresent,
    };

    enum button_state_t {
        BS_Normal = 0,
        BS_Pressed,
        BS_Disabled
    };

protected:
    void paintEvent (QPaintEvent* event);
    void mousePressEvent (QMouseEvent* e);
    void mouseReleaseEvent (QMouseEvent* e);
    void setColor (QPainter& p, const char* color);
    void timerEvent (QTimerEvent* e);

protected slots:
    void startClicked ();
    void stopClicked ();

    void humidityUpClicked ();
    void humidityDownClicked ();

    void waterUpClicked ();
    void waterDownClicked ();
    void humidityEditorReturnPressed ();

signals:
    void startPressed (int stage);
    void stopPressed (int stage);
    void targetHumidityUpdated (int stage, double value);
    void targetWaterFlowUpdated (int stage, double value);

public:
    StageControl (QWidget* parent);

    void setNumber (int number)
        { _number = number; update (); };
    int number () const
        { return _number; };
    
    void setEnabled (bool enabled)
        { _enabled = enabled; setFixedSize (_enabled ? QSize (250, 500) : QSize (0, 0)); update (); };
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

    bool running () const 
        { return _running; };
    void setRunning (bool running);

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
        { _label = label; update (); };

    StageSettings::mode_t mode () const
        { return _mode; };
    void setMode (StageSettings::mode_t mode);

    bool waterPresent () const 
        { return _waterPresent; };
    void setWaterPresent (bool present)
        { _waterPresent = present; update (); };

    grainstate_t grainState () const
        { return _grainState; };
    void setGrainState (grainstate_t state)
        { _grainState = state; update (); };

    bool cleaning () const
        { return _cleaning; };
    void setCleaning (bool cleaning)
        { _cleaning = cleaning; update (); };

    double targetWaterFlow () const
        { return _targetWaterFlow; };
    void setTargetWaterFlow (double val);

    void setMinMaxWaterFlow (double min, double max)
        { _minWaterFlow = min; _maxWaterFlow = max; };

private:
    int _number;
    bool _enabled, _running;
    double _flow;
    double _humidity;
    double _nature;
    double _temp;
    double _waterFlow;
    double _targetHumidity;
    double _setting;
    bool _sensors;
    QString _label;
    StageSettings::mode_t _mode;
    bool _waterPresent;
    grainstate_t _grainState;
    bool _cleaning;
    double _targetWaterFlow;

    double _minWaterFlow, _maxWaterFlow;

    QToolButton *_humidityUp, *_humidityDown;
    QToolButton *_waterUp, *_waterDown;
    QLineEdit* _humidityEdit;

    // pixmaps (argument - mode)
    QPixmap _img[3];
    
    // SVG helpers (argument - mode)
    QSvgRenderer* _svg[3];

    // buttons pixmaps
    QPixmap _startImages[3];
    QPixmap _stopImages[3];
    QRect _startRect, _stopRect;
    bool _startPressed, _stopPressed;

    // water pixmap
    QPixmap _waterImages[2];
    QPixmap _grainImages[3];

    // snek pixmaps and counter
    QPixmap _snekImages[5];
    int _snekCounter;
    int _snekTimer;
};

#endif
