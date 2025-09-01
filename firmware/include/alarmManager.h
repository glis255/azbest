#pragma once
#include <Arduino.h>
#include "clock.h"
#include "weekdays.h"

#define ALARMS_OFFSET 256U
#define LARGEST_HOUR 1439U

class AlarmManager {
    private:
    public:
        Clock* clock;
        bool alarmsHalted;
        bool alarmsLoaded = false;
        bool alarmTriggered = false;
        uint8_t selectedPlan = 1;
        uint16_t queuedAlarms[128];
        uint8_t numberOfAlarms = 0; // max 128
        uint8_t alarmsInPreset = 0;
        uint16_t lastAlarmTime = 0xffff;
        uint8_t plan = 0;
        uint32_t alarmDuration = 5000;
        uint8_t weekdays;
        bool forceMute = false;
        bool emergencyAlarm = false;
    public:
        AlarmManager(Clock* clock);
        void begin();
        void editPlan(uint8_t planNr, uint16_t* alarms);
        void loadAlarms();
        void update();
        void getPlan(uint8_t planNr, uint16_t* buffer);
        uint8_t getPlanWeekDays(uint8_t planNr);
        void setWeekdays(uint8_t planNr, uint8_t days);
        bool setPlanName(uint8_t planNr, String name); // true if successful
        String getPlanName(uint8_t planNr);
        String getPlanName();
        void emergencyAlarmUpdate();
        void engageEmergencyAlarm();
        void disengageEmergencyAlarm();
};