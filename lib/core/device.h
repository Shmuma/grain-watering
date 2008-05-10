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
	SetStages		= 0x34,

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
    unsigned char _low, _high;
    bool _valid;

protected:
    char calcCRC () const
        { return (char)0xAA + _kind + _low + _high; };

public:
    DeviceCommand (kind_t kind, unsigned char low = 0, unsigned char high = 0);
    DeviceCommand (stage_t stage, unsigned char low = 0, unsigned char high = 0);
    DeviceCommand (const QByteArray& data);
    DeviceCommand (kind_t kind, stage_t stage);

    void setStage (stage_t stg);

    stage_t replyStage () const
        { return _reply_stage; };

    bool operator == (const DeviceCommand& cmd) const;

    QByteArray pack () const;

    kind_t kind () const
        { return _kind; };

    bool valid () const
        { return _valid; };

    unsigned char low () const
        { return _low; };
    
    unsigned char high () const
        { return _high; };

    unsigned int value () const
        { return (unsigned int)_high*256 + _low; };

    int delay () const;

    static bool isOK (const QByteArray& data, kind_t kind, stage_t stage);
    static stage_t stageByNum (int stage);
};



class Device
{
private:
    SerialPort* _port;
    bool _manual, _connected;
    char _drainBitmask;
    
protected:
    static unsigned char getLow (unsigned int value)
        { return (unsigned char)(value % 256); };
    static unsigned char getHigh (unsigned int value)
        { return (unsigned char)((value >> 8) % 256); };

public:
    Device (SerialPort* port) throw (QString);

    bool initialize ();
    
    void updateState () throw (QString);

    bool isManualMode () const
        { return _manual; };
    bool isConnected () const
        { return _connected; };

    unsigned int getGrainFlow (DeviceCommand::stage_t stage) const;
    unsigned int getGrainHumidity (DeviceCommand::stage_t stage) const;
    unsigned int getGrainTemperature (DeviceCommand::stage_t stage) const;
    unsigned int getGrainNature (DeviceCommand::stage_t stage) const;
    unsigned int getWaterFlow (DeviceCommand::stage_t stage) const;

    bool getGrainPresent (DeviceCommand::stage_t stage) const;
    bool getBSUPowered (DeviceCommand::stage_t stage) const;
    unsigned int getWaterPressure () const;
    unsigned int getControllerID () const;
    unsigned int getP5State () const;
    unsigned int getP4State () const;
    unsigned int getCleanResult () const;

    bool setWaterGateS1 (unsigned int value);
    bool setWaterGateS2 (unsigned int value);
    bool setWaterGateS3 (unsigned int value);
    bool setWaterGateS4 (unsigned int value);

    bool setFilterGates (bool g1, bool g2, bool g3, bool g4, bool g5, bool engine);
    bool setKGates (DeviceCommand::stage_t stage, bool k, bool kk);

    bool setStages (bool s1, bool s2, bool s3, bool s4);

    bool startWater (DeviceCommand::stage_t stage);
    bool stopWater (DeviceCommand::stage_t stage);
    bool powerGates (DeviceCommand::stage_t stage, bool on);
    bool cleanSystem (bool s1, bool s2, bool s3, bool s4);
    bool drainWater (bool s1, bool s2, bool s3, bool s4);
    bool setOutputSignal (bool on);
    bool startFilterAutomat ();

    QByteArray sendRawCommand (const QByteArray& data);

    bool syncWithDevice ();

    bool cleaningStarted ();
    bool waterDrained ();
};


#endif
