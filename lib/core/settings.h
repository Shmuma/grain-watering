#ifndef __SETTINGS_H__
#define __SETTINGS_H__


class StageSettings
{
private:
    double _targetHumidity, _humidityCoeff, _minGrainFlow;
    double _waterFlowK, _minWaterFlow, _maxWaterFlow;
    int _waterFormula;
    bool _valid;

public:
    StageSettings ()
        : _targetHumidity (0.0),
          _humidityCoeff (0.0),
          _minGrainFlow (0.0),
          _waterFlowK (0.0),
          _minWaterFlow (0.0),
          _maxWaterFlow (0.0),
          _waterFormula (0),
          _valid (false)
    {};

    StageSettings (const QString& str);

    StageSettings (const StageSettings& sett)
        : _targetHumidity (sett._targetHumidity),
          _humidityCoeff (sett._humidityCoeff),
          _minGrainFlow (sett._minGrainFlow),
          _waterFlowK (sett._waterFlowK),
          _minWaterFlow (sett._minWaterFlow),
          _maxWaterFlow (sett._maxWaterFlow),
          _waterFormula (sett._waterFormula),
          _valid (sett._valid)
    {};

    StageSettings (double targetHumidity, double humidityCoeff, double minGrainFlow, 
                    double waterFlowK, double minWaterFlow, double maxWaterFlow, int waterFormula)
        : _targetHumidity (targetHumidity),
          _humidityCoeff (humidityCoeff),
          _minGrainFlow (minGrainFlow),
          _waterFlowK (waterFlowK),
          _minWaterFlow (minWaterFlow),
          _maxWaterFlow (maxWaterFlow),
          _waterFormula (waterFormula),
          _valid (true)
    {};

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

    QString toString () const;
};


#endif
