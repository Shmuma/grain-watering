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
