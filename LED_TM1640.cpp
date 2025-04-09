#include "LED_TM1640.h"

#ifdef DISPLAY_TYPE_LED_TM1640

void Display::initialize() {
    setup_display();
}

void Display::setup_display() {
    sendCommand(TM16XX_CMD_DATA_FIXED);
    TM1640::setupDisplay(_active, _brightness);
}

void Display::on_refresh() {
    
    _blink_state = !_blink_state; 

    clear(false);

    switch(_display_mode) {
        case DISPLAY_FREQ:
            setInputReading( _vac_in->get_frequency(), UNIT_HZ );
            setOutputReading( _vac_out->get_frequency(), UNIT_HZ);
            break;

        default:
            setInputReading( _vac_in->readingR(), UNIT_VAC );
            setOutputReading( _vac_out->readingR(), UNIT_VAC );
            break;
    }

    float battery_level = _lineups->getBatteryLevel();
    ReadingDirection direction = _lineups->isBatteryMode() ? 
                                    LEVEL_DECREASING : 
                                    ( _charger->get_mode() <= CHARGING_BY_CV ? LEVEL_INCREASING : LEVEL_NO_CHANGE );

    setBatteryLevel( battery_level, direction );
    float load_level = _ac_out->reading() / INTERACTIVE_MAX_AC_OUT;
    setLoadLevel( load_level );
    setFlag( ( load_level > 0.0 ? LOAD_INDICATOR : 0 ) | 
                ( battery_level > 0.0 ? BATTERY_INDICATOR : 0 ) |
                ( _lineups->isBatteryMode() ? BATTERY_MODE_INDICATOR : AC_MODE_INDICATOR ) );
    setInputRelayStatus( _lineups->readStatus(INPUT_CONNECTED) );
    setOutputRelayStatus( _lineups->readStatus(OUTPUT_CONNECTED) );
    
    setBlink( ( _lineups->readStatus( OVERLOAD ) ? LOAD_INDICATOR : 0) |
                ( _lineups->readStatus( BATTERY_LOW ) ? BATTERY_INDICATOR : 0) |
                ( _lineups->readStatus( UNUSUAL_STATE ) ? UNUSUAL_MODE_INDICATOR : 0) |
                ( _lineups->readStatus( UPS_FAULT ) ? UPS_FAULT_INDICATOR : 0) );

    show();

}


void Display::setInputReading(int reading, ReadingUnit mode ) {
    setReading( reading, mode, 2, 0 );
}

void Display::setOutputReading( int reading, ReadingUnit mode ) {
    setReading( reading, mode, 5, 3 );
}

void Display::setReading( int reading, ReadingUnit mode, int start, int stop ) {
    int digits = reading;
    int base = ( !mode ? 16 : 10 );
    for( int grid = start; grid >= stop; grid-- ) {
        uint8_t grid_mode = grid - stop;
        uint8_t mod_flag = ( mode > 0 ) && ( grid_mode == mode ) ;
        board[grid] = pgm_read_byte( DISPLAY_NUMBER_FONT + ( digits % base ) ) | mod_flag;
        digits /= base;
        if(!digits) break;
    }
}

void Display::setBatteryLevel( float level, ReadingDirection direction ) {
    setLevel(level, true, direction);
}

void Display::setLoadLevel( float level ) {
    setLevel(level, false, LEVEL_NO_CHANGE);
}

void Display::setLevel( float level, bool isHI , ReadingDirection direction ) {
    uint16_t flags = 0;
    uint16_t blink_segment = 0;

    uint16_t* display_level = ( isHI ? DISPLAY_BATTERY_LEVEL : DISPLAY_LOAD_LEVEL );

    if( level > 0.0F ) 
        for( int ptr = 0; ptr < DISPLAY_LEVEL_N_SEGMENTS; ptr++ ) {
            uint16_t segment = pgm_read_word( display_level + ptr );
            flags |= segment;
            float threshold = DISPLAY_LEVEL_INTERVAL * ptr; 
            if( level < threshold + DISPLAY_LEVEL_INTERVAL ) {
                if( ( direction == LEVEL_DECREASING && level < threshold + DISPLAY_LEVEL_INTERVAL * 0.3 ) || 
                    ( direction == LEVEL_INCREASING ) )
                    blink_segment = segment;
                break;
            }
        }
    

    if(isHI) {
        board[FLAG_HI_GRID] |= highByte(flags);
        blink[FLAG_HI_GRID] |= highByte(blink_segment);
    }
    else {
        board[FLAG_LO_GRID] |= lowByte(flags);
        blink[FLAG_LO_GRID] |= lowByte(blink_segment);
    }
    
}

void Display::setFlag(DisplayFlag flag) {
    board[FLAG_HI_GRID] |= highByte((uint16_t) flag);
    board[FLAG_LO_GRID] |= lowByte((uint16_t) flag);
}

void Display::setBlink(DisplayFlag flag) {
    blink[FLAG_HI_GRID] |= highByte((uint16_t) flag);
    blink[FLAG_LO_GRID] |= lowByte((uint16_t) flag);
}

void Display::setRelayStatus( bool rly_status, uint8_t grid ) {
    board[grid] |= rly_status;
}


void Display::clear(bool clear_display) {
    if(clear_display) clearDisplay(); 
    memset(board, 0x0, sizeof(board));
    memset(blink, 0x0, sizeof(blink));
}

void Display::show() {
    sendCommand(TM16XX_CMD_DATA_AUTO);
    start();
    send(TM16XX_CMD_ADDRESS | 0x0);	
    for( int i = 0; i < DISPLAY_MAX_POS; i++ ) {
        send( ( _blink_state? board[i] ^ blink[i] : board[i] ));
    }
    stop();

}

#endif

