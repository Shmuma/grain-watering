#ifndef __BSUCONTROL_H__
#define __BSUCONTROL_H__

#include <QtGui>

class BSUControl : public QWidget
{
    Q_OBJECT
private:
    double _waterPressure;

protected:
    void paintEvent (QPaintEvent* event);

public:
    BSUControl (QWidget* parent);

    void setWaterPressure (double press)
        { _waterPressure = press; update (); };
    double waterPressure () const
        { return _waterPressure; };
};


#endif
