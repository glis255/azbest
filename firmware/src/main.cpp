#include <Arduino.h>
#include "pins.h"
#include "UIPEthernet.h"
#include "TimeLib.h"
#include "NTPClient.h"
#include "RTClib.h"
#include "LiquidCrystal_I2C.h"
#include "menu.h"
#include <EEPROM.h>
#include "lcdMenu.h"
#include "io.h"
#include "clock.h"
#include "serialHandler.h"
#include "lcdCustomChars.h"
#include "flashOrg.h"
#include "webServer.h"
#include "esp_task_wdt.h"

#define WATCHDOG_TIMEOUT 10

EthernetUDP udp;
NTPClient timeClient(udp, "pl.pool.ntp.org", 0, 3600000 /* godzina */);
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Clock timeClock(&udp, &timeClient, &rtc);

AlarmManager alarms(&timeClock);
LcdMenu menu(&lcd, &alarms);
IO io(&alarms);
SerialHandler cSerial;
WebServer server;

void encoderChanged(ESPRotary& r) {
  io.encoderPosition = r.getPosition();
}

void setup() {
  esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
  esp_task_wdt_add(NULL);

  io.showColor({ 20, 0, 20 });
  cSerial.init();
  cSerial.print("\n\n");
  cSerial.println("Hello handsome :3");

  EEPROM.begin(2048);

  if (EEPROM.readUChar(FLASH_INIT_UCHAR) != 0x55) {
    EEPROM.writeUChar(FLASH_INIT_UCHAR, 0x55);

    EEPROM.writeUChar(PLAN_SELECTED_UCHAR, 0);
    EEPROM.writeUInt(ALARM_DURATION_UINT, 5000);
    EEPROM.writeBool(MENU_SCREEN_BLANKING_BOOL, true);
    EEPROM.writeBool(ALARMS_HALTED_BOOL, false);

    EEPROM.writeUInt(PLAN_0_OFFSET_UINT, 0xffff);
    EEPROM.writeUInt(PLAN_1_OFFSET_UINT, 0xffff);
    EEPROM.writeUInt(PLAN_2_OFFSET_UINT, 0xffff);
    EEPROM.writeUInt(PLAN_3_OFFSET_UINT, 0xffff);
    EEPROM.writeUInt(PLAN_4_OFFSET_UINT, 0xffff);

    EEPROM.writeUChar(PLAN_0_WEEKDAYS_UCHAR, ALL_DAYS);
    EEPROM.writeUChar(PLAN_1_WEEKDAYS_UCHAR, ALL_DAYS);
    EEPROM.writeUChar(PLAN_2_WEEKDAYS_UCHAR, ALL_DAYS);
    EEPROM.writeUChar(PLAN_3_WEEKDAYS_UCHAR, ALL_DAYS);
    EEPROM.writeUChar(PLAN_4_WEEKDAYS_UCHAR, ALL_DAYS);
    
    EEPROM.writeString(PLAN_0_NAME_STRING, "");
    EEPROM.writeString(PLAN_1_NAME_STRING, "");
    EEPROM.writeString(PLAN_2_NAME_STRING, "");
    EEPROM.writeString(PLAN_3_NAME_STRING, "");
    EEPROM.writeString(PLAN_4_NAME_STRING, "");

    EEPROM.writeString(WEBSERVER_PASSWORD, "YWRtaW46YWRtaW4=");

    EEPROM.commit();
    cSerial.println("Reseted flash");
  }
  
  menu.begin();

  lcd.clear();

  menu.printLine("AZBEST-3", 0);

  menu.printLine("Uruchamianie...", 1);

  timeClock.timeOffsetHours = 0;

  int clockStatus = timeClock.begin();

  if (clockStatus) {
    lcd.clear();
    lcd.print("Blad krytyczny");
    lcd.setCursor(0, 1);
    lcd.print("Kod ");
    lcd.print(clockStatus);
    while (true) {
      cSerial.print("[CRITICAL] Code ");
      cSerial.println(clockStatus);
      io.showColor({ 255, 0, 0 });
      delay(1000);
      io.showColor({ 0, 0, 0 });
      delay(1000);
    }
  }

  if (timeClock.readyToConnect) {
    io.showColor({ 0, 0, 50 });
    lcd.setCursor(0, 1);
    menu.printLine("Laczenie...", 1);
    timeClock.connect();
    server.begin();
  }

  menu.wakeUpScreen();

  if (!timeClock.connected) menu.setStatusMsg("Brak sieci");
  else menu.setStatusMsg("Rozruch OK");

  alarms.begin();

  io.encoder.setChangedHandler(encoderChanged);
}

uint32_t mainLoopTimestamp = millis();
uint32_t connectionLostTimestamp = millis();
void loop() {
  esp_task_wdt_reset();
  io.encoder.loop();
  alarms.emergencyAlarmUpdate();

  if (timeClock.readyToConnect) {
    io.showColor({ 0, 0, 50 });
    if (menu.screen == LcdMenu::ScreenMain) {
      menu.printLine("Laczenie...", 1);
    }
    timeClock.connect();
    menu.setStatusMsg("Polaczono");
    alarms.loadAlarms();
    server.begin();
  }

  if (millis() - mainLoopTimestamp >= 1000UL) {
    mainLoopTimestamp = millis();
    timeClock.update();

    if (timeClock.timeSet) {
      alarms.update();

      String mainMenuString = timeClock.getTimeFormated();
      String pernamentStatusString = "";

      mainMenuString += "  Plan ";
      mainMenuString += alarms.plan + 1;

      if (alarms.emergencyAlarm) {
        pernamentStatusString = "TRWA ALARM";
      } else if (alarms.alarmsHalted) {
        pernamentStatusString += "Dzwonki wyl.";
      } else if (!alarms.alarmsInPreset) {
        pernamentStatusString += "Brak dzwonkow";
      } else if ((alarms.weekdays & timeClock.getDay()) == 0) {
        pernamentStatusString += "Dzwonki dzis wyl";
      } else {
        pernamentStatusString += char(BELL_CHAR);

        pernamentStatusString += Clock::minutesToTimeFormated(alarms.queuedAlarms[alarms.numberOfAlarms - 1]);

        if (alarms.alarmsInPreset < 10) pernamentStatusString += "    ";
        else if (alarms.alarmsInPreset >= 10 && alarms.alarmsInPreset < 100) {
          if (((alarms.alarmsInPreset - alarms.numberOfAlarms) + 1) >= 10) pernamentStatusString += "    ";
          else pernamentStatusString += "    ";
        }

        pernamentStatusString += (alarms.alarmsInPreset - alarms.numberOfAlarms) + 1;
        pernamentStatusString += "/";
        pernamentStatusString += alarms.alarmsInPreset;
      }

      menu.setPernamentStatusMsg(pernamentStatusString);

      menu.setMainMenuString(mainMenuString);

      if (timeClock.connected) io.showColor({ 0, 20, 0 });
      else io.showColor({ 60, 10, 0 });
    } else {
      io.showColor({ 40, 0, 0 });

      menu.setMainMenuString("Czas nienast.");
      menu.setPernamentStatusMsg("Podlacz siec");
    }

    if (menu.screen == LcdMenu::ScreenMain) menu.update();
  }

  if (alarms.alarmTriggered) {
    io.triggerRelay();
    menu.setStatusMsg("Wyjscie aktywne");
    alarms.alarmTriggered = false;
  }

  menu.checkBlanking();

  auto serialAction = cSerial.handle();

  if (serialAction) {
    cSerial.println(serialAction);
    switch (serialAction) {
      case SerialHandler::MENU_ENTER:
        menu.enter();
        break;
      case SerialHandler::MENU_MOVE_RIGHT:
        menu.moveDown();
        break;
      case SerialHandler::MENU_MOVE_LEFT:
        menu.moveUp();
        break;
      case SerialHandler::TRIGGER:
        io.triggerRelay();
        break;
      case SerialHandler::RELOAD_ALARMS:
        alarms.loadAlarms();
        break;
      default:
        break;
    }
  }

  auto IOAction = io.update();

  if (IOAction) {
    switch (IOAction) {
    case IO::ENCODER_RIGHT:
      menu.moveDown();
      break;
    case IO::ENCODER_LEFT:
      menu.moveUp();
      break;
    case IO::BUTTON_PRESS:
      menu.enter();
      break;
    case IO::NO_ACTION:
      break;
    }
  }

  server.handle(&alarms, &timeClock);
}