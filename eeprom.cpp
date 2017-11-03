#include <Arduino.h>
#include <EEPROM.h>

template <class T> int EEPROM_writeAnything(int ee, const T& value) {
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++) {
      if (EEPROM.read(ee) != *p) {
        EEPROM.write(ee, *p);  // Only write the data if it is different to what's there
      }
      ee++;
      p++; 
    }
    EEPROM.commit();
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value) {
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++) {
          *p++ = EEPROM.read(ee++);
    }
    return i;
}
