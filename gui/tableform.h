#ifndef __TABLEFORM_H__
#define __TABLEFORM_H__

#include <QtGui>
#include "ui_tableform.h"


class TableForm : public QDialog, private  Ui::TableForm
{
    Q_OBJECT
private:
    QMap<int, double> _res;

    QMap<int, double> dataFromTable () const;

public:
    TableForm (QWidget* parent, const QString& caption, const QString& label, const QString& key, const QString& val, bool calc);

    QMap<int, double> result () const
        { return _res; };

    void setData (const QMap<int, double>& data);

protected slots:
    void okButtonClicked ();
    void cancelButtonClicked ();
    void addButtonClicked ();
    void removeButtonClicked ();
    void tableItemChanged (QTreeWidgetItem* cur, QTreeWidgetItem* prev);
    void clearTableButtonClicked ();
    void calculateButtonClicked ();
};


#endif
