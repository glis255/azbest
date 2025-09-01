#include "clock.h"

Clock::Clock(EthernetUDP* udp, NTPClient* ntp, RTC_DS3231* rtc): udp(udp), ntp(ntp), rtc(rtc) {}

void Clock::initEthAndNTP() {
    Serial.println("Initializing Ethernet and NTP.");
    uint8_t mac[6] = MAC_ADDRESS;
    Ethernet.begin(mac);
    ntp->begin();
    ntpClientReady = true;
    connected = true;
    readyToConnect = false;
    Serial.println("Ethernet and NTP ready!");
}

int Clock::begin() {
    if (Ethernet.hardwareStatus() == EthernetNoHardware) return 100;
    if (!rtc->begin()) return 101;

    if (Ethernet.linkStatus() == LinkON) {
        readyToConnect = true;
    } else if (!rtc->lostPower()) {
        timeUpdatedFromRTCTimestamp = millis();
        setTime(tz.toLocal(rtc->now().unixtime() + (timeOffsetHours * 3600)));
        timeSet = true;
    }

    return 0;
}

void Clock::update() {
    uint32_t rtcTime = rtc->now().unixtime();
    uint32_t ntpTime = ntp->getEpochTime();
    time_t timelibTime = now();

    if (!ntpClientReady && (Ethernet.linkStatus() == LinkON)) {
        readyToConnect = true;
    }

    if (ntpClientReady) {
        if (Ethernet.linkStatus() == LinkON) {
            if (ntp->update()) {
                setTime(tz.toLocal(ntp->getEpochTime() + (timeOffsetHours * 3600)));
                timeSet = true;
                if (ntpTime != rtcTime) {
                    ntpTime = ntp->getEpochTime();
                    rtc->adjust(ntpTime);
                    Serial.println("Adjusted RTC from NTP");
                }
            }
        }

        if (!connected && Ethernet.linkStatus() == LinkON) connected = true;
        if (connected && Ethernet.linkStatus() == LinkOFF) connected = false;
    }

    if (!connected && millis() - timeUpdatedFromRTCTimestamp >= RTC_UPDATE_TIMEOUT) {
        Serial.println("Updated time from RTC");
        timeUpdatedFromRTCTimestamp = millis();
        setTime(tz.toLocal(rtc->now().unixtime() + (timeOffsetHours * 3600)));
    }
}

String Clock::getTimeFormated() {
    int hours = hour();
    int minutes = minute();
    int secs = second();

    String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
    String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);
    String secondStr = secs < 10 ? "0" + String(secs) : String(secs);

    return hoursStr + ":" + minuteStr + ":" + secondStr;
}

uint16_t Clock::minutesFromMidnight(){
    return hour() * 60 + minute();
}

String Clock::minutesToTimeFormated(int minutes) {
    int hours = minutes / 60;
    int minutesModulo = minutes % 60;

    String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
    String minuteStr = minutesModulo < 10 ? "0" + String(minutesModulo) : String(minutesModulo);

    return hoursStr + ":" + minuteStr;
}

void Clock::connect() {
    initEthAndNTP();
    update();
}

uint64_t Clock::getEpoch() {
    if (connected) {
        return ntp->getEpochTime();
    } else {
        return rtc->now().unixtime();
    }
}

uint8_t Clock::getDay() {
    uint8_t day = ntp->getDay();

    switch (day) {
    case 0:
        return SUNDAY;
    case 1:
        return MONDAY;
    case 2:
        return TUESDAY;
    case 3:
        return WENSDAY;
    case 4:
        return THURSDAY;
    case 5:
        return FRIDAY;
    case 6:
        return SATURDAY;
    default:
        return 0;
    }
}

String Clock::getIP() {
    if (connected) {
        return Ethernet.localIP().toString();
    } else {
        return "";
    }
}

String Clock::getTimezone() {
    return tz.locIsDST(getEpoch()) ? "GMT+2" : "GMT+1";
}