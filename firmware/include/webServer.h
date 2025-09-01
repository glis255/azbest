#pragma once
#include <Arduino.h>
#include <UIPEthernet.h>
#include "alarmManager.h"
#include "clock.h"
#include "websites.h"
#include "ArduinoJson.h"
#include <EEPROM.h>
#include <flashOrg.h>

class WebServer {
private:
    EthernetServer server = EthernetServer(80);
private:
public:
    WebServer();
    void begin();
    void handle(AlarmManager* alarms, Clock* clk);
};

class Request {
private:
public:
    String method;
    String path;
    bool internalError = false;
    String errorCause;
    String headers[32];
    EthernetClient* client;
    String data;
private:
public:
    Request(EthernetClient* client);
};

class Response {
private:
public:
    uint16_t code = 200;
    String status = "OK";
    EthernetClient* client;
    String headers[32];
    uint8_t headersCount = 0;
    String contentType = "text/html";
private:
    // Send status and headers
    void transmissionBegin();
public:
    Response(Request* request);
    void setStatus(uint16_t code, String status);
    void send(String data);
    void sendStatus(uint16_t code, String status);
    void sendStatus();
    // Returns true if header was added successfully
    bool addHeader(String header);
};