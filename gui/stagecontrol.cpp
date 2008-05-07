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
    _targetWaterFlow = _flow = _humidity = _nature = _temp = _waterFlow = _targetHumidity = 0;
    _label = QString ();
    setEnabled (false);
    _cleaning = false;

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

    // humidity buttons
    _humidityUp = new QToolButton (this);
    _humidityDown = new QToolButton (this);
    _humidityUp->setText ("+");
    _humidityDown->setText ("-");
    r = _svgWithSensors.boundsOnElement ("HumiditySpin").toRect ();
    _humidityUp->setGeometry (r.adjusted (0, 0, 0, -r.height () / 2));
    _humidityDown->setGeometry (r.adjusted (0, r.height () / 2, 0, 0));

    connect (_humidityUp, SIGNAL (clicked ()), this, SLOT (humidityUpClicked ()));
    connect (_humidityDown, SIGNAL (clicked ()), this, SLOT (humidityDownClicked ()));

    // water flow buttons
    _waterUp = new QToolButton (this);
    _waterDown = new QToolButton (this);
    _waterUp->setText ("+");
    _waterDown->setText ("-");
    r = _svgWithSensors.boundsOnElement ("FlowSpin").toRect ();
    _waterUp->setGeometry (r.adjusted (0, 0, 0, -r.height () / 2));
    _waterDown->setGeometry (r.adjusted (0, r.height () / 2, 0, 0));

    connect (_waterUp, SIGNAL (clicked ()), this, SLOT (waterUpClicked ()));
    connect (_waterDown, SIGNAL (clicked ()), this, SLOT (waterDownClicked ()));

    // water flow editor
    _humidityEdit = new QLineEdit (this);
//     QFont f = _humidityEdit->font ();
//     f.setPointSize (f.pointSize ()-4);
//     _humidityEdit->setFont (f);

    r = _svgWithSensors.boundsOnElement ("WaterFlow").toRect ();
    _humidityEdit->setGeometry (r);

    connect (_humidityEdit, SIGNAL (returnPressed ()), this, SLOT (humidityEditorReturnPressed ()));

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

    if (_autoMode) {
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
    }

    p.setFont (QFont ("Verdana", 9));
    r = _svgWithSensors.boundsOnElement ("WaterFlow").adjusted (2, 2, -2, -2);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1\nl/h").arg (QString ().sprintf ("%.0f", _waterFlow)));

    p.setFont (QFont ("Verdana", 9));
    r = _svgWithSensors.boundsOnElement ("TargetHumidity").adjusted (2, 2, -2, -2);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 %").arg (QString ().sprintf ("%.2f", _targetHumidity)));

    // fill grain area if grain present
    r = _svgWithSensors.boundsOnElement ("GrainArea").adjusted (1, 1, -2, -2);

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
    r = _svgWithSensors.boundsOnElement ("WaterArea").adjusted (1, 1, -2, -2);
    p.fillRect (r, QBrush (_waterPresent ? Qt::cyan : Qt::lightGray));

    // stage mode
    p.setFont (QFont ("Arial", 13));
    r = _svgWithSensors.boundsOnElement ("StageMode").adjusted (2, 2, -2, -2);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, _autoMode ? tr ("Auto", "Auto mode") : tr ("S/A", "Semiauto mode"));

    p.setFont (QFont ("Arial", 16));
    r = _svgWithSensors.boundsOnElement ("StageTitle").adjusted (2, 2, -2, -2);
    if (_cleaning) {
        QPen pen (p.pen ());
        pen.setColor (Qt::red);
        p.setPen (pen);
    }      
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("Stage %1: %2").arg (_number+1).arg (_cleaning ? tr ("Cleaning") : _label));
}


void StageControl::setRunning (bool running)
{
    _inHandleState = true;
    _running = running;
    if (_running) {
        _start->setEnabled (false);
        _stop->setEnabled (true);

        QPalette pal;

        _start->setPalette (qApp->palette (_start));

        pal = _stop->palette ();
        pal.setColor (QPalette::Background, Qt::red);
        _stop->setPalette (pal);

    }
    else {
        _start->setEnabled (true);
        _stop->setEnabled (false);

        QPalette pal = _start->palette ();
        pal.setColor (QPalette::Background, Qt::green);
        _start->setPalette (pal);

        _stop->setPalette (qApp->palette (_stop));
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


void StageControl::humidityUpClicked ()
{
    _targetHumidity += 0.05;
    targetHumidityUpdated (_number, _targetHumidity);
    update ();
}


void StageControl::humidityDownClicked ()
{
    _targetHumidity -= 0.05;
    if (_targetHumidity < 0)
        _targetHumidity = 0;
    targetHumidityUpdated (_number, _targetHumidity);
    update ();
}


void StageControl::waterUpClicked ()
{
    double val = _targetWaterFlow + 0.05;
    setTargetWaterFlow (val);
    targetWaterFlowUpdated (_number, val);
}


void StageControl::waterDownClicked ()
{
    double val = _targetWaterFlow - 0.05;
    if (val > 0) {
        setTargetWaterFlow (val);
        targetWaterFlowUpdated (_number, val);   
    }
}


void StageControl::humidityEditorReturnPressed ()
{
    bool ok;
    double val;

    val = _humidityEdit->text ().toDouble (&ok);

    if (!ok || val < 0) {
        QMessageBox::warning (this, tr ("Invalid water flow value"), tr ("You've entered nvalid water flow value."));
        return;
    }

    targetWaterFlowUpdated (_number, val);
    setTargetWaterFlow (val);
}


void StageControl::setTargetWaterFlow (double val)
{
    _targetWaterFlow = val;
    update ();

    if (!_autoMode)
        _humidityEdit->setText (QString ().sprintf ("%.02f", val));
}


void StageControl::setAutoMode (bool mode)
{
    _autoMode = mode; update ();

    if (_autoMode) {
        _humidityEdit->hide ();
        _waterUp->hide ();
        _waterDown->hide ();
    }
    else {
        _humidityEdit->show ();
        _waterUp->show ();
        _waterDown->show ();
    }
}
