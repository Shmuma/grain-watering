#include <QtGui>

#include "tableform.h"
#include "ui_tableform.h"


// --------------------------------------------------
// TableForm
// --------------------------------------------------
TableForm::TableForm (QWidget* parent, const QString& caption, const QString& label, const QString& key, const QString& val)
    : QDialog (parent)
{
    QStringList list;

    setupUi (this);

    setWindowTitle (caption);
    tableLabel->setText (label);

    list += key;
    list += val;
    tableWidget->setHeaderLabels (list);
}

