#pragma once
#include <Arduino.h>
#include "LiquidCrystal_I2C.h"
#include "menu.h"
#include "alarmManager.h"

#define SCREEN_ACTIVE_TIMEOUT 30000UL
#define SCREEN_STATUS_MESSAGE_TIMEOUT 5000UL

class LcdMenu {
    private:
        LiquidCrystal_I2C* lcd;
        char statusMsgBuffer[17];
        char permanentStatusMsgBuffer[17];
        uint32_t statusMsgChangedTimestamp = 0;
        uint32_t screenActiveTimestamp = 0;
        uint8_t cursorLineNr = 0;
        uint8_t scrollOffset = 0;
        uint8_t* entriesTypesPtr;
        char mainMenuString[17];
        bool mainMenuStringUpdated = false;
        uint8_t selectedEntry = 0;
        uint8_t numOfEntries;
        bool* togglables[32];
        AlarmManager* alarms;

    private:
        void renderMenuEntries(String* entries, uint8_t* entryTypes, uint8_t numOfEntries);
        void renderPlanSelect();
        void renderEditPlan();
        void renderSlider(String sliderTitle, String sliderSuffix);

    public:
        enum Screen { ScreenMain = 0, ScreenMainMenu, ScreenProgram, ScreenNetwork, ScreenSystem };
        bool statusMsgVisible = false;
        Screen screen = ScreenMain;
        bool inMenu = false;
        bool screenBlanknig = false;
        bool* togglablesMainMenu[32];
        bool* togglablesProgramMenu[32];
        bool* togglablesNetwrokMenu[32];
        bool* togglablesSystemMenu[32];
        bool screenSleeping = false;
        bool promptActive = false;

    public:
        LcdMenu(LiquidCrystal_I2C* lcdDisplay, AlarmManager* alarms);
        void begin();
        void printLine(String string, int lineNum, int start = 0, int leaveSpace = 0);
        void setStatusMsg(String statusStr);
        void setPernamentStatusMsg(String statusStr);
        void wakeUpScreen();
        void setMainMenuString(String menu);
        void update();
        void enter();
        void moveUp();
        void moveDown();
        bool checkBlanking();
        void prompt(String name);
};