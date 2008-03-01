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
