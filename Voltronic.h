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
#ifndef DISPLAY_TYPE_NONE    
    COMMAND_SET_BRIGHTNESS,
    COMMAND_TOGGLE_DISPLAY,
    COMMAND_TOGGLE_DISPLAY_MODE,
#endif
    COMMAND_READ_SENSOR,
    COMMAND_TUNE_SENSOR,
    COMMAND_SAVE_SENSORS
};

enum VoltronicParam {
    PARAM_INPUT_VAC,
    PARAM_INPUT_FAULT_VAC,
    PARAM_OUTPUT_VAC,
    PARAM_OUTPUT_VAC_NOMINAL,
    PARAM_OUTPUT_AC_NOMINAL,
    PARAM_OUTPUT_FREQ,
    PARAM_OUTPUT_FREQ_NOMINAL,
    PARAM_OUTPUT_LOAD_LEVEL,
    PARAM_BATTERY_VDC,
    PARAM_BATTERY_VDC_NOMINAL,
    PARAM_BATTERY_LEVEL,
    PARAM_INTERNAL_TEMP,
    PARAM_SELFTEST_MIN,         // minutes to do selftest, default is 12 sec
    PARAM_SHUTDOWN_MIN,         // minutes till disconnect output
    PARAM_REMAINING_MIN,        // remaining time on battery in minutes
    PARAM_RESTORE_MIN,          // get the minutes till restore output
#ifndef DISPLAY_TYPE_NONE
    PARAM_DISPLAY_BRIGHTNESS_LEVEL,
#endif
    PARAM_NUMPARAM
};

/**
 * @brief this class implements Voltronic protocol for serial communication with the UPS controller
 * 
 */
class Voltronic {

    public:
        Voltronic( HardwareSerial* stream, char protocol = VOLTRONIC_DEFAULT_PROTOCOL );
        
        void begin( int baud_rate = VOLTRONIC_DEFAULT_BAUD_RATE );

        // process input/output over the Serial interface
        char process();

        ExecuteCommand executeCommand();

        void setStatus( uint8_t status ) { _status = status; };
        uint8_t getStatus() { return _status; };

        float getParam(int index) { return _param[index];};
        void setParam(int index, float value) { _param[index] = value;};

        int getSensorPtr() { return _sensor_ptr; };
        float getSensorParamValue() { return _sensor_param_value; };
        int getSensorParam() { return _sensor_param; };

        void printSensorParams(float offset, float scale, float value = 0, int reading = 0, long reading_sum = 0L);

        void printParam(const char* fmp, ...);

        void printPartModel();
        void printPrompt();
        
        void writeEOL();

    private:

        char _buf[COMMAND_BUFFER_SIZE];
        int _buf_ptr = 0;
        
        HardwareSerial* _stream;

        float _param[PARAM_NUMPARAM];

        uint8_t _status;

        char _protocol = VOLTRONIC_DEFAULT_PROTOCOL;

        // sensor manipulation params
        int _sensor_ptr = 0;
        float _sensor_param_value = 0;
        int _sensor_param = 0;

        void writeFloat( float val , int length, int dec );
        void writeInt( int val, int length );
        void writeBin( uint8_t val );

        float parseFloat(int startpos, int len);

        void printFixed(const char* str, int len);
        void printRatedInfo();

        int get_width_modifier(const char * modifier, int * index);
        int get_precision_modifier(const char * modifier, int * index);

};

#endif