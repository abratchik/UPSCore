#include "SimpleTimer.h"

void SimpleTimer::tick() {

    // if timer is disabled, do nothing
    if(!_enabled) {
        
        if(_active && _on_finish)
            _on_finish();
        _active = false;
        return;
    }

    if( _active ) {
        // counter did not reach _period yet


        // if the timer is active and duration reached, set _active to false  
        if( _counter >= _duration ) {
            // _dbg->print(_id);
            // _dbg->println(" duration reached");
            _active = false;
            if(_on_finish) 
                _on_finish();
            
            if( _period == 0 ) stop();
        }
    }
    else {
        // reset the counter and launch the on_start handler if defined. Timer will be active till _counter is below _duration
        if( _counter >= _period ) {

            _counter = 0;
            _active = ( _duration > 0 ); // activate only if duration > 0 else timer becomes disabled
            _enabled = _active || ( _period > 0 );
            if(_on_start) 
                _on_start();
        }
    }

    _counter++;

}

void SimpleTimerManager::tick() {

    _ticks++;

    for(int i=0; i < _num_timers; i++) 
        _simple_timers[i]->tick();
    
}

SimpleTimer* SimpleTimerManager::create(unsigned long period, unsigned long duration, bool bstart, 
                                        callback on_start, callback on_finish) {
    
    if(_num_timers >= MAX_NUM_TIMERS) return nullptr;
    
    _num_timers++;

    _simple_timers[_num_timers - 1] = new SimpleTimer(_dbg, period, duration, bstart, on_start, on_finish);

    _simple_timers[_num_timers - 1]->setId(_num_timers);

    return _simple_timers[_num_timers - 1];
}

SimpleTimer* SimpleTimerManager::get( int timer_id ) {
       
    for(int i=0; i < _num_timers; i++) {
        SimpleTimer* timer = *(_simple_timers+i);
        if(timer->getId() == timer_id)
            return timer;
    }

    return nullptr;
}