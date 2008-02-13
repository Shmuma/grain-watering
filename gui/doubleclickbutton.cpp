#include <QtGui>

#include "doubleclickbutton.h"


// --------------------------------------------------
// DoubleClickButton
// --------------------------------------------------
DoubleClickButton::DoubleClickButton (QWidget* parent)
    : QPushButton (parent)
{
}


void DoubleClickButton::mouseDoubleClickEvent (QMouseEvent * event)
{
    toggle ();
}


void DoubleClickButton::mousePressEvent (QMouseEvent *event)
{
}
