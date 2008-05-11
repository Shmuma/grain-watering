#include <QtGui>
#include "bsucontrol.h"


// --------------------------------------------------
// BSUControl
// --------------------------------------------------
BSUControl::BSUControl (QWidget* parent)
    : QWidget (parent),
      _waterPressure (0),
      _cleaning (false),
      _img (QString (":/stages/svg/filter.png"), "PNG"),
      _svg (QString (":/stages/svg/filter.svg"), this)
{
}


void BSUControl::paintEvent (QPaintEvent* event)
{
    QPainter p (this);
    QRectF r;

    p.drawPixmap (QPoint (0, 0), _img);

    p.setFont (QFont ("Arial", 15));
    r = _svg.boundsOnElement ("StateArea").adjusted (2, 2, -2, -2);

    if (_cleaning) {
        QPen pen (p.pen ());
        pen.setColor (Qt::red);
        p.setPen (pen);
        p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("Cleaning"));
    }
    else
        p.drawText (r, Qt::AlignHCenter | Qt::AlignVCenter, tr ("%1 bar").arg (QString ().sprintf ("%.2f", waterPressure ())));

}
