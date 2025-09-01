#pragma once
#include <Arduino.h>
#include <lcdMenu.h>
#include <ESPRotary.h>
#include "pins.h"

struct RGB_COLOR
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class IO {
    private:
        int32_t previousEncoderPosition = 0;
        uint32_t relayTriggeredTimestamp = 0;
        uint32_t encoderChangedTimestamp = 0;
        uint32_t buttonClickTimestamp = 0;
        AlarmManager* alarms;

    public:
        enum IOAction {
            NO_ACTION = 0,
            ENCODER_RIGHT,
            ENCODER_LEFT,
            BUTTON_PRESS,
        };

        IO(AlarmManager* alarms);
        ESPRotary encoder;
        bool relayTriggered = false;
        bool buttonPressed = false;
        int32_t encoderPosition = 0;
    
    public:
        IOAction update();
        void triggerRelay();
        void showColor(RGB_COLOR color);
};