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
    _flow = _humidity = _nature = _temp = _waterFlow = _targetHumidity = 0;
}


void StageControl::paintEvent (QPaintEvent* event)
{
    QPainter p (this);
    QRect r;
    QPoint pt, pt2;

    p.setFont (QFont ("Arial", 15));
    if (_enabled)
        p.setPen (Qt::black);
    else
        p.setPen (Qt::lightGray);

    p.drawText (QRect (0, 0, geometry ().width (), 20), Qt::AlignHCenter, tr ("Stage %1").arg (_number));

    r.setRect (10, 50, (geometry ().width () / 2)-10, 120);
    p.drawRect (r);

    pt = QPoint (r.topLeft ().x () + r.width () / 3, r.topLeft ().y ());
    p.drawLine (pt, pt + QPoint (0, -10));
    pt2 = pt + QPoint (r.width () / 3, 0);
    p.drawLine (pt2, pt2 + QPoint (0, -10));

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

    // water gate rect
    r.setRect (geometry ().width () / 2, 180, (geometry ().width ()/2)-10, 80);
    p.drawRect (r);
    p.setFont (QFont ("Arial", 13));
    p.drawText (r.adjusted (5, 5, -5, 55-90), Qt::AlignHCenter, tr ("Water gate").replace (' ', '\n'));
    p.setFont (QFont ("Arial", 11));
    p.drawText (r.adjusted (5, 55, -5, -5), Qt::AlignHCenter, tr ("Water flow"));

    p.drawText (r.adjusted (r.width ()/3, r.height ()+5, 0, 20), Qt::AlignHCenter, tr ("%1 l/h").arg (_waterFlow));

    r = QRect (10, 290, geometry ().width ()-20, 40);
    p.drawRect (r);
    p.drawText (r.adjusted (15, 5, -5, -5), Qt::AlignLeft | Qt::AlignVCenter, tr ("BSU"));
    r.translate (0, 35);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("Given grain humidity"));
    r.adjust (r.width ()/2, 20, 0, 20);
    p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 %").arg (_targetHumidity));
}
