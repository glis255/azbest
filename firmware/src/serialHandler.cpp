#include "serialHandler.h"
#include "keys.h"

void SerialHandler::init() {
    Serial.begin(115200);
}

SerialHandler::SerialAction SerialHandler::handle() {
    while (Serial.available()) {
        char incoming = Serial.read();
        currentCommand = incoming;

        switch (currentCommand) {
        case MENU_SERIAL_ENTER:
            return MENU_ENTER;
        case MENU_SERIAL_UP:
            return MENU_MOVE_LEFT;
        case MENU_SERIAL_DOWN:
            return MENU_MOVE_RIGHT;
        case MENU_SERIAL_TRIGGER:
            return TRIGGER;
        case MENU_SERIAL_RELOAD_ALARMS:
            return RELOAD_ALARMS;
        default:
            break;
        }
    }

    return NO_ACTION;
}

size_t SerialHandler::write(uint8_t d) {
    return Serial.write(d);
}