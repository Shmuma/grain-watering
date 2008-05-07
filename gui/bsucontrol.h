#ifndef __BSUCONTROL_H__
#define __BSUCONTROL_H__

#include <QtGui>
#include <QtSvg>

class BSUControl : public QWidget
{
    Q_OBJECT
private:
    double _waterPressure;
    bool _cleaning;

    QPixmap _img;
    QSvgRenderer _svg;

protected:
    void paintEvent (QPaintEvent* event);

public:
    BSUControl (QWidget* parent);

    void setWaterPressure (double press)
        { _waterPressure = press; update (); };
    double waterPressure () const
        { return _waterPressure < 0 ? 0.0 : _waterPressure; };

    void setCleaning (bool cleaning)
        { _cleaning = cleaning; update (); };
    bool cleaning () const
        { return _cleaning; };
};


#endif
