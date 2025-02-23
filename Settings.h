#ifndef Settings_h
#define Settings_h

#include "EEPROM.h"

enum SettingsBlock {
    SETTINGS_SENSORS,
    SETTINGS_CHARGER,
    SETTINGS_NUMBLOCKS
};


class Settings {
    public:
        long getAddr( SettingsBlock index ) {
            long addr = 0;
            for(int b=0; b < SETTINGS_NUMBLOCKS; b++ ) {
                if( b == index ) return addr;
                addr += _blocksize[b];
            }
            return addr;
        };

        void updateSize( SettingsBlock index, long bsize ) { _blocksize[index] = bsize; };

    private:
        long _blocksize[ SETTINGS_NUMBLOCKS ];
};

#endif
