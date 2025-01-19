#ifndef Voltronic_h
#define Voltronic_h

#define VOLTRONIC_RELEASE 2.0
#define VOLTRONIC_DEFAULT_PROTOCOL  'V'
#define VOLTRONIC_DEFAULT_BAUD_RATE 2400

#define COMMAND_BUFFER_SIZE 32
#define DEFAULT_INTERNAL_TEMP 25.0

#include <HardwareSerial.h>
#include <Arduino.h>

#include "config.h"

static const float MIN_SELFTEST_DURATION = 0.2F;

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

enum ExecuteCommand {
    COMMAND_NONE,
    COMMAND_BEEPER_MUTE,
    COMMAND_SELF_TEST,
    COMMAND_SELF_TEST_CANCEL,
    COMMAND_SHUTDOWN,
    COMMAND_SHUTDOWN_CANCEL,
    COMMAND_SET_BRIGHTNESS,
    COMMAND_TOGGLE_DISPLAY,
    COMMAND_READ_SENSOR,
    COMMAND_TUNE_SENSOR,
    COMMAND_SAVE_SENSORS
};


class Voltronic {

    public:
        Voltronic( HardwareSerial* stream, char protocol = VOLTRONIC_DEFAULT_PROTOCOL );
        
        void begin( int baud_rate = VOLTRONIC_DEFAULT_BAUD_RATE );

        // process input/output over the Serial interface
        char process();

        ExecuteCommand executeCommand();

        // get the minutes before shutdown output
        float getShutdownMin() { return _shutdown_min; };

        // get the minutes till restore output
        int getRestoreMin() { return _restore_min; };

        float getSelftestMin() { return _selftest_min; };

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

        void setBatteryLevel( int battery_lvl ) { _battery_lvl = battery_lvl; };
        int getBatteryLevel() { return _battery_lvl; };

        void setRemainingMin(int remaining_min ) { _remaining_min = remaining_min;};
        int getRemainingMin() { return _remaining_min; };

        void setBrightnessLevel( int brightness_lvl ) { _brightness_lvl = brightness_lvl; };
        int getBrightnessLevel() { return _brightness_lvl; };

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

        int getSensorPtr() { return _sensor_ptr; };
        float getSensorParamValue() { return _sensor_value; };
        int getSensorParam() { return _sensor_param; };

        void printSensorParams(float offset, float scale, float value = 0);

        void printPartModel();
        void printPrompt();
        
        void writeEOL();

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

        // minutes till restore output
        int _restore_min;

        // minutes till disconnect output
        float _shutdown_min;

        // remaining battery level in %
        int _battery_lvl; 

        // remaining time on battery in minutes
        int _remaining_min;

        // minutes to do selftest, default is 12 sec
        float _selftest_min = MIN_SELFTEST_DURATION;

        int _brightness_lvl = DISPLAY_MAX_BRIGHTNESS;

        int _sensor_ptr = 0;
        float _sensor_value = 0;
        int _sensor_param = 0;

        int _ptr = 0;

        void writeFloat( float val , int length, int dec );
        void writeInt( int val, int length );
        void writeBin( uint8_t val );

        float parseFloat(int startpos, int len);

        void printFixed(const char* str, int len);
        void printRatedInfo();


};

#endif