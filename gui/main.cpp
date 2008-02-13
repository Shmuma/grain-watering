#include <QApplication>

#include "mainwindow.h"


int main (int argc, char** argv)
{
    QApplication app (argc, argv);
    QString locale = QLocale::system ().name ();
    QTranslator translator;

    translator.load (QString ("gui_")+locale);
    app.installTranslator (&translator);

    MainWindow win;
    win.show ();

    return app.exec ();
}
