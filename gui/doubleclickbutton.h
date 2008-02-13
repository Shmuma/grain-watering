#ifndef __DOUBLECLICKBUTTON_H__
#define __DOUBLECLICKBUTTON_H__

#include <QtGui>


class DoubleClickButton : public QPushButton
{
    Q_OBJECT
private:

protected:
    virtual void mouseDoubleClickEvent (QMouseEvent * event);
    virtual void mousePressEvent (QMouseEvent *event);

public:
    DoubleClickButton (QWidget* parent);
};


#endif
