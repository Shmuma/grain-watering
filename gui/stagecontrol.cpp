#include <QtGui>
#include "stagecontrol.h"


// --------------------------------------------------
// StageControl
// --------------------------------------------------
StageControl::StageControl (QWidget* parent)
    : QWidget (parent)
{
    _number = 0;
    _enabled = false;
    _flow = _humidity = _nature = _temp = 0;
}


void StageControl::paintEvent (QPaintEvent* event)
{
    QPainter p (this);
    QRect r;

    p.setFont (QFont ("Arial", 15));
    if (_enabled)
        p.setPen (Qt::black);
    else
        p.setPen (Qt::lightGray);

    p.drawText (QRect (0, 0, geometry ().width (), 20), Qt::AlignHCenter, tr ("Stage %1").arg (_number));

    r.setRect (10, 50, (geometry ().width () / 2)-10, 120);
    p.drawRect (r);
    p.setFont (QFont ("Arial", 13));
    p.drawText (r.adjusted (5, 5, -5, 30-120), Qt::AlignHCenter, tr ("Sensor"));
    p.setFont (QFont ("Arial", 11));
    p.drawText (r.adjusted (5, 35, -5, 50-120), Qt::AlignLeft, tr ("Grain flow"));
    p.drawText (r.adjusted (5, 55, -5, 70-120), Qt::AlignLeft, tr ("Humidity"));
    p.drawText (r.adjusted (5, 75, -5, 90-120), Qt::AlignLeft, tr ("Nature"));
    p.drawText (r.adjusted (5, 95, -5, 0),      Qt::AlignLeft, tr ("Temperature"));

    r.setRect (geometry ().width () / 2 + 10, 85, geometry ().width () / 2, 90);
    p.drawText (r.adjusted (5, 0,  -5, 20-90), Qt::AlignLeft, tr ("%1 t/h").arg (_flow));
    p.drawText (r.adjusted (5, 20, -5, 40-90), Qt::AlignLeft, tr ("%1 %").arg (_humidity));
    p.drawText (r.adjusted (5, 40, -5, 60-90), Qt::AlignLeft, tr ("%1 g/l").arg (_nature));
    p.drawText (r.adjusted (5, 60, -5, 0),     Qt::AlignLeft, tr ("%1 C").arg (_temp));
}

