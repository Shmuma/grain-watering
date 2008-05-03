#include <QtGui>
#include <QtSvg>
#include "stagecontrol.h"


// --------------------------------------------------
// StageControl
// --------------------------------------------------
StageControl::StageControl (QWidget* parent)
    : QWidget (parent),
      _inHandleState (false),
      _imgWithSensors (QString (":/stages/svg/stage1.png"), "PNG"),
      _svgWithSensors (QString (":/stages/svg/stage1-full.svg"), this)
{
    _number = 0;
    _grainState = GS_GrainMissing;
    _waterPresent = false;
    _flow = _humidity = _nature = _temp = _waterFlow = _targetHumidity = 0;
    _label = QString ();
    setEnabled (false);

    // control buttons
    _start = new QToolButton (this);
    _stop = new QToolButton (this);

    connect (_start, SIGNAL (clicked ()), this, SLOT (startClicked ()));
    connect (_stop, SIGNAL (clicked ()), this, SLOT (stopClicked ()));

    _start->setText (tr ("Start"));
    _stop->setText (tr ("Stop"));

    QRect r;

    r = _svgWithSensors.boundsOnElement ("StartButton").adjusted (-4, -4, 4, 4).toRect ();
    _start->setGeometry (r);
    r = _svgWithSensors.boundsOnElement ("StopButton").adjusted (-4, -4, 4, 4).toRect ();
    _stop->setGeometry (r);

    setRunning (false);
}


void StageControl::paintEvent (QPaintEvent*)
{
    if (!_enabled)
        return;

    QPainter p (this);
    QRectF r;

    // draw underlying pixmap
    p.drawPixmap (QPoint (0, 0), _imgWithSensors);

//     p.drawRect (QRect (0, 0, geometry ().width ()-1, geometry ().height ()-1));

    p.setFont (QFont ("Arial", 16));
    r = _svgWithSensors.boundsOnElement ("StageTitle").adjusted (2, 2, -2, -2);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("Stage %1: %2").arg (_number+1).arg (_label));

    p.setFont (QFont ("Arial", 16));
    r = _svgWithSensors.boundsOnElement ("GrainHumidity").adjusted (2, 2, -2, -2);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 %").arg (QString ().sprintf ("%.2f", _humidity)));

    p.setFont (QFont ("Arial", 15));
    r = _svgWithSensors.boundsOnElement ("GrainTemperature").adjusted (2, 2, -2, -2);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 C").arg (QString ().sprintf ("%.0f", _temp)));

    p.setFont (QFont ("Arial", 15));
    r = _svgWithSensors.boundsOnElement ("GrainNature").adjusted (2, 2, -2, -2);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 g/l").arg (QString ().sprintf ("%.2f", _nature)));
    
    p.setFont (QFont ("Arial", 16));
    r = _svgWithSensors.boundsOnElement ("GrainFlow").adjusted (2, 2, -2, -2);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 t/h").arg (QString ().sprintf ("%.2f", _flow)));

    p.setFont (QFont ("Verdana", 9));
    r = _svgWithSensors.boundsOnElement ("WaterFlow").adjusted (2, 2, -2, -2);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1\nl/h").arg (QString ().sprintf ("%.0f", _waterFlow)));

    // fill grain area if grain present
    r = _svgWithSensors.boundsOnElement ("GrainArea").adjusted (2, 2, -1, -1);

    switch (_grainState) {
    case GS_GrainPresent:
        p.fillRect (r, QBrush (Qt::yellow));
        break;
    case GS_GrainMissing:
        p.fillRect (r, QBrush (Qt::lightGray));
        break;
    case GS_GrainLow:
        p.fillRect (r.adjusted (0, 0, 0, -r.height ()/2), QBrush (Qt::lightGray));
        p.fillRect (r.adjusted (0, r.height ()/2, 0, 0), QBrush (Qt::yellow));
        break;
    }

    // fill water area
    r = _svgWithSensors.boundsOnElement ("WaterArea").adjusted (2, 2, -1, -1);
    p.fillRect (r, QBrush (_waterPresent ? Qt::cyan : Qt::lightGray));

    // stage mode
    p.setFont (QFont ("Arial", 13));
    r = _svgWithSensors.boundsOnElement ("StageMode").adjusted (2, 2, -2, -2);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, _autoMode ? tr ("Auto", "Auto mode") : tr ("S/A", "Semiauto mode"));
}


void StageControl::setRunning (bool running)
{
    _inHandleState = true;
    _running = running;
    if (_running) {
        _start->setEnabled (false);
        _stop->setEnabled (true);
    }
    else {
        _start->setEnabled (true);
        _stop->setEnabled (false);
    }
    _inHandleState = false;
}


void StageControl::startClicked ()
{
    if (_inHandleState)
        return;

    startPressed (_number);
}


void StageControl::stopClicked ()
{
    if (_inHandleState)
        return;

    stopPressed (_number);
}
