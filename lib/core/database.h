#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <QtSql>


class Database : public QObject
{
    Q_OBJECT

private:
    QSqlDatabase _db;

public:
    Database (const QString& file) throw (QString&);
    ~Database ();

    void appendCommandHistory (const QString& cmd, const QString& res, bool ok);
};


#endif
