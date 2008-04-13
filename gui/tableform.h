#ifndef __TABLEFORM_H__
#define __TABLEFORM_H__

#include <QtGui>
#include "ui_tableform.h"


class TableForm : public QDialog, private  Ui::TableForm
{
    Q_OBJECT
private:
public:
    TableForm (QWidget* parent, const QString& caption, const QString& label, const QString& key, const QString& val);
};


#endif
