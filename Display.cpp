#include "Display.h"

Display::Display()
  : TM1640(DISPLAY_DA_OUT, DISPLAY_CLK_OUT, DISPLAY_MAX_POS) {  
    setupDisplay(true, DISPLAY_DEFAULT_BRIGHTNESS);  
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

void Display::show(bool blink_state) {
    sendCommand(TM16XX_CMD_DATA_AUTO);
    start();
    send(TM16XX_CMD_ADDRESS | 0x0);	
    for( int i = 0; i < DISPLAY_MAX_POS; i++ ) {
        send( ( blink_state? board[i] ^ blink[i] : board[i] ));
    }
    stop();

}

void Display::setupDisplay(boolean active, byte intensity) {
    _active = active;
    _brightness = intensity;
    TM1640::setupDisplay(_active, intensity);

}

void Display::toggle() {
    setupDisplay(!_active, _brightness);
}