#ifndef SimpleTimer_h
#define SimpleTimer_h

#include <Arduino.h>

#include <HardwareSerial.h>

typedef void (*callback)(void);

class SimpleTimer {
    public:
        SimpleTimer(HardwareSerial * dbg, unsigned long period = 0, unsigned long duration = 0, bool enabled = false, 
                     callback on_start = nullptr, callback on_finish = nullptr ){
            _dbg = dbg;
            
            _period = period;
            _duration = duration;
            _on_start = on_start;
            _on_finish = on_finish;
            _counter = 0;
            _enabled = enabled;
        };

        void start() { 
            _dbg->print(_id);
            _dbg->print(" started with period=");
            _dbg->print(_period);
            _dbg->print(",duration=");
            _dbg->println(_duration);
            _counter = 0;
            _enabled = true; 
        };

        void start( unsigned long period, unsigned long duration ) {
            _period = period;
            _duration = duration;
            start();
        };
        
        void stop() { 
            _dbg->print(_id);
            _dbg->println(" stopped");
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

        HardwareSerial * _dbg;
};


class SimpleTimerManager {
    public:
        SimpleTimerManager(HardwareSerial * dbg){
            _dbg = dbg;
        };

        // function to be called from the hardware timer 
        void tick();

        unsigned long getTicks() { return _ticks; };

        void resetTicks() { _ticks = 0; };

        SimpleTimer* create(unsigned long period = 0, unsigned long duration = 0, bool bstart = false, 
                     callback on_start = nullptr, callback on_finish = nullptr);

        SimpleTimer* get(int timer_id);

        int getNumTimers() { return _num_timers; };

    private:

        HardwareSerial * _dbg;

        SimpleTimer* _simple_timers[10];
        int _num_timers = 0;

        unsigned long _ticks = 0L;
};

#endif