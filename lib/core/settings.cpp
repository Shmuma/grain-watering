#include <QtCore>

#include "settings.h"



// --------------------------------------------------
// StageSettings
// --------------------------------------------------
StageSettings::StageSettings (const QString& str)
    : _targetHumidity (0.0),
      _humidityCoeff (0.0),
      _minGrainFlow (0.0),
      _waterFlowK (0.0),
      _minWaterFlow (0.0),
      _maxWaterFlow (0.0),
      _waterFormula (0),
      _valid (false)
{
    _humidityTable.clear ();
    _grainFlowTable.clear ();
    _grainNatureTable.clear ();
    _grainTempTable.clear ();
    _grainNatureCoeffTable.clear ();

    if (str.isEmpty () || str.toLower () == "invalid" || str.toLower () == "disabled")
        return;

    // parse settings data
    QStringList list = str.toLower ().split (",");

    for (int i = 0; i < list.size (); i++) {
        double val;
        bool ok;
        QStringList l = list[i].split ("=");

        if (l.size () != 2)
            continue;
        
        if (l[0] == "ht") 
            _humidityTable = parseHash (l[1]);
        else if (l[0] == "gf")
            _grainFlowTable = parseHash (l[1]);
        else if (l[0] == "gn")
            _grainNatureTable = parseHash (l[1]);
        else if (l[0] == "gt")
            _grainTempTable = parseHash (l[1]);
        else if (l[0] == "gnc")
            _grainNatureCoeffTable = parseHash (l[1]);
        else {
            val = l[1].toDouble (&ok);
            if (!ok)
                continue;

            if (l[0] == "th")
                _targetHumidity = val;
            else if (l[0] == "hc")
                _humidityCoeff = val;
            else if (l[0] == "mingf")
                _minGrainFlow = val;
            else if (l[0] == "wfk")
                _waterFlowK = val;
            else if (l[0] == "minwf")
                _minWaterFlow = val;
            else if (l[0] == "maxwf")
                _maxWaterFlow = val;
            else if (l[0] == "wf")
                _waterFormula = l[1].toInt ();
        }
    }

    _valid = true;
}


QString StageSettings::toString () const
{
    if (!_valid)
        return QString ("invalid");

//     printf ("toString: TH=%f,HC=%f,minGF=%f,WFK=%f,minWF=%f,maxWF=%f,WF=%d\n", 
//             _targetHumidity, _humidityCoeff, _minGrainFlow, _waterFlowK, 
//             _minWaterFlow, _maxWaterFlow, _waterFormula);
        
    return QString ().sprintf ("TH=%f,HC=%f,minGF=%f,WFK=%f,minWF=%f,maxWF=%f,WF=%d,HT=%s,GF=%s,GN=%s,GT=%s,GNC=%s", 
                               _targetHumidity, _humidityCoeff, _minGrainFlow, _waterFlowK, 
                               _minWaterFlow, _maxWaterFlow, _waterFormula, 
                               hash2string (_humidityTable).toAscii ().constData (),
                               hash2string (_grainFlowTable).toAscii ().constData (),
                               hash2string (_grainNatureTable).toAscii ().constData (),
                               hash2string (_grainTempTable).toAscii ().constData (),
                               hash2string (_grainNatureCoeffTable).toAscii ().constData ());
}


QMap<int, double> StageSettings::parseHash (const QString& str) const
{
    QStringList l = str.split (":");
    QMap<int, double> res;

    for (int i = 0; i < l.size (); i += 2)
        res[l[i].toUInt (NULL, 16)] = l[i+1].toDouble ();
    return res;
}


QString StageSettings::hash2string (const QMap<int, double>& hash) const
{
    QMap<int, double>::const_iterator it = hash.begin ();
    QString res;

    while (it != hash.end ()) {
        if (!res.isEmpty ())
            res += ":";
        res += QString::number (it.key (), 16)+":";
        res += QString::number (it.value ());
        it++;
    }

    return res;
}
