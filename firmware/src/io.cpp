#include "io.h"

IO::IO(AlarmManager* alarms) {
    this->alarms = alarms;

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    pinMode(LED_R_PIN, OUTPUT);
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    digitalWrite(LED_R_PIN, HIGH);
    digitalWrite(LED_G_PIN, HIGH);
    digitalWrite(LED_B_PIN, HIGH);

    pinMode(ENC_BTN_PIN, INPUT_PULLUP);
    pinMode(ENC_A_PIN, INPUT_PULLUP);
    pinMode(ENC_B_PIN, INPUT_PULLUP);

    encoder = ESPRotary(ENC_A_PIN, ENC_B_PIN, 4);
}

IO::IOAction IO::update() {
    if (relayTriggered && (millis() - relayTriggeredTimestamp >= alarms->alarmDuration)) {
        relayTriggered = false;
        digitalWrite(RELAY_PIN, LOW);
    }

    if (!buttonPressed && !digitalRead(ENC_BTN_PIN) && millis() - buttonClickTimestamp >= 250UL) {
        buttonClickTimestamp = millis();
        buttonPressed = true;
        return BUTTON_PRESS;
    } else if (buttonPressed && digitalRead(ENC_BTN_PIN) && millis() - buttonClickTimestamp >= 250UL) {
        buttonClickTimestamp = millis();
        buttonPressed = false;
    }

    // i love china
    if (encoderPosition != previousEncoderPosition) {
        if (encoderPosition > previousEncoderPosition) {
            previousEncoderPosition = encoderPosition;
            return ENCODER_RIGHT;
        } else {
            previousEncoderPosition = encoderPosition;
            return ENCODER_LEFT;
        }
    }

    return NO_ACTION;
}

void IO::triggerRelay() {
    relayTriggeredTimestamp = millis();
    relayTriggered = true;
    digitalWrite(RELAY_PIN, HIGH);
}

void IO::showColor(RGB_COLOR color) {
    analogWrite(LED_R_PIN, 255 - color.r);
    analogWrite(LED_G_PIN, 255 - color.g);
    analogWrite(LED_B_PIN, 255 - color.b);
}