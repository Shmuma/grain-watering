#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <QtCore>

class StageSettings
{
public:
    enum mode_t {
        M_Auto = 0,
        M_SemiAuto,
        M_Fixed,
    };

private:
    double _targetHumidity, _humidityCoeff, _minGrainFlow;
    double _waterFlowK, _minWaterFlow, _maxWaterFlow;
    int _waterFormula;
    bool _sensors;
    bool _valid;
    QString _bsu_label;
    mode_t _mode;
    double _target_flow;
    double _fixedHumidity;

    QMap<int, double> _humidityTable;
    QMap<int, double> _grainFlowTable;
    QMap<int, double> _grainNatureTable;
    QMap<int, double> _grainTempTable;
    QMap<int, double> _grainNatureCoeffTable;

protected:
    QMap<int, double> parseHash (const QString& str) const;
    QString hash2string (const QMap<int, double>& hash) const;

public:
    StageSettings ()
        : _targetHumidity (0.0),
          _humidityCoeff (0.0),
          _minGrainFlow (0.0),
          _waterFlowK (0.0),
          _minWaterFlow (0.0),
          _maxWaterFlow (0.0),
          _waterFormula (0),
          _sensors (true),
          _valid (false),
          _bsu_label (QString ()),
          _mode (M_Auto),
          _target_flow (0.0),
          _fixedHumidity (0.0)
    {
        _humidityTable.clear ();
        _grainFlowTable.clear ();
        _grainNatureTable.clear ();
        _grainTempTable.clear ();
        _grainNatureCoeffTable.clear ();
    };

    StageSettings (const QString& str);

    StageSettings (const StageSettings& sett)
        : _targetHumidity (sett._targetHumidity),
          _humidityCoeff (sett._humidityCoeff),
          _minGrainFlow (sett._minGrainFlow),
          _waterFlowK (sett._waterFlowK),
          _minWaterFlow (sett._minWaterFlow),
          _maxWaterFlow (sett._maxWaterFlow),
          _waterFormula (sett._waterFormula),
          _sensors (sett._sensors),
          _valid (sett._valid),
          _bsu_label (sett._bsu_label),
          _mode (sett._mode),
          _target_flow (sett._target_flow),
          _fixedHumidity (sett._fixedHumidity),
          _humidityTable (sett._humidityTable),
          _grainFlowTable (sett._grainFlowTable),
          _grainNatureTable (sett._grainNatureTable),
          _grainTempTable (sett._grainTempTable),
          _grainNatureCoeffTable (sett._grainNatureCoeffTable)
    {
    };

    double targetHumidity () const
        { return _targetHumidity; };
    void setTargetHumidity (double targetHumidity)
        { _targetHumidity = targetHumidity; };

    double humidityCoeff () const
        { return _humidityCoeff; };
    void setHumidityCoeff (double humidityCoeff)
        { _humidityCoeff = humidityCoeff; };

    double minGrainFlow () const
        { return _minGrainFlow; };
    void setMinGrainFlow (double minGrainFlow)
        { _minGrainFlow = minGrainFlow; };

    double waterFlowK () const
        { return _waterFlowK; };
    void setWaterFlowK (double waterFlowK)
        { _waterFlowK = waterFlowK; };

    double minWaterFlow () const
        { return _minWaterFlow; };
    void setMinWaterFlow (double minWaterFlow)
        { _minWaterFlow = minWaterFlow; };

    double maxWaterFlow () const
        { return _maxWaterFlow; };
    void setMaxWaterFlow (double maxWaterFlow)
        { _maxWaterFlow = maxWaterFlow; };

    int waterFormula () const
        { return _waterFormula; };
    void setWaterFormula (int waterFormula)
        { _waterFormula = waterFormula; };

    bool valid () const
        { return _valid; };
    void setValid (bool valid)
        { _valid = valid; };

    QString bsuLabel () const
        { return _bsu_label; };
    void setBsuLabel (const QString& label)
        { _bsu_label = label; };

    QString toString () const;

    void setHumidityTable (const QMap<int, double>& val)
        { _humidityTable = val; };
    QMap<int, double> humidityTable () const
        { return _humidityTable; };

    void setGrainFlowTable (const QMap<int, double>& val)
        { _grainFlowTable = val; };
    QMap<int, double> grainFlowTable () const
        { return _grainFlowTable; };

    void setGrainNatureTable (const QMap<int, double>& val)
        { _grainNatureTable = val; };
    QMap<int, double> grainNatureTable () const
        { return _grainNatureTable; };

    void setGrainTempTable (const QMap<int, double>& val)
        { _grainTempTable = val; };
    QMap<int, double> grainTempTable () const
        { return _grainTempTable; };

    void setGrainNatureCoeffTable (const QMap<int, double>& val)
        { _grainNatureCoeffTable = val; };
    QMap<int, double> grainNatureCoeffTable () const
        { return _grainNatureCoeffTable; };

    void setSensors (bool sensors)
        { _sensors = sensors; };
    bool sensors () const
        { return _sensors; };
   
    void setMode (mode_t mode)
        { _mode = mode; };
    void setMode (const QString& str);
    mode_t mode () const
        { return _mode; };

    void setTargetFlow (double val)
        { _target_flow = val; };
    double targetFlow () const
        { return _target_flow; };

    void setFixedHumidity (double val)
        { _fixedHumidity = val; };
    double fixedHumidity () const 
        { return _fixedHumidity; };
};


#endif
