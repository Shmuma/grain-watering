#include <QtCore>

#include "settings.h"



// --------------------------------------------------
// StageSettings
// --------------------------------------------------
StageSettings::StageSettings (const QString& str)
    : _valid (false)
{
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

    _valid = true;
}


QString StageSettings::toString () const
{
    if (!_valid)
        return QString ("invalid");

//     printf ("toString: TH=%f,HC=%f,minGF=%f,WFK=%f,minWF=%f,maxWF=%f,WF=%d\n", 
//             _targetHumidity, _humidityCoeff, _minGrainFlow, _waterFlowK, 
//             _minWaterFlow, _maxWaterFlow, _waterFormula);
        
    return QString ().sprintf ("TH=%f,HC=%f,minGF=%f,WFK=%f,minWF=%f,maxWF=%f,WF=%d", 
                               _targetHumidity, _humidityCoeff, _minGrainFlow, _waterFlowK, 
                               _minWaterFlow, _maxWaterFlow, _waterFormula);
}
