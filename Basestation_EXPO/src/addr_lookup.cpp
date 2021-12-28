#include <addr_lookup.h>

bool lookup_addr(uint8_t table[], uint8_t addr) {
    for (size_t i = 0; i < sizeof(table); i++) {
        if(table[i] == addr) { 
            #ifdef VERY_EXPLICIT
                Serial.printf("%x\n",table[i]);
            #endif

            return true;
        } 
    }
    return false;
}