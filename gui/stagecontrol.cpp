#include <QtGui>
#include <QtSvg>
#include "stagecontrol.h"
#include "settings.h"


// --------------------------------------------------
// StageControl
// --------------------------------------------------
StageControl::StageControl (QWidget* parent)
    : QWidget (parent)
{
    _img[StageSettings::M_Auto] = QPixmap (QString (":/stages/svg/stage-auto.png"), "PNG");
    _img[StageSettings::M_SemiAuto] = QPixmap (QString (":/stages/svg/stage-semi.png"), "PNG");
    _img[StageSettings::M_Fixed] = QPixmap (QString (":/stages/svg/stage-fixed.png"), "PNG");

    _svg[StageSettings::M_Auto] = new QSvgRenderer (QString (":/stages/svg/stage-auto.svg"), this);
    _svg[StageSettings::M_SemiAuto] = new QSvgRenderer (QString (":/stages/svg/stage-semi.svg"), this);
    _svg[StageSettings::M_Fixed] = new QSvgRenderer (QString (":/stages/svg/stage-fixed.svg"), this);

    _mode = StageSettings::M_Auto;
    _number = 0;
    _grainState = GS_GrainMissing;
    _waterPresent = false;
    _targetWaterFlow = _flow = _humidity = _nature = _temp = _waterFlow = _targetHumidity = 0;
    _label = QString ();
    setEnabled (false);
    _cleaning = false;
    _minWaterFlow = 0;
    _maxWaterFlow = 1e10;

    // load images for buttons
    _startPressed = _stopPressed = false;

    _startImages[0] = QPixmap (":/stages/buttons/start-normal.png");
    _startImages[1] = QPixmap (":/stages/buttons/start-pressed.png");
    _startImages[2] = QPixmap (":/stages/buttons/start-disabled.png");

    _stopImages[0] = QPixmap (":/stages/buttons/stop-normal.png");
    _stopImages[1] = QPixmap (":/stages/buttons/stop-pressed.png");
    _stopImages[2] = QPixmap (":/stages/buttons/stop-disabled.png");

    // load water pixmaps

    _waterCounter = 0;
    for (int i = 0; i < 3; i++)
        _waterImages[i] = QPixmap (QString (":/stages/svg/water-%1.png").arg (i));
    _waterTimer = 0;

    _waternoCounter = 0;
    for (int i = 0; i < 2; i++)
        _waternoImages[i] = QPixmap (QString (":/stages/svg/water-no%1.png").arg (i));
    _waternoTimer = 0;

    // load grain pixmaps

    _grainCounter = 0;
    for (int i = 0; i < 3; i++)
        _grainImages[i] = QPixmap (QString (":/stages/svg/grain-%1.png").arg (i));
    _grainTimer = 0;

    _grainlowCounter = 0;
    for (int i = 0; i < 3; i++)
        _grainlowImages[i] = QPixmap (QString (":/stages/svg/grain-low%1.png").arg (i));
    _grainlowTimer = 0;

    _grainnoCounter = 0;
    for (int i = 0; i < 2; i++)
        _grainnoImages[i] = QPixmap (QString (":/stages/svg/grain-no%1.png").arg (i));
    _grainnoTimer = 0;


    // snek 
    _snekCounter = 0;
    for (int i = 0; i < 4; i++)
        _snekImages[i] = QPixmap (QString (":/stages/svg/snek-%1.png").arg (i));
    _snekTimer = 0;

    QRect r;

    _startRect = _svg[_mode]->boundsOnElement ("StartButton").toRect ();
    _stopRect =  _svg[_mode]->boundsOnElement ("StopButton").toRect ();

    setRunning (false);

    // humidity buttons
    _humidityUp = new QToolButton (this);
    _humidityDown = new QToolButton (this);
    _humidityUp->setText ("+");
    _humidityDown->setText ("-");
    r = _svg[_mode]->boundsOnElement ("HumiditySpin").toRect ();
    _humidityUp->setGeometry (r.adjusted (0, 0, 0, -r.height () / 2));
    _humidityDown->setGeometry (r.adjusted (0, r.height () / 2, 0, 0));

    connect (_humidityUp, SIGNAL (clicked ()), this, SLOT (humidityUpClicked ()));
    connect (_humidityDown, SIGNAL (clicked ()), this, SLOT (humidityDownClicked ()));

    // water flow buttons
    _waterUp = new QToolButton (this);
    _waterDown = new QToolButton (this);
    _waterUp->setText ("+");
    _waterDown->setText ("-");
    r = _svg[_mode]->boundsOnElement ("FlowSpin").toRect ();
    _waterUp->setGeometry (r.adjusted (0, 0, 0, -r.height () / 2));
    _waterDown->setGeometry (r.adjusted (0, r.height () / 2, 0, 0));

    connect (_waterUp, SIGNAL (clicked ()), this, SLOT (waterUpClicked ()));
    connect (_waterDown, SIGNAL (clicked ()), this, SLOT (waterDownClicked ()));

    // water flow editor
    _humidityEdit = new QLineEdit (this);
    _humidityEdit->setFont (QFont ("Verdana", 7));

    r = _svg[_mode]->boundsOnElement ("WaterFlow").toRect ();
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
    p.drawPixmap (QPoint (0, 0), _img[_mode]);

    // draw buttons
    if (_running) {
        p.drawPixmap (_startRect.topLeft (), _startImages[2]);
        p.drawPixmap (_stopRect.topLeft (), _stopImages[_stopPressed ? 1 : 0]);
    }
    else {
        p.drawPixmap (_startRect.topLeft (), _startImages[_startPressed ? 1 : 0]);
        p.drawPixmap (_stopRect.topLeft (), _stopImages[2]);       
    }

    if (_mode != StageSettings::M_SemiAuto) {
        setColor (p, "#E7B953");
        p.setFont (QFont ("Arial", 14));
        r = _svg[_mode]->boundsOnElement ("GrainFlow").adjusted (2, 2, -2, -2);
        p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 t/h").arg (QString ().sprintf ("%.2f", _flow)));
        
        setColor (p, "#C1E8FB");
	p.setFont (QFont ("Verdana", 9));
	r = _svg[_mode]->boundsOnElement ("TargetHumidity").adjusted (1, 1, -1, -1);
	p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 %").arg (QString ().sprintf ("%.1f", _targetHumidity)));

	setColor (p, "#F1FAFE");
	p.setFont (QFont ("Verdana", 9));
	r = _svg[_mode]->boundsOnElement ("WaterFlow").adjusted (2, 2, -2, -2);
	p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1").arg (QString ().sprintf ("%.0f", _waterFlow)));
    }

    if (_mode == StageSettings::M_Auto) {
        setColor (p, "#E7B953");
        p.setFont (QFont ("Arial", 14));
        r = _svg[_mode]->boundsOnElement ("GrainHumidity").adjusted (2, 2, -2, -2);
        p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 %").arg (QString ().sprintf ("%.2f", _humidity)));

        setColor (p, "#E7B953");
        p.setFont (QFont ("Arial", 12));
        r = _svg[_mode]->boundsOnElement ("GrainTemperature").adjusted (2, 2, -2, -2);
        p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 C").arg (QString ().sprintf ("%.0f", _temp)));

        p.setFont (QFont ("Arial", 12));
        r = _svg[_mode]->boundsOnElement ("GrainNature").adjusted (2, 2, -2, -2);
        p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 g/l").arg (QString ().sprintf ("%.2f", _nature)));
    
        setColor (p, "#C1E8FB");
        p.setFont (QFont ("Verdana", 9));
        r = _svg[_mode]->boundsOnElement ("HumidityDelta").adjusted (2, 2, -2, -2);
        p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1\n%").arg (QString ().sprintf ("%.1f", _targetHumidity - _humidity)));
    }


    // grain area
    r = _svg[_mode]->boundsOnElement ("GrainArea").adjusted (0, 0, 0, 0);

    switch (_grainState) {
    case GS_GrainPresent:
        p.drawPixmap (r.topLeft (), _grainImages[_grainCounter]);
        break;
    case GS_GrainMissing:
        p.drawPixmap (r.topLeft (), _grainnoImages[_grainnoCounter]);
        break;
    case GS_GrainLow:
        p.drawPixmap (r.topLeft (), _grainlowImages[_grainlowCounter]);
        break;
    }

    // water area
    r = _svg[_mode]->boundsOnElement ("WaterArea").adjusted (1, 0, 0, 0);
    if (_waterPresent) {
        p.drawPixmap (r.topLeft (), _waterImages[_waterCounter]);
    }
    else {
        p.drawPixmap (r.topLeft (), _waternoImages[_waternoCounter]);
    }

    // snek area
    r = _svg[_mode]->boundsOnElement ("SnekArea").adjusted (-1, -1, -1, -1);
    p.drawPixmap (r.topLeft (), _snekImages[_snekCounter]);

    // stage mode
    p.setFont (QFont ("Arial", 12));
    setColor (p, "#626262");
    r = _svg[_mode]->boundsOnElement ("StageMode").adjusted (2, 2, -2, -2);
    QString modeLabel;

    switch (_mode) {
    case StageSettings::M_Auto:
        modeLabel = tr ("Auto", "Auto mode");
        break;
    case StageSettings::M_SemiAuto:
        modeLabel = tr ("S/A", "Semiauto mode");
        break;
    case StageSettings::M_Fixed:
        modeLabel = tr ("Fixed", "Fixed humidity mode");
        break;
    }

    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, modeLabel);

    p.setFont (QFont ("Arial", 14));
    r = _svg[_mode]->boundsOnElement ("StageTitle").adjusted (2, 2, -2, -2);
    if (_cleaning) {
        QPen pen (p.pen ());
        pen.setColor (Qt::red);
        p.setPen (pen);
    }      
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("Stage %1: %2").arg (_number+1).arg (_cleaning ? tr ("Cleaning") : _label));
}


void StageControl::setRunning (bool running)
{
    _running = running;
    _waterPresent = running;
     update ();

     if (running) {
         if (_snekTimer)
             killTimer (_snekTimer);
         _snekTimer = startTimer (180);
         if (_waterTimer)
             killTimer (_waterTimer);
         _waterTimer = startTimer (250);
         if (_waternoTimer)
             killTimer (_waternoTimer);
         _waternoTimer = startTimer (1000);
         if (_grainTimer)
             killTimer (_grainTimer);
         _grainTimer = startTimer (280);
         if (_grainlowTimer)
             killTimer (_grainlowTimer);
         _grainlowTimer = startTimer (280);
         if (_grainnoTimer)
             killTimer (_grainnoTimer);
         _grainnoTimer = startTimer (1000);
         if (_updateTimer)
             killTimer (_updateTimer);
         _updateTimer = startTimer (250);
     }
     else {
         if (_snekTimer)
             killTimer (_snekTimer);
         _snekTimer = 0;
         if (_waterTimer)
             killTimer (_waterTimer);
         _waterTimer = 0;
         if (_waternoTimer)
             killTimer (_waternoTimer);
         _waternoTimer = 0;
         if (_grainTimer)
             killTimer (_grainTimer);
         _grainTimer = 0;
         if (_grainlowTimer)
             killTimer (_grainlowTimer);
         _grainlowTimer = 0;
         if (_grainnoTimer)
             killTimer (_grainnoTimer);
         _grainnoTimer = 0;
         if (_updateTimer)
             killTimer (_updateTimer);
         _updateTimer = 0;
     }
}


void StageControl::timerEvent (QTimerEvent* e)
{
    if (e->timerId () == _snekTimer) {
	    _snekCounter += 1;
    	    _snekCounter %= 4;
    }
    if (e->timerId () == _waterTimer) {
	    _waterCounter += 1;
    	    _waterCounter %= 3;
    }
    if (e->timerId () == _waternoTimer) {
	    _waternoCounter += 1;
    	    _waternoCounter %= 2;
    }
    if (e->timerId () == _grainTimer) {
	    _grainCounter += 1;
    	    _grainCounter %= 3;
    }
    if (e->timerId () == _grainlowTimer) {
	    _grainlowCounter += 1;
    	    _grainlowCounter %= 3;
    }
    if (e->timerId () == _grainnoTimer) {
	    _grainnoCounter += 1;
    	    _grainnoCounter %= 2;
    }
    if (e->timerId () == _updateTimer)
        update ();
}


void StageControl::startClicked ()
{
    startPressed (_number);
}


void StageControl::stopClicked ()
{
    stopPressed (_number);
}


void StageControl::humidityUpClicked ()
{
    _targetHumidity += 0.1;
    targetHumidityUpdated (_number, _targetHumidity);
    update ();
}


void StageControl::humidityDownClicked ()
{
    _targetHumidity -= 0.1;
    if (_targetHumidity < 0)
        _targetHumidity = 0;
    targetHumidityUpdated (_number, _targetHumidity);
    update ();
}


void StageControl::waterUpClicked ()
{
    double val = _targetWaterFlow + 1;
    setTargetWaterFlow (val);
    targetWaterFlowUpdated (_number, val);
}


void StageControl::waterDownClicked ()
{
    double val = _targetWaterFlow - 1;
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

    if (!ok) {
        QMessageBox::warning (this, tr ("Invalid water flow value"), tr ("You've entered invalid water flow value."));
        return;
    }
    
    if (val < _minWaterFlow) {
        QMessageBox::warning (this, tr ("Invalid water flow value"), tr ("Value of water flow is less than minimal for this stage"));
        return;
    }

    if (val > _maxWaterFlow) {
        QMessageBox::warning (this, tr ("Invalid water flow value"), tr ("Value of water flow is greater than maximum for this stage"));
        return;
    }

    targetWaterFlowUpdated (_number, val);
    setTargetWaterFlow (val);
}


void StageControl::setTargetWaterFlow (double val)
{
    _targetWaterFlow = val;
    update ();

    if (_mode == StageSettings::M_SemiAuto && !_humidityEdit->hasFocus ())
        _humidityEdit->setText (QString ().sprintf ("%.0f", val));
}


void StageControl::setMode (StageSettings::mode_t mode)
{
    _mode = mode; update ();

    if (_mode != StageSettings::M_SemiAuto) {
        _waterUp->hide ();
        _waterDown->hide ();
        _humidityEdit->hide ();
        _humidityUp->show ();
        _humidityDown->show ();
    }
    else {
        _waterUp->show ();
        _waterDown->show ();
        _humidityUp->hide ();
        _humidityDown->hide ();
        _humidityEdit->show ();
    }
}


void StageControl::mousePressEvent (QMouseEvent* e)
{
    if (_startRect.contains (e->pos ()) && !_running) {
        _startPressed = true;
        update ();
    }
    else
        _startPressed = false;

    if (_stopRect.contains (e->pos ()) && _running) {
        _stopPressed = true;
        update ();
    }
    else
        _stopPressed = false;
}


void StageControl::mouseReleaseEvent (QMouseEvent* e)
{
    _startPressed = _stopPressed = false;
    update ();

    if (_startRect.contains (e->pos ()) && !_running)
        startClicked ();

    if (_stopRect.contains (e->pos ()) && _running)
        stopClicked ();
}


void StageControl::setColor (QPainter& p, const char* color)
{
    QPen pen (p.pen ());
    pen.setColor (QColor (color));
    p.setPen (pen);
}
