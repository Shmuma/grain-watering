#include <QtGui>

#include "tableform.h"
#include "ui_tableform.h"


// --------------------------------------------------
// TableForm
// --------------------------------------------------
TableForm::TableForm (QWidget* parent, const QString& caption, const QString& label, 
                      const QString& key, const QString& val, bool calc)
    : QDialog (parent)
{
    QStringList list;

    setupUi (this);

    setWindowTitle (caption);
    tableLabel->setText (label);

    list += key;
    list += val;
    tableWidget->setHeaderLabels (list);

    // signals
    connect (okButton, SIGNAL (clicked ()), this, SLOT (okButtonClicked ()));
    connect (cancelButton, SIGNAL (clicked ()), this, SLOT (cancelButtonClicked ()));
    connect (addButton, SIGNAL (clicked ()), this, SLOT (addButtonClicked ()));
    connect (removeButton, SIGNAL (clicked ()), this, SLOT (removeButtonClicked ()));
    connect (tableWidget, SIGNAL (currentItemChanged (QTreeWidgetItem*, QTreeWidgetItem*)), 
             this, SLOT (tableItemChanged (QTreeWidgetItem*, QTreeWidgetItem*)));
    connect (clearTableButton, SIGNAL (clicked ()), this, SLOT (clearTableButtonClicked ()));
    connect (calculateButton, SIGNAL (clicked ()), this, SLOT (calculateButtonClicked ()));

    calculateButton->setEnabled (calc);
}


void TableForm::okButtonClicked ()
{
    _res = dataFromTable ();
    accept ();
}


void TableForm::cancelButtonClicked ()
{
    reject ();
}


void TableForm::addButtonClicked ()
{
    QStringList l;
    bool ok;

    // check for value
    keyEdit->text ().toUInt (&ok);

    if (!ok) {
        QMessageBox::warning (this, tr ("Key error"), tr ("Key is incorrect"));
        return;
    }

    valueEdit->text ().toDouble (&ok);

    if (!ok) {
        QMessageBox::warning (this, tr ("Value error"), tr ("Value is incorrect"));
        return;
    }

    // check for duplicates
    QList<QTreeWidgetItem*> dupl = tableWidget->findItems (keyEdit->text (), Qt::MatchExactly, 0);

    if (dupl.size () > 0) {
        if (QMessageBox::question (this, tr ("Key duplicates"), 
                                   tr ("Key you entered is already present in map. Do you want to replace old value?"),
                                   QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            for (int i = 0; i < dupl.size (); i++)
                delete dupl[i];
        }
        else
            return;
    }

    l += keyEdit->text ();
    l += valueEdit->text ();
    new QTreeWidgetItem (tableWidget, l);

    keyEdit->clear ();
    valueEdit->clear ();

    keyEdit->setFocus (Qt::TabFocusReason);
}


void TableForm::removeButtonClicked ()
{
    if (tableWidget->currentItem ())
        delete tableWidget->currentItem ();
}


void TableForm::tableItemChanged (QTreeWidgetItem* cur, QTreeWidgetItem*)
{
    removeButton->setEnabled (cur != NULL);
}


void TableForm::setData (const QMap<int, double>& data)
{
    QMap<int, double>::const_iterator it = data.begin ();

    tableWidget->clear ();

    while (it != data.end ()) {
        QStringList l;

        l += QString::number (it.key ());
        l += QString::number (it.value ());
        new QTreeWidgetItem (tableWidget, l);
        
        it++;
    }
}


void TableForm::clearTableButtonClicked ()
{
    tableWidget->clear ();
}


QMap<int, double> TableForm::dataFromTable () const
{
    QMap<int, double> res;

    for (int i = 0; i < tableWidget->topLevelItemCount (); i++) {
        int k = tableWidget->topLevelItem (i)->data (0, Qt::DisplayRole).toString ().toUInt (NULL);
        double v = tableWidget->topLevelItem (i)->data (1, Qt::DisplayRole).toString ().toDouble ();
        res[k] = v;
    }

    return res;
}



void TableForm::calculateButtonClicked ()
{
    QMap<int, double> data = dataFromTable ();
    int from, i, j;
    double a, b;

    from = -1;

    for (i = 0; i <= 255; i++) {
        if (data.find (i) != data.end ()) {
            if (from < 0)
                from = i;
            else {
                a = data[from];
                b = data[i];
                
                for (j = from+1; j < i; j++)
                    data[j] = ((b-a)/(i-from)) * (j-from) + a;
                from = i;
            }
        }
    }
    
    setData (data);
}
