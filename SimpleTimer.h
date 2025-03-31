#ifndef SimpleTimer_h
#define SimpleTimer_h

#include "config.h"

#include <Arduino.h>

#include <Print.h>

typedef void (*callback)(void);

/**
 * @brief The SimpleTimer class is implementing a timer functionality, which is used for periodic and deferred function calls.
 * 
 */
class SimpleTimer {
    public:
        SimpleTimer(unsigned long period = 0, unsigned long duration = 0, bool enabled = false, 
                    callback on_start = nullptr, callback on_finish = nullptr, Print *dbg = nullptr ){
            _dbg = dbg;
            
            _period = period;
            _duration = duration;
            _on_start = on_start;
            _on_finish = on_finish;
            _counter = 0;
            _enabled = enabled;
        };

        void start() { 

            _counter = 0;
            _enabled = true; 
        };

        void start( unsigned long period, unsigned long duration ) {
            _period = period;
            _duration = duration;
            start();
        };

        void stop() { 
            // _dbg->print(_id);
            // _dbg->println(" timer stopped");
            _enabled = false; 
        };

        bool isActive() { return _active; };
        bool isEnabled() { return _enabled; };

        void setId( int id ) { _id = id; };
        int getId() { return _id; };

        void setPeriod( unsigned long period ) { _period = period; };
        unsigned long getPeriod() { return _period; };

        void setDuration( unsigned long duration ) { _duration = duration; };
        unsigned long getDuration() { return _duration; };

        void setOnStart( callback on_start = nullptr ) { _on_start = on_start; };
        void setOnFinish( callback on_finish = nullptr ) { _on_finish = on_finish; };

        void tick(); 

        unsigned long getCounter() { return _counter; };

    private:
        int _id;

        unsigned long _period;
        unsigned long _duration;

        unsigned long _counter;
        bool _active;
        bool _enabled;

        callback _on_start;
        callback _on_finish;

        Print * _dbg;
};

/**
 * @brief this class is managing the timers allowing to create or increment all the timers at once.
 * 
 */
class SimpleTimerManager {
    public:
        SimpleTimerManager(Print * dbg = nullptr){
            _dbg = dbg;
        };

        // function to be called from the hardware timer 
        void tick();

        unsigned long getTicks() { return _ticks; };

        void resetTicks() { _ticks = 0; };

        SimpleTimer* create(unsigned long period = 0, unsigned long duration = 0, bool bstart = false, 
                     callback on_start = nullptr, callback on_finish = nullptr);

        SimpleTimer* get(int timer_id);

        uint8_t getNumTimers() { return _num_timers; };

    private:

        Print * _dbg;

        SimpleTimer* _simple_timers[MAX_NUM_TIMERS];
        uint8_t _num_timers = 0;

        unsigned long _ticks = 0L;
};

#endif