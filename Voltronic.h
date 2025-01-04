#ifndef Voltronic_h
#define Voltronic_h

#define VOLTRONIC_RELEASE 2.0
#define VOLTRONIC_DEFAULT_PROTOCOL  'P'
#define VOLTRONIC_DEFAULT_BAUD_RATE 2400

#define COMMAND_BUFFER_SIZE 32
#define DEFAULT_INTERNAL_TEMP 25.0

#include <HardwareSerial.h>
#include <Arduino.h>

#include "config.h"

enum StatusBit {
    STATUS_BEEPER_ACTIVE,
    STATUS_SHUTDOWN_ACTIVE,
    STATUS_SELFTEST,
    STATUS_LINE_INTERACTIVE,
    STATUS_FAULT,
    STATUS_BOOST_BACK_ACTIVE,
    STATUS_BATTERY_LOW,
    STATUS_UTILITY_FAIL 
};

enum CommandStatus {
    COMMAND_NONE,
    COMMAND_BEEPER_MUTE,
    COMMAND_SELF_TEST,
    COMMAND_SHUTDOWN,
    COMMAND_SHUTDOWN_CANCEL
};


class Voltronic {

    public:
        Voltronic( HardwareSerial* stream, char protocol = VOLTRONIC_DEFAULT_PROTOCOL );
        
        void begin( int baud_rate = VOLTRONIC_DEFAULT_BAUD_RATE );

        // process input/output over the Serial interface
        char process();

        CommandStatus executeCommandBuffer();

        // get the minutes before shutdown output
        float getShutdownMin() { return _shutdown_min; };

        // get the minutes till restore output
        int getRestoreMin() { return _restore_min; };

        void setProtocol( char protocol ) { _protocol = protocol; };
        char getProtocol() { return _protocol; };

        void setInputVoltage( float in_v ) { _in_v = in_v; };
        float getInputVoltage() { return _in_v; };

        void setInputFaultVoltage( float in_flt_v ) { _in_flt_v = in_flt_v; };
        float getInputFaultVoltage() { return _in_flt_v; };

        void setOutputVoltage( float out_v ) { _out_v = out_v; };
        float getOutputVoltage() { return _out_v; };

        void setLoadLevel( int load_lvl ) { _load_lvl = load_lvl; };
        int getLoadLevel() { return _load_lvl; };

        void setOutputFreq( float out_f ) { _out_f = out_f; };
        float getOututFreq() { return _out_f; };

        void setBatteryVoltage( float bat_v ) { _bat_v = bat_v; };
        float getBatteryVoltage() { return _bat_v; };

        void setInternalTemp( float int_t ) { _int_t = int_t; };
        float getInternalTemp() { return _int_t; };

        void setStatus( uint8_t status ) { _status = status; };
        uint8_t getStatus() { return _status; };

        void setOutputVoltageNom( float out_v_nom ) { _out_v_nom = out_v_nom; };
        float getOutputVoltageNom() { return _out_v_nom; };

        void setOutputCurrentNom( int out_c_nom ) { _out_c_nom = out_c_nom; };
        int getOutputCurrentNom() { return _out_c_nom; };

        void setBatteryVoltageNom( float bat_v_nom ) { _bat_v_nom = bat_v_nom; };
        float getBatteryVoltageNom() { return _bat_v_nom; };

        void setOutputFreqNom( int out_f_nom ) { _out_f_nom = out_f_nom; };
        int getOutputFreqNom() { return _out_f_nom; };

    private:

        char _buf[COMMAND_BUFFER_SIZE];
        
        HardwareSerial* _stream;

        float _in_v;
        float _in_flt_v;
        float _out_v;
        int _load_lvl;
        float _out_f = INTERACTIVE_DEFAULT_FREQ;
        float _bat_v;
        float _int_t = DEFAULT_INTERNAL_TEMP;
        uint8_t _status;

        float _out_v_nom = INTERACTIVE_DEFAULT_INPUT_VOLTAGE;
        int _out_c_nom = INTERACTIVE_MAX_AC_OUT;
        float _bat_v_nom = INTERACTIVE_MAX_V_BAT;
        float _out_f_nom = INTERACTIVE_DEFAULT_FREQ;

        char _protocol = VOLTRONIC_DEFAULT_PROTOCOL;

        void writeEOL();
        void writeFloat( float val , int length, int dec );
        void writeInt( int val, int length );
        void writeBin( uint8_t val );

        // minutes till restore output
        int _restore_min;

        // minutes till disconnect output
        float _shutdown_min;

        int _ptr = 0;

        bool parseShutdownRestoreTimeouts();


};

#endif