#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <QtCore>
#include "serial.h"


class Device
{
public:
    enum stages_mask_t {
	Stg_First  = 1,
	Stg_Second = 2,
	Stg_Third  = 4,
	Stg_Fourth = 8,
    };

private:
    SerialPort* _port;
    bool _manual;
    
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
    stages_mask_t getControllerConfig () const;
    int getP5State () const;
    int getP4State () const;
    int getCleanResult () const;
};


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
	GetControllerConfig	= 0x34,
	GetP5State		= 0x58,
	GetP4State		= 0x5C,
	GetCleanResult		= 0x70,
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
        { return 0xAA + _kind + _low + _high; };

public:
    DeviceCommand (kind_t kind);
    DeviceCommand (const QByteArray& data);

    void setStage (stage_t stg);

    stage_t getReplyStage () const
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
};

#endif
