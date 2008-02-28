#include <QtGui>

#include "doubleclickbutton.h"


// --------------------------------------------------
// DoubleClickButton
// --------------------------------------------------
DoubleClickButton::DoubleClickButton (QWidget* parent)
    : QPushButton (parent)
{
}


void DoubleClickButton::mouseDoubleClickEvent (QMouseEvent*)
{
    doubleClicked ();
}


void DoubleClickButton::mousePressEvent (QMouseEvent*)
{
}
