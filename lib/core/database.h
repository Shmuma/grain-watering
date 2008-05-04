#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <QtSql>

#include "settings.h"
#include "history.h"


class Database : public QObject
{
    Q_OBJECT

public:
    enum setting_t {
        S_TempK = 0,
        S_TempResist,
    };

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
    void addHistory (history_stage_t stage, history_kind_t param, int time, double val);

    QList< QPair <time_t, QString> > getEvents (int from, int to);

    void setTempCoef (double k, double res);

    double getTempK () const;
    double getTempResist () const;

    void logMessage (const QString& msg);
};


#endif
