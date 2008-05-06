#include <QtSql>

#include "database.h"



// --------------------------------------------------
// Database
// --------------------------------------------------
Database::Database (const QString& file) throw (QString&)
    : QObject (),
      _db (QSqlDatabase::addDatabase ("QSQLITE"))
{
    _db.setDatabaseName (file);
    if (!_db.open ())
        throw _db.lastError ().text ();

    // analyse schema and create missing tables
    QStringList tables = _db.tables ();

    if (!tables.contains ("commands"))
        _db.exec ("create table commands (cmd_date integer, cmd_text text, ok integer, cmd_res text);");
    if (!tables.contains ("stage_settings"))
        _db.exec ("create table stage_settings (stage integer, settings text);");
    if (!tables.contains ("users")) {
        _db.exec ("create table users (user text, pass text);");
        _db.exec ("insert into  users (user, pass) values ('admin', 'admin');");
        _db.exec ("insert into  users (user, pass) values ('config', 'config');");
    }
    if (!tables.contains ("history")) {
        _db.exec ("create table history (stage integer, hist integer, time integer, val double);");
        _db.exec ("create index history_1 on history (stage);");
        _db.exec ("create index history_2 on history (hist);");
        _db.exec ("create index history_3 on history (time);");
    }
    if (!tables.contains ("settings")) {
        _db.exec ("create table settings (key integer, value text);");
        _db.exec (QString ().sprintf ("insert into settings (key, value) values ('%d', '0.0097');", S_TempK));
        _db.exec (QString ().sprintf ("insert into settings (key, value) values ('%d', '992');", S_TempResist));
        _db.exec (QString ().sprintf ("insert into settings (key, value) values ('%d', '2.0');", S_MinPress));
    }
    if (!tables.contains ("messages")) {
        _db.exec ("create table messages (date integer, msg text);");
    }
    if (!tables.contains ("clean_messages")) {
        _db.exec ("create table clean_messages (date integer, msg text);");
    }
}


Database::~Database ()
{
    QSqlDatabase::database ().close ();
}


void Database::appendCommandHistory (const QString& cmd, const QString& res, bool ok)
{
    QSqlQuery query (QSqlDatabase::database ());
    query.prepare ("insert into commands (cmd_date, cmd_text, ok, cmd_res) values (:date, :text, :ok, :res)");
    query.bindValue (":date", QDateTime::currentDateTime ().toTime_t ());
    query.bindValue (":text", cmd);
    query.bindValue (":ok", ok ? 1 : 0);
    query.bindValue (":res", res);
    if (!query.exec ())
        printf ("DB error: %s\n", QSqlDatabase::database ().lastError ().text ().toAscii ().constData ());
}


QString Database::getStageSettings (int stage)
{
    QSqlQuery query (QSqlDatabase::database ());

    query.prepare ("select settings from stage_settings where stage = ?");
    query.addBindValue (stage);
   
    if (!query.exec () || !query.next ())
        return QString ("invalid");
    else
        return query.value (0).toString ();
}


void Database::setStageSettings (int stage, const QString& sett)
{
    QSqlQuery query (QSqlDatabase::database ());

    query.prepare ("select count(*) from stage_settings where stage = ?");
    query.addBindValue (stage);

    if (!query.exec () || !query.next ()) {
        printf ("DB error: %s\n", QSqlDatabase::database ().lastError ().text ().toAscii ().constData ());
        return;
    }

    if (query.value (0).toUInt ()) {
        query.prepare ("update stage_settings set settings = ? where stage = ?");
        query.addBindValue (sett);
        query.addBindValue (stage);
        if (!query.exec ())
            printf ("DB error: %s\n", QSqlDatabase::database ().lastError ().text ().toAscii ().constData ());
    }
    else {
        query.prepare ("insert into stage_settings (stage, settings) values (?, ?)");
        query.addBindValue (stage);
        query.addBindValue (sett);
        if (!query.exec ())
            printf ("DB error: %s\n", QSqlDatabase::database ().lastError ().text ().toAscii ().constData ());
    }
}


void Database::setPass (const QString& user, const QString& pass)
{
    QSqlQuery query (QSqlDatabase::database ());

    query.prepare ("update users set pass = ? where user = ?");
    query.addBindValue (pass);
    query.addBindValue (user);
    query.exec ();
}


QString Database::getPass ()
{
    QSqlQuery query (QSqlDatabase::database ());
    QString res;

    query.prepare ("select user, pass from users");
    if (!query.exec ())
        return QString ();

    while (query.next ()) {
        if (!res.isEmpty ())
            res += ",";
        res += query.value (0).toString () + "=" + query.value (1).toString ();
    }

    return res;
}



QList< QPair <time_t, double> > Database::getHistory (int stage, int param, int from, int to)
{
    QList< QPair <time_t, double> > res;
    QSqlQuery query (QSqlDatabase::database ());

    query.prepare ("select time, val from history where stage = :stage and hist = :param and time >= :from and time <= :to");
    query.bindValue (":stage", stage);
    query.bindValue (":param", param);
    query.bindValue (":from", from);
    query.bindValue (":to", to);
    if (!query.exec ())
        return res;

    while (query.next ())
        res.push_back (QPair<time_t, double> (query.value (0).toInt (), query.value (1).toDouble ()));

    return res;
}


void Database::addHistory (history_stage_t stage, history_kind_t param, int time, double val)
{
    QSqlQuery query (QSqlDatabase::database ());
    query.prepare ("insert into history (stage, hist, time, val) values (:stage, :hist, :time, :val)");
    query.bindValue (":stage", (int)stage);
    query.bindValue (":hist", (int)param);
    query.bindValue (":time", time);
    query.bindValue (":val", val);
    if (!query.exec ())
        printf ("DB error: %s\n", QSqlDatabase::database ().lastError ().text ().toAscii ().constData ());
}


void Database::setTempCoef (double k, double res)
{
    QSqlQuery query (QSqlDatabase::database ());

    query.prepare ("update settings set value = :val where key = :key");
    query.bindValue (":val", QString::number (k));
    query.bindValue (":key", S_TempK);
    query.exec ();

    query.prepare ("update settings set value = :val where key = :key");
    query.bindValue (":val", QString::number (res));
    query.bindValue (":key", S_TempResist);
    query.exec ();
}


double Database::getTempK () const
{
    QSqlQuery query (QSqlDatabase::database ());
    QString res;
    double def = 0.0097;

    query.prepare ("select value from settings where key = :key");
    query.bindValue (":key", S_TempK);
    if (!query.exec ())
        return def;

    if (!query.next ())
        return def;

    return query.value (0).toDouble ();
}


double Database::getTempResist () const
{
    QSqlQuery query (QSqlDatabase::database ());
    QString res;
    double def = 997;

    query.prepare ("select value from settings where key = :key");
    query.bindValue (":key", S_TempResist);

    if (!query.exec ())
        return def;

    if (!query.next ())
        return def;
    
    return query.value (0).toDouble ();
}


void Database::logMessage (const QString& msg)
{
    QSqlQuery query (QSqlDatabase::database ());
    query.prepare ("insert into messages (date, msg) values (:date, :msg)");
    query.bindValue (":date", QDateTime::currentDateTime ().toTime_t ());
    query.bindValue (":msg", msg);
    if (!query.exec ())
        printf ("DB error: %s\n", QSqlDatabase::database ().lastError ().text ().toAscii ().constData ());   
}


QList< QPair <time_t, QString> > Database::getEvents (bool clean, int from, int to)
{
    QList< QPair <time_t, QString> > res;
    QSqlQuery query (QSqlDatabase::database ());

    if (clean)
        query.prepare ("select date, msg from clean_messages where date >= :from and date <= :to");
    else
        query.prepare ("select date, msg from messages where date >= :from and date <= :to");
    query.bindValue (":from", from);
    query.bindValue (":to", to);
    if (!query.exec ()) {
        printf ("DB error: %s\n", QSqlDatabase::database ().lastError ().text ().toAscii ().constData ());
        return res;
    }

    while (query.next ())
        res.push_back (QPair<time_t, QString> (query.value (0).toInt (), query.value (1).toString ().replace (',', ';')));

    return res;
}


void Database::logCleanMessage (const QString& msg)
{
    QSqlQuery query (QSqlDatabase::database ());
    query.prepare ("insert into clean_messages (date, msg) values (:date, :msg)");
    query.bindValue (":date", QDateTime::currentDateTime ().toTime_t ());
    query.bindValue (":msg", msg);
    if (!query.exec ())
        printf ("DB error: %s\n", QSqlDatabase::database ().lastError ().text ().toAscii ().constData ());   
}


double Database::getMinPressure ()
{
    QSqlQuery query (QSqlDatabase::database ());
    QString res;
    double def = 2.0;

    query.prepare ("select value from settings where key = :key");
    query.bindValue (":key", S_MinPress);
    if (!query.exec ())
        return def;

    if (!query.next ())
        return def;

    return query.value (0).toDouble ();
}


void Database::setMinPressure (double val)
{
    QSqlQuery query (QSqlDatabase::database ());

    query.prepare ("update settings set value = :val where key = :key");
    query.bindValue (":val", QString::number (val));
    query.bindValue (":key", S_MinPress);
    query.exec ();
}


