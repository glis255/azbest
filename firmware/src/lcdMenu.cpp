#include "LiquidCrystal_I2C.h"
#include "lcdMenu.h"
#include "lcdCustomChars.h"
#include <EEPROM.h>
#include "flashOrg.h"

#define HAWK_TUAH_DEBUG

const char chArrow[] = CHAR_ARROW;
const char chArrowBack[] = CHAR_ARROW_BACK;
const char chArrowDown[] = CHAR_ARROW_DOWN;
const char chArrowUp[] = CHAR_ARROW_UP;
const char chBell[] = CHAR_BELL;
// const char chNoConnection[] = CHAR_NO_CONNECTION;
// const char chX[] = CHAR_X;
const char chCheckmark[] = CHAR_CHECKMARK;
const char chCircle[] = CHAR_CIRCLE;

String mainMenuEntries[] = MAIN_MENU_ENTRIES;
uint8_t mainMenuEntriesTypes[] = MAIN_MENU_ENTRIES_TYPES;

String programMenuEntries[] = PROGRAM_MENU_ENTRIES;
uint8_t programMenuEntriesTypes[] = PROGRAM_MENU_ENTRIES_TYPES;

String networkMenuEntries[] = NETWORK_MENU_ENTRIES;
uint8_t networkMenuEntriesTypes[] = NETWORK_MENU_ENTRIES_TYPES;

String systemMenuEntries[] = SYSTEM_MENU_ENTRIES;
uint8_t systemMenuEntriesTypes[] = SYSTEM_MENU_ENTRIES_TYPES;

// togglables entry points
bool networkIp = false;
bool editPlan = false;
bool selectPlan = false;
bool alarmDuration = false;
bool inAlarmDuration = false;
bool passwordReset = false;
bool flashReset = false;
bool alarmsReset = false;

uint8_t selectedPlanNr = 1;
bool selectingPlan = false;
bool planSelected = false;
bool exitSelected = false;

bool clickToAccept = false;

uint8_t* sliderValuePtr;
uint8_t sliderTempValue = 0;
uint8_t sliderStep = 100;
uint16_t sliderValueMax = 2000;
uint8_t sliderValueMin = 0;
bool sliderAccept = false;
bool sliderRendering = false;
bool screenblankingSettingEEPROM;

bool ALARM = false;

LcdMenu::LcdMenu(LiquidCrystal_I2C* lcdDisplay, AlarmManager* alarms) {
    this->alarms = alarms;
    lcd = lcdDisplay;
    
    togglablesMainMenu[0] = &ALARM;

    togglablesProgramMenu[0] = &alarms->alarmsHalted;
    togglablesProgramMenu[1] = &selectPlan;
    togglablesProgramMenu[2] = &alarmDuration;
    togglablesProgramMenu[3] = &alarms->alarmTriggered;

    togglablesNetwrokMenu[0] = &networkIp;
    
    togglablesSystemMenu[0] = &screenBlanknig;
    togglablesSystemMenu[1] = &passwordReset;
}

void LcdMenu::renderMenuEntries(String* entries, uint8_t* entriesTypes, uint8_t numOfEntries) {
    this->entriesTypesPtr = entriesTypes;
    this->numOfEntries = numOfEntries;

    // debug
    Serial.print("scrollOffset ");
    Serial.println(scrollOffset);
    Serial.print("cursorLineNr ");
    Serial.println(cursorLineNr);
    Serial.print("Selected: ");
    Serial.println(entries[scrollOffset + cursorLineNr]);
    Serial.print("numOfEntries: ");
    Serial.println(numOfEntries);
    Serial.println("");

    selectedEntry = scrollOffset + cursorLineNr;

    // display entries
    printLine(entries[scrollOffset], 0, 1, 3);
    printLine(entries[scrollOffset + 1], 1, 1, 3);

    // select arrow
    uint8_t cursorStyle = ARROW_CHAR;
    uint8_t selectedEntryType = entriesTypes[selectedEntry];
    if (selectedEntryType == 0) cursorStyle = CIRCLE_CHAR;
    else if (selectedEntryType == 1) cursorStyle = ARROW_BACK_CHAR;

    lcd->setCursor(0, cursorLineNr);
    lcd->write(cursorStyle);
    lcd->setCursor(0, 1 - cursorLineNr);
    lcd->print(' ');

    // scroll arrows
    lcd->setCursor(15, 0);
    if (scrollOffset > 0) lcd->write(ARROW_UP_CHAR);
    else lcd->write(' ');

    lcd->setCursor(15, 1);
    if (scrollOffset < numOfEntries - 2) lcd->write(ARROW_DOWN_CHAR);
    else lcd->write(' ');

    // check togglables
    // line 0 togglable
    lcd->setCursor(14, 0);
    if ((entriesTypes[scrollOffset] & ENTRY_TOGGLE)) lcd->write(*togglables[entriesTypes[scrollOffset] ^ ENTRY_TOGGLE] ? CHECKMARK_CHAR : 'x');
    else lcd->write(' ');

    // line 1 togglable
    lcd->setCursor(14, 1);
    if ((entriesTypes[scrollOffset + 1] & ENTRY_TOGGLE)) lcd->write(*togglables[(entriesTypes[scrollOffset + 1]) ^ ENTRY_TOGGLE] ? CHECKMARK_CHAR : 'x');
    else lcd->write(' ');
}

void LcdMenu::renderPlanSelect() {
    // debug
    Serial.print("selectedPlanNr: ");
    Serial.println(selectedPlanNr);
    Serial.print("exitSelected: ");
    Serial.println(exitSelected);

    selectingPlan = true;
    planSelected = false;

    if (exitSelected) {
        printLine("Wyjscie", 0, 1);
        lcd->setCursor(0, 0);
        lcd->write(ARROW_BACK_CHAR);
    } else {
        char buffer[17];
        sprintf(buffer, "Plan %d", selectedPlanNr);
        printLine(buffer, 0);
    }

    printLine(((String)"Aktualny: ") + (alarms->plan + 1), 1);
}

void LcdMenu::renderEditPlan() {

}

/*
    To jest masakra
*/
void LcdMenu::renderSlider(String sliderTitle, String sliderSuffix) {
    sliderRendering = true;

    float progress = floor(float(sliderTempValue) / float(sliderValueMax) * 16);
    Serial.println(progress);

    String sliderUwU = "";
    
    for (uint i = 0; i <= 16; i++) {
        if (i < progress) sliderUwU += char(0xff);
        else if (i == progress) sliderUwU += char(0xdb);
        else sliderUwU += char(0x20);
    }

    printLine(sliderTitle + sliderTempValue + sliderSuffix, 0);
    printLine(sliderUwU, 1);
}

void LcdMenu::begin() {
    lcd->init();
    lcd->clear();
    lcd->backlight();

    lcd->createChar(ARROW_CHAR, chArrow);
    lcd->createChar(ARROW_DOWN_CHAR, chArrowDown);
    lcd->createChar(ARROW_UP_CHAR, chArrowUp);
    lcd->createChar(BELL_CHAR, chBell);
    // lcd->createChar(NO_CONNECTION_CHAR, chNoConnection);
    // lcd->createChar(5, chX);
    lcd->createChar(ARROW_BACK_CHAR, chArrowBack);
    lcd->createChar(CHECKMARK_CHAR, chCheckmark);
    lcd->createChar(CIRCLE_CHAR, chCircle);

    lcd->setCursor(0, 0);

    screenBlanknig = EEPROM.readBool(MENU_SCREEN_BLANKING_BOOL);
    screenblankingSettingEEPROM = screenBlanknig;
}

void LcdMenu::printLine(String string, int lineNum, int start, int leaveSpace) {
    String line = string;

    for (int i = string.length() - start; i < 16 - leaveSpace - start; i++) line += ' ';

    lcd->setCursor(start, lineNum);
    lcd->print(line);
}

void LcdMenu::setStatusMsg(String statusStr) {
    statusMsgChangedTimestamp = millis();
    strcpy(statusMsgBuffer, statusStr.c_str());
    statusMsgVisible = true;
    if (screen == ScreenMain) update();
}

void LcdMenu::setPernamentStatusMsg(String statusStr) {
    strcpy(permanentStatusMsgBuffer, statusStr.c_str());
    if (screen == ScreenMain) update();
}

void LcdMenu::wakeUpScreen() {
    screenActiveTimestamp = millis();
    lcd->backlight();
    screenSleeping = false;
}

void LcdMenu::setMainMenuString(String menu) {
    strcpy(mainMenuString, menu.c_str());
    mainMenuStringUpdated = true;
}

/*
    w sumie to nie wiem co tu się dzieje
    czasami wchodzi się do podmenu przez ustawianie entrypointów z konstruktora,
    a raz przez zmianę w screen
    też to jest zrobione na wskaźnikach, aby łatwo nie było
*/

void LcdMenu::update() {
    if (statusMsgVisible && (millis() - statusMsgChangedTimestamp >= SCREEN_STATUS_MESSAGE_TIMEOUT)) statusMsgVisible = false;

    if (clickToAccept) return;

    if (ALARM) {
        ALARM = false;
        if (!alarms->emergencyAlarm) {
            printLine("ALARM WLACZONY", 0);
            printLine("EWAKUACJA", 1);
            alarms->engageEmergencyAlarm();
            delay(2000);
        } else {
            lcd->clear();
            printLine("ALARM WYLACZONY", 0);
            alarms->disengageEmergencyAlarm();
            delay(2000);
        }
        update();
        return;
    }

    if (passwordReset) {
        passwordReset = false;
        EEPROM.writeString(WEBSERVER_PASSWORD, String("YWRtaW46YWRtaW4="));
        EEPROM.commit();
        printLine("Zresetowano", 0);
        printLine("Haslo dostepu", 1);
        Serial.println("Reseted password");
        delay(2000);
        update();
        return;
    }

    if (flashReset) {
        flashReset = false;
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
        Serial.println("Reseted flash");
        printLine("Zresetowano", 0);
        printLine("Pamiec Flash", 1);
        alarms->loadAlarms();
        delay(2000);
        update();
        return;
    }

    if (alarmsReset) {
        alarmsReset = false;
        EEPROM.writeUChar(PLAN_SELECTED_UCHAR, 0);
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
        EEPROM.commit();
        Serial.println("Reseted alarms");
        printLine("Zresetowano", 0);
        printLine("Dzwonki", 1);
        alarms->loadAlarms();
        delay(2000);
        update();
        return;
    }

    switch (screen) {
        case ScreenMain:
            inMenu = false;

            if (mainMenuStringUpdated) {
                printLine(mainMenuString, 0);
                if (statusMsgVisible) printLine(statusMsgBuffer, 1);
                else printLine(permanentStatusMsgBuffer, 1);
            }

            mainMenuStringUpdated = false;
            break;
        case ScreenMainMenu:
            inMenu = true;

            memcpy(togglables, togglablesMainMenu, 32);
            renderMenuEntries(mainMenuEntries, mainMenuEntriesTypes, MAIN_MENU_ENTRIES_SIZE);
            break;
        case ScreenProgram:
            if (editPlan) {
                if (!planSelected) renderPlanSelect();
                else {
                    planSelected = false;
                    clickToAccept = true;
                    printLine("UOH", 0);
                    printLine("", 1);
                }
            } else if (selectPlan) {
                if (!planSelected) renderPlanSelect();
                else {
                    alarms->plan = selectedPlanNr - 1;

                    alarms->loadAlarms();
                    EEPROM.writeUChar(PLAN_SELECTED_UCHAR, selectedPlanNr - 1);
                    EEPROM.commit();

                    selectingPlan = false;
                    selectPlan = false;
                    planSelected = false;
                    lcd->clear();
                    char buffer[17];
                    sprintf(buffer, "Wybrano plan %d", selectedPlanNr);
                    printLine(buffer, 0);
                    delay(2000);
                    selectedPlanNr = 1;
                    update();
                }
            } else if (alarmDuration) {
                if (!inAlarmDuration) {
                    sliderStep = 1U;
                    sliderValueMax = 20U;
                    sliderValueMin = 1U;
                    sliderTempValue = alarms->alarmDuration / 1000;
                    inAlarmDuration = true;
                }
                if (!sliderAccept) renderSlider("Czas dzwonka ", "s");
                else {
                    alarmDuration = false;
                    inAlarmDuration = false;
                    sliderAccept = false;
                    printLine("Ustawiono czas", 0);
                    printLine((String)"dzwonka na " + sliderTempValue + "s", 1);
                    EEPROM.writeUInt(ALARM_DURATION_UINT, sliderTempValue * 1000);
                    EEPROM.commit();

                    alarms->alarmDuration = sliderTempValue * 1000;
                    delay(2000);
                    update();
                }
            } else {
                inMenu = true;
                memcpy(togglables, togglablesProgramMenu, 32);
                renderMenuEntries(programMenuEntries, programMenuEntriesTypes, PROGRAM_MENU_ENTRIES_SIZE);
            }
            break;
        case ScreenNetwork:
            if (networkIp) {
                networkIp = false;
                clickToAccept = true;

                printLine("Adres IP:", 0);
                printLine(alarms->clock->getIP(), 1);
            } else {
                inMenu = true;
                memcpy(togglables, togglablesNetwrokMenu, 32);
                renderMenuEntries(networkMenuEntries, networkMenuEntriesTypes, NETWORK_MENU_ENTRIES_SIZE);
            }
            break;
        case ScreenSystem:
            memcpy(togglables, togglablesSystemMenu, 32);
            renderMenuEntries(systemMenuEntries, systemMenuEntriesTypes, SYSTEM_MENU_ENTRIES_SIZE);
            break;
        default:
            printLine("UNKNOWN SCREEN", 0);
            printLine("", 1);
            delay(1000);
            screen = ScreenMainMenu;
            update();
    }
}

void LcdMenu::enter() {
    Serial.println("[MENU] Enter");
    if (screenSleeping) {
        wakeUpScreen();
        return;
    }
    wakeUpScreen();

    if (clickToAccept) {
        clickToAccept = false;
        update();
        return;
    }

    if (!inMenu) screen = ScreenMainMenu;
    else if (selectingPlan) {
        if (exitSelected) {
            exitSelected = false;
            selectingPlan = false;

            editPlan = false;
            selectPlan = false;
        } else {
            planSelected = true;
        }
    } else if (sliderRendering) {
        sliderAccept = true;
        sliderRendering = false;
    } else {
        if (entriesTypesPtr[selectedEntry] == ENTRY_BACK) {
            switch (screen) {
            case ScreenProgram:
                screen = ScreenMainMenu;
                scrollOffset = 1;
                cursorLineNr = 0;
                break;

            case ScreenNetwork:
                screen = ScreenMainMenu;
                scrollOffset = 2;
                cursorLineNr = 0;
                break;

            case ScreenSystem:
                screen = ScreenMainMenu;
                scrollOffset = 3;
                cursorLineNr = 1;
                break;

            default:
                screen = ScreenMain;
                scrollOffset = 0;
                cursorLineNr = 0;
            }
        } else if (entriesTypesPtr[selectedEntry] & ENTRY_SCREEN) {
            screen = (Screen)(entriesTypesPtr[selectedEntry] ^ ENTRY_SCREEN);
            scrollOffset = 0;
            cursorLineNr = 0;
        } else if (entriesTypesPtr[selectedEntry] & ENTRY_TOGGLE) {
            *togglables[entriesTypesPtr[selectedEntry] ^ ENTRY_TOGGLE] = !*togglables[entriesTypesPtr[selectedEntry] ^ ENTRY_TOGGLE];
        } else if (entriesTypesPtr[selectedEntry] & ENTRY_TOGGLE_INTERNAL) {
            *togglables[entriesTypesPtr[selectedEntry] ^ ENTRY_TOGGLE_INTERNAL] = !*togglables[entriesTypesPtr[selectedEntry] ^ ENTRY_TOGGLE_INTERNAL];
        }
    }

    update();
}

void LcdMenu::moveUp() {
    Serial.println("[MENU] Up");
    if (screenSleeping) {
        wakeUpScreen();
        return;
    }
    wakeUpScreen();

    if (clickToAccept) return;
    if (!inMenu) screen = ScreenMainMenu;
    else if (selectingPlan) {
        if (selectedPlanNr > 1) selectedPlanNr--;
        else exitSelected = true;
    } else if (sliderRendering) {
        if (sliderTempValue > sliderValueMin) sliderTempValue -= sliderStep;
    } else {
        if (cursorLineNr == 1) cursorLineNr = 0;
        else if (scrollOffset > 0) scrollOffset--;
    }
    update();
}

void LcdMenu::moveDown() {
    Serial.println("[MENU] Down");
    if (screenSleeping) {
        wakeUpScreen();
        return;
    }
    wakeUpScreen();

    if (clickToAccept) return;
    if (!inMenu) screen = ScreenMainMenu;
    else if (selectingPlan) {
        if (exitSelected) exitSelected = false;
        else if (selectedPlanNr < 5) selectedPlanNr++;
    } else if (sliderRendering) {
        if (sliderTempValue < sliderValueMax) sliderTempValue += sliderStep;
    } else {
        if (cursorLineNr == 0) cursorLineNr = 1;
        else if (scrollOffset != numOfEntries - 2) scrollOffset++;
    }
    update();
}

bool LcdMenu::checkBlanking() {
    if (screenblankingSettingEEPROM != screenBlanknig) {
        screenblankingSettingEEPROM = screenBlanknig;
        EEPROM.writeBool(MENU_SCREEN_BLANKING_BOOL, screenBlanknig);
        EEPROM.commit();
    }
    
    if (screenBlanknig && millis() - screenActiveTimestamp >= SCREEN_ACTIVE_TIMEOUT) {
        lcd->noBacklight();
        screenSleeping = true;
        return true;
    } else {
        return false;
    }
}