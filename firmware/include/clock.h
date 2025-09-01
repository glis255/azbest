#pragma once
#include <Arduino.h>
#include "macAddress.h"
#include "UIPEthernet.h"
#include "TimeLib.h"
#include "NTPClient.h"
#include "RTClib.h"
#include "Timezone.h"
#include "weekdays.h"

// 1.5h 
#define RTC_UPDATE_TIMEOUT 5400000

class Clock {
    private:
        EthernetUDP* udp;
        NTPClient* ntp;
        RTC_DS3231* rtc;
        bool ntpClientReady = false;
        TimeChangeRule dstRule = { "CEST", Last, Sun, Mar, 2, 120 };
        TimeChangeRule stdRule = { "CET", Last, Sun, Oct, 3, 60 };
        Timezone tz = Timezone(dstRule, stdRule);
        ulong timeUpdatedFromRTCTimestamp;
    public:
        int32_t timeOffsetHours = 0;
        bool connected = false;
        bool ok = false;
        bool readyToConnect = false;
        bool timeSet = false;
        uint32_t lastSyncTime;

    private:
        void initEthAndNTP();
    public:
        Clock(EthernetUDP* udp, NTPClient* ntp, RTC_DS3231* rtc);
        int begin();
        void update();
        String getTimeFormated();
        uint64_t getEpoch();
        uint16_t minutesFromMidnight();
        static String minutesToTimeFormated(int minutes);
        void connect();
        uint8_t getDay();
        String getIP();
        String getTimezone();
};