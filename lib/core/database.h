#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <QtSql>

#include "settings.h"


class Database : public QObject
{
    Q_OBJECT

private:
    QSqlDatabase _db;

public:
    Database (const QString& file) throw (QString&);
    ~Database ();

    void appendCommandHistory (const QString& cmd, const QString& res, bool ok);

    QString getStageSettings (int stage);
    void setStageSettings (int stage, const QString& sett);
    void setPass (const QString& user, const QString& pass);
    QString getPass ();

    QList< QPair <time_t, double> > getHistory (int stage, int param, int from, int to);
    void addHistory (int stage, int param, int time, double val);
};


#endif
