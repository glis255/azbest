#pragma once
#include <Arduino.h>

class SerialHandler : public Print {
    private:
        char currentCommand = ' ';
        bool commandSelected = false;
    public:
        enum SerialAction {
            NO_ACTION = 0,
            MENU_ENTER,
            MENU_MOVE_RIGHT,
            MENU_MOVE_LEFT,
            TRIGGER,
            RELOAD_ALARMS,
        };
        bool outputEnabled = true;
        bool debug = true;

    public:
        void init();
        SerialAction handle();
        virtual size_t write(uint8_t);
};