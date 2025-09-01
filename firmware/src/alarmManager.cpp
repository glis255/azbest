#include <Arduino.h>
#include "alarmManager.h"
#include <EEPROM.h>
#include "flashOrg.h"
#include "pins.h"

bool alarmsHaltedSettingEEPROM;

AlarmManager::AlarmManager(Clock* clock) {
    this->clock = clock;
}

void AlarmManager::begin() {
    plan = EEPROM.readUChar(PLAN_SELECTED_UCHAR);

    alarmDuration = EEPROM.readUInt(ALARM_DURATION_UINT);
    weekdays = EEPROM.readUChar(plan + PLAN_0_WEEKDAYS_UCHAR);

    alarmsHalted = EEPROM.readBool(ALARMS_HALTED_BOOL);
    alarmsHaltedSettingEEPROM = alarmsHalted;
}

void AlarmManager::editPlan(uint8_t planNr, uint16_t* alarms) {
    if (planNr > 5 && planNr < 0) planNr = 0;

    uint8_t index = 0;
    for (uint16_t addr = ALARMS_OFFSET + (256U * planNr); addr <= ALARMS_OFFSET + 256U + (256U * planNr); addr = addr + 2U) {
        EEPROM.writeUInt(addr, alarms[index]);
        if (alarms[index] == 0xffff) break;
        index++;
    }

    EEPROM.commit();

    Serial.print("Edited plan ");
    Serial.print(plan);
    Serial.print(" with ");
    Serial.print(index);
    Serial.println(" alarms");
}

void AlarmManager::loadAlarms() {
    Serial.print("Loading plan: ");
    Serial.println(plan);

    selectedPlan = plan;

    if (plan > 5 && plan < 0) plan = 0;

    numberOfAlarms = 0;

    for (uint16_t i = ALARMS_OFFSET + (256 * plan); i <= ALARMS_OFFSET + 256 + (256 * plan); i = i + 2) {
        uint16_t read = EEPROM.readUInt(i);

        if (read == 0xffff) break;

        queuedAlarms[numberOfAlarms] = read;
        numberOfAlarms++;
    }

    alarmsInPreset = numberOfAlarms;

    uint16_t timeNow = this->clock->minutesFromMidnight();
    for (int i = numberOfAlarms - 1; i >= 0; i--) {
        if (queuedAlarms[i] > timeNow) {
            numberOfAlarms = i + 1;
            break;
        }
    }

    Serial.print("Alarms: ");
    Serial.print(numberOfAlarms);
    Serial.print("/");
    Serial.println(alarmsInPreset);

    weekdays = EEPROM.readUChar(plan + PLAN_0_WEEKDAYS_UCHAR);

    Serial.print("Weekdays: 0b");
    Serial.println(weekdays, BIN);

    alarmsLoaded = true;
}

void AlarmManager::update() {
    uint16_t nextAlarmTime = queuedAlarms[numberOfAlarms - 1];
    if (nextAlarmTime != lastAlarmTime && (clock->minutesFromMidnight() == nextAlarmTime) && (weekdays & clock->getDay()) != 0) {
        lastAlarmTime = nextAlarmTime;
        numberOfAlarms--;
        if (!forceMute && !alarmsHalted) alarmTriggered = true;
        if (numberOfAlarms == 0) loadAlarms();
    }

    if (!alarmsLoaded) loadAlarms();

    if (alarmsHaltedSettingEEPROM != alarmsHalted) {
        alarmsHaltedSettingEEPROM = alarmsHalted;
        EEPROM.writeBool(ALARMS_HALTED_BOOL, alarmsHalted);
        EEPROM.commit();
    }
}

void AlarmManager::getPlan(uint8_t planNr, uint16_t* buffer) {
    if (planNr > 5 && planNr < 0) planNr = 0;

    uint8_t index = 0;
    for (uint16_t addr = ALARMS_OFFSET + (256U * planNr); addr <= ALARMS_OFFSET + 256U + (256U * planNr); addr = addr + 2U) {
        uint16_t read = EEPROM.readUInt(addr);

        buffer[index] = read;

        if (read == 0xffff) break;
        index ++;
    }
}

uint8_t AlarmManager::getPlanWeekDays(uint8_t planNr) {
    return EEPROM.readUChar(planNr + PLAN_0_WEEKDAYS_UCHAR);
}

void AlarmManager::setWeekdays(uint8_t planNr, uint8_t days) {
    EEPROM.writeUChar(planNr + PLAN_0_WEEKDAYS_UCHAR, days);
    EEPROM.commit();
}

bool AlarmManager::setPlanName(uint8_t planNr, String name) {
    EEPROM.writeString(PLAN_0_NAME_STRING + (planNr * 33), name);
    return EEPROM.commit();
}

String AlarmManager::getPlanName(uint8_t planNr) {
    return EEPROM.readString(PLAN_0_NAME_STRING + (planNr * 33));
}

String AlarmManager::getPlanName() {
    return getPlanName(plan);
}

uint8_t cycle = 0;
unsigned long bellTimestamp = 0UL;
bool bellOn = false;
void AlarmManager::emergencyAlarmUpdate() {
    if (emergencyAlarm) {
        uint16_t interval;
        if (cycle == 0) interval = 5000;
        else if (cycle == 1) interval = 3000;
        else if (cycle == 2) interval = 5000;
        else if (cycle == 3) interval = 3000;
        else if (cycle == 4) interval = 5000;
        else if (cycle == 5) interval = 5000;

        if (millis() - bellTimestamp >= interval) {
            bellTimestamp = millis();
            bellOn = !bellOn;
            digitalWrite(RELAY_PIN, bellOn);
            cycle ++;
            if (cycle == 6) cycle = 0;
        }
    }
}

void AlarmManager::engageEmergencyAlarm() {
    cycle = 0;
    bellTimestamp = 0UL;
    emergencyAlarm = true;
    forceMute = true;
    bellOn = true;
    digitalWrite(RELAY_PIN, true);
}

void AlarmManager::disengageEmergencyAlarm() {
    emergencyAlarm = false;
    forceMute = false;
    digitalWrite(RELAY_PIN, false);
}
