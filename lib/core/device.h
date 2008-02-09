#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <QtCore>
#include "serial.h"


class DeviceCommand
{
public:
    enum kind_t {
        Init  			= 0xFF,
        GetStateWord		= 0x04,
	GetGrainFlow 		= 0x08,
	GetGrainHumidity 	= 0x0C,
	GetGrainTemperature	= 0x10,
	GetGrainNature		= 0x14,
	GetWaterFlow		= 0x18,
	GetSystemPowered	= 0x1C,
	GetGrainPresent		= 0x20,
	GetBSUPowered		= 0x24,
	GetWaterPressure	= 0x50,
	GetControllerID		= 0x30,
	GetP5State		= 0x58,
	GetP4State		= 0x5C,
	GetCleanResult		= 0x70,

	SetWaterGateS1		= 0x38,
	SetWaterGateS2		= 0x3C,
	SetWaterGateS3		= 0x40,
	SetWaterGateS4		= 0x44,
	SetFilterGates		= 0x48,
	SetKGates		= 0x4C,
	SetControllerConfig	= 0x34,

	StartWater		= 0x28,
	StopWater		= 0x2C,
	PowerGates		= 0x54,
	CleanSystem		= 0x60,
	DrainWater		= 0x64,
	SetOutputSignal		= 0x68,
	StartFilterAutomat	= 0x6C,
    };

    enum stage_t {
	Stg_First  = 1,
	Stg_Second = 2,
	Stg_Third  = 3,
	Stg_Fourth = 4,
	Stg_All    = 0xFF,
   };

private:
    kind_t _kind;
    stage_t _reply_stage;		// valid for reply only
    char _low, _high;
    bool _valid;

protected:
    char calcCRC () const
    { return (char)0xAA + _kind + _low + _high; };

public:
    DeviceCommand (kind_t kind, char low = 0, char high = 0);
    DeviceCommand (const QByteArray& data);

    void setStage (stage_t stg);

    stage_t replyStage () const
    { return _reply_stage; };

    bool operator == (const DeviceCommand& cmd) const;

    QByteArray pack () const;
    bool valid () const
        { return _valid; };

    char low () const
    { return _low; };
    
    char high () const
    { return _high; };

    int value () const
    { return (int)_high*256 + _low; };

    int delay () const;

    static bool isOK (const QByteArray& data, kind_t kind, stage_t stage);
};



class Device
{
private:
    SerialPort* _port;
    bool _manual;
    
protected:
    static char getLow (int value)
        { return (char)(value % 256); };
    static char getHigh (int value)
        { return (char)((value >> 16) % 256); };

public:
    Device (SerialPort* port) throw (QString);

    bool initialize ();
    
    void updateState () throw (QString);

    bool isManualMode () const
        { return _manual; };

    int getGrainFlow () const;
    int getGrainHumidity () const;
    int getGrainTemperature () const;
    int getGrainNature () const;
    int getWaterFlow () const;

    bool getSystemPowered () const;
    bool getGrainPresent () const;
    bool getBSUPowered () const;
    int getWaterPressure () const;
    int getControllerID () const;
    int getP5State () const;
    int getP4State () const;
    int getCleanResult () const;

    bool setWaterGateS1 (int value);
    bool setWaterGateS2 (int value);
    bool setWaterGateS3 (int value);
    bool setWaterGateS4 (int value);

    bool setFilterGates (bool g1, bool g2, bool g3, bool g4, bool g5, bool engine);
    bool setKGates (DeviceCommand::stage_t stage, bool k, bool kk);

    bool setControllerConfig (bool s1, bool s2, bool s3, bool s4);

    bool startWater (DeviceCommand::stage_t stage);
    bool stopWater (DeviceCommand::stage_t stage);
    bool powerGates (DeviceCommand::stage_t stage, bool on);
    bool cleanSystem (bool s1, bool s2, bool s3, bool s4);
    bool drainWater (bool s1, bool s2, bool s3, bool s4);
    bool setOutputSignal (bool on);
    bool startFilterAutomat ();
};


#endif
