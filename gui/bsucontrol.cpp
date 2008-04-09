#include <QtGui>
#include "bsucontrol.h"


// --------------------------------------------------
// BSUControl
// --------------------------------------------------
BSUControl::BSUControl (QWidget* parent)
    : QWidget (parent),
      _waterPressure (0)
{
    
}


void BSUControl::paintEvent (QPaintEvent* event)
{
    QPainter p (this);
    QRect r;
    QPoint pt;

    p.setFont (QFont ("Arial", 15));
    p.setPen (Qt::black);
    
    p.drawText (QRect (0, 30, geometry ().width (), 20), Qt::AlignHCenter, tr ("Filter block"));

    r.setRect (20, 20, geometry ().width ()-40, geometry ().height ()-40);
    p.drawRect (r);

    p.setFont (QFont ("Arial", 12));

    r.setRect (30, geometry ().height ()/2, geometry ().width ()/2 - 30, 20);

    p.drawText (r, Qt::AlignLeft, tr ("Pressure"));
    p.drawText (r.translated (geometry ().width ()/2-30, 0), Qt::AlignLeft, tr ("%1 bar").arg (QString ().sprintf ("%.2f", _waterPressure)));

    pt = QPoint (0, geometry ().height ()/2 - 30);
    p.drawLine (pt, pt + QPoint (20, 0));
    p.drawLine (pt + QPoint (0, 60), pt + QPoint (20, 60));

    pt = QPoint (geometry ().width ()-20, geometry ().height ()/2 - 30);
    p.drawLine (pt, pt + QPoint (20, 0));
    p.drawLine (pt + QPoint (0, 60), pt + QPoint (20, 60));
}
