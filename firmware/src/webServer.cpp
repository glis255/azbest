#include "webServer.h"

WebServer::WebServer() {
}

void WebServer::begin() {
    server.begin();
    Serial.println("Started server");
}

void WebServer::handle(AlarmManager* alarms, Clock* clk) {
    EthernetClient client = server.available();

    if (client) {
        Request req(&client);
        Response res(&req);

        if (req.internalError) {
            res.setStatus(500U, "Internal Server Error");

            char buffer[1024]; // 1 KB

            if (req.errorCause.length() > 0) {
                sprintf(
                    buffer, 
                    "<!DOCTYPE HTML>"
                    "<html>"
                    "<h1>"
                    "Error 500 Internal Server Error"
                    "</h1>"
                    "<p>"
                    "%s"
                    "</p>"
                    "</html>",
                    req.errorCause
                );
            } else {
                strcpy(
                    buffer,
                    "<!DOCTYPE HTML>"
                    "<html>"
                    "<h1>"
                    "Error 500 Internal Server Error"
                    "</h1>"
                    "</html>"
                );
            }

            res.send(buffer);
        } else {
            Serial.print(req.method);
            Serial.print(" => ");
            Serial.println(req.path);

            bool authorized = false;
            String password = EEPROM.readString(WEBSERVER_PASSWORD);
            for (auto header : req.headers) {
                if (header == String("Authorization: Basic ") + password) authorized = true;
            }

            if (!authorized) {
                res.addHeader("WWW-Authenticate: Basic realm=\"Azbest-3\"");
                res.code = 401;
                res.status = "Unauthorized";
                res.send(HTML_401);
                return;
            }

            char buffer[1024]; // 1 KB

            if (req.method == "GET") {
                if (req.path == "/") {
                    res.send(HTML_INDEX);
                } else if (req.path == "/app-index.js") {
                    res.contentType = "text/javascript";
                    res.send(JS_APP);
                } else if (req.path == "/style.css") {
                    res.contentType = "text/css";
                    res.send(CSS_STYLE);
                } else if (req.path == "/clock") {
                    JsonDocument clockData;

                    clockData["epoch"] = clk->getEpoch();
                    clockData["formated"] = clk->getTimeFormated();
                    clockData["lastSync"] = clk->lastSyncTime;
                    clockData["timezone"] = clk->getTimezone();
                    clockData["mac"] = MAC_ADDRESS_STRING;

                    serializeJson(clockData, &buffer, 1024);

                    res.contentType = "application/json";
                    res.send(buffer);
                } else if (req.path == "/alarms") {
                    JsonDocument alarmData;

                    alarmData["planSelected"] = alarms->plan;
                    alarmData["alarmDuration"] = alarms->alarmDuration;
                    alarmData["alarmsHalted"] = alarms->alarmsHalted;
                    alarmData["alarmCount"] = alarms->alarmsInPreset;
                    alarmData["planSelectedName"] = alarms->getPlanName();
                    alarmData["planSelectedWeekdays"] = alarms->getPlanWeekDays(alarms->plan);

                    serializeJson(alarmData, &buffer, 1024);

                    res.contentType = "application/json";
                    res.send(buffer);
                } else if (req.path.startsWith("/alarms/") && req.path.length() == 9) {
                    uint8_t plan = req.path.charAt(8) - '0';

                    if (plan >= 5U) {
                        res.sendStatus(400, "Bad Request");
                    } else {
                        uint16_t planAlarms[128];

                        alarms->getPlan(plan, planAlarms);

                        JsonDocument payload;

                        payload["alarms"].to<JsonArray>();

                        for (uint8_t i = 0; i < 128; i++) {
                            if (planAlarms[i] == 0xffff) break;
                            payload["alarms"][i] = planAlarms[i];
                        }

                        payload["weekdays"] = alarms->getPlanWeekDays(plan);
                        payload["name"] = alarms->getPlanName(plan);

                        serializeJson(payload, &buffer, 1024);

                        res.contentType = "application/json";
                        res.send(buffer);
                    }
                } else {
                    res.setStatus(404U, "Not Found");
                    res.send(HTML_404);
                }
            } else if (req.method == "POST") {
                if (req.path.startsWith("/alarms/") && req.path.length() == 9) {
                    uint8_t plan = req.path.charAt(8) - '0';

                    if (plan >= 5U) {
                        res.sendStatus(400, "Bad Request");
                    } else {
                        JsonDocument jsonData;
                        DeserializationError error = deserializeJson(jsonData, req.data);

                        if (error || jsonData["alarms"].size() > 127 || jsonData["weekdays"].isNull() || jsonData["weekdays"].as<uint8_t>() > 127 || jsonData["name"].as<String>().length() > 32) {
                            res.sendStatus(400, "Bad Request");
                        } else if (jsonData["alarms"].size() == 0) {
                            uint16_t buffer[] = { 0xffff };
                            alarms->editPlan(plan, buffer);
                            alarms->setWeekdays(plan, jsonData["weekdays"]);

                            if (plan == alarms->plan) {
                                alarms->loadAlarms();
                            }

                            res.sendStatus(201, "Created");
                        } else {
                            uint16_t alarmsBuffer[128];

                            uint16_t previousAlarmTime = 0xffff;

                            bool transferSuccessful = true;

                            for (uint8_t i = 0; i < jsonData["alarms"].size(); i++) {
                                uint16_t alarm = jsonData["alarms"][i];
                                if ((alarm > previousAlarmTime) || alarm > LARGEST_HOUR) {
                                    res.sendStatus(400, "Bad Request");
                                    transferSuccessful = false;
                                    break;
                                }
                                alarmsBuffer[i] = alarm;
                                alarmsBuffer[i+1] = 0xffff;
                                previousAlarmTime = alarm;
                            }

                            if (!alarms->setPlanName(plan, jsonData["name"])) {
                                res.sendStatus(400, "Bad Request");
                                transferSuccessful = false;
                            }

                            if (transferSuccessful) {
                                alarms->editPlan(plan, alarmsBuffer);
                                alarms->setWeekdays(plan, jsonData["weekdays"]);

                                if (plan == alarms->plan) {
                                    alarms->loadAlarms();
                                }

                                res.sendStatus(201, "Created");
                            }
                        }
                    }
                } else if (req.path == "/alarms") {
                    JsonDocument jsonData;
                    DeserializationError error = deserializeJson(jsonData, req.data);

                    if (error) {
                        res.setStatus(400, "Bad Request");
                        res.contentType = "text/plain";
                        res.send("Data malfolmed");
                        return;
                    }

                    uint8_t planSelected = jsonData["planSelected"];
                    if (!jsonData["planSelected"].isNull()) {
                        if (planSelected >= 5) {
                            res.setStatus(400, "Bad Request");
                            res.contentType = "text/plain";
                            res.send("Plan out of range.");
                            return;
                        }

                        alarms->plan = planSelected;
                        alarms->loadAlarms();
                    }

                    res.sendStatus(200, "OK");
                } else if (req.path == "/trigger") {
                    alarms->alarmTriggered = true;
                    res.sendStatus(200, "OK");
                } else if (req.path == "/password") {
                    JsonDocument jsonData;
                    DeserializationError error = deserializeJson(jsonData, req.data);

                    if (error || jsonData["password"].isNull() || jsonData["password"].as<String>().length() > 63) {
                        res.setStatus(400, "Bad Request");
                        res.contentType = "text/plain";
                        res.send("Data malfolmed");
                        return;
                    }

                    EEPROM.writeBytes(WEBSERVER_PASSWORD, jsonData["password"].as<String>().c_str(), jsonData["password"].as<String>().length() + 1);
                    EEPROM.commit();

                    res.sendStatus(200, "OK");
                } else {
                    res.sendStatus(400, "Bad Request");
                }
            } else {
                res.sendStatus(501, "Not Implemented");
            }
        }
    }
}

Request::Request(EthernetClient* client) {
    this->client = client;
    if (client) {
        char requestMethodBuffer[10];
        char requestPathBuffer[100];

        uint8_t word = 0;
        uint8_t headerNumber = 0;
        uint8_t characterNumber = 0;
        bool firstLine = true;
        char previousChar = '\0';
        char headerBuffer[256];
        while (client->available() > 0) {
            char c = client->read();

            if (firstLine && c == '\n') firstLine = false;

            if (c == ' ' && firstLine) {
                word++;
                characterNumber = 0;
            }

            if (word == 0) {
                if (characterNumber < 9) {
                    requestMethodBuffer[characterNumber] = c;
                    requestMethodBuffer[characterNumber + 1] = '\0';
                    characterNumber++;
                } else {
                    internalError = true;
                    errorCause = "Method too long";
                }
            } else if (word == 1) {
                if (characterNumber < 99) {
                    requestPathBuffer[characterNumber] = c;
                    requestPathBuffer[characterNumber + 1] = '\0';
                    characterNumber++;
                } else {
                    internalError = true;
                    errorCause = "Path too long";
                }
            }

            if (c == '\r' && previousChar == '\r') { // data block start; the previous byte was eaten by header parser so we check for \r\r
                client->read(); // eat last \n
                break;
            }

            if (!firstLine) { // headers
                if (headerNumber > 32) {
                    internalError = true;
                    errorCause = "Too many headers";
                    break;
                }

                if (c != '\r') {
                    if (characterNumber > 253) {
                        internalError = true;
                        errorCause = "Header too long";
                    } else {
                        headerBuffer[characterNumber] = c;
                        headerBuffer[characterNumber + 1] = '\0';
                        characterNumber++;
                    }
                } else { // eol
                    client->read(); // eat that \n
                    headers[headerNumber] = String(headerBuffer);
                    headerNumber++;
                    characterNumber = 0;
                } 
            }

            previousChar = c;
        }

        if (client->available() > 0) { // data
            uint64_t size = client->available();
            char* dataArrPtr = (char*) malloc(size + 1);

            if (dataArrPtr == NULL) {
                internalError = true;
                errorCause = "Not enough memory to process data";
            } else {
                for (uint64_t i = 0; i < size; i++) {
                    dataArrPtr[i] = client->read();
                    dataArrPtr[i + 1] = '\0';
                }
            }

            data = String(dataArrPtr);
            free(dataArrPtr);
        }

        method = String(requestMethodBuffer);
        method.trim();
        path = String(requestPathBuffer);
        path.trim();
    }
}

void Response::transmissionBegin() {
    this->client->print("HTTP/1.1 ");
    this->client->print(code, DEC);
    this->client->println(" " + status);
    this->client->println("Content-Type: " + this->contentType);
    for (uint8_t i = 0; i < headersCount; i ++) {
        this->client->println(headers[i]);
    }
    this->client->println();
}

Response::Response(Request* req) {
    this->client = req->client;
    this->addHeader("Server: Azbest-3/3.0");
    this->addHeader("Connection: close");
    this->addHeader("Access-Control-Allow-Origin: *");
}

void Response::setStatus(uint16_t code, String status) {
    this->code = code;
    this->status = status;
}

void Response::send(String data) {
    this->transmissionBegin();
    this->client->print(data);
    this->client->stop();
}

void Response::sendStatus(uint16_t code, String status) {
    this->setStatus(code, status);
    this->transmissionBegin();
    this->client->stop();
}

void Response::sendStatus() {
    this->transmissionBegin();
    this->client->stop();
}

bool Response::addHeader(String header) {
    if (headersCount < 32) {
        headers[headersCount] = header;
        headersCount++;
        return true;
    } else {
        return false;
    }
}