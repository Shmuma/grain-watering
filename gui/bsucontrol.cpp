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

    p.setFont (QFont ("Arial", 15));
    p.setPen (Qt::black);
    
    p.drawText (QRect (0, 0, geometry ().width (), 20), Qt::AlignHCenter, tr ("BSU"));

    r.setRect (20, 50, geometry ().width () - 40, geometry ().height () - 60);
    p.drawRect (r);

    p.setFont (QFont ("Arial", 12));

    r.setRect (30, 50 + (geometry ().height () - 60) / 2, geometry ().width () / 2 - 30,  20);

    p.drawText (r, Qt::AlignLeft, tr ("Pressure"));
    p.drawText (r.translated (geometry ().width () / 2 - 30, 0), Qt::AlignLeft, tr ("%1 bar").arg (QString ().sprintf ("%.2f", _waterPressure)));
}
