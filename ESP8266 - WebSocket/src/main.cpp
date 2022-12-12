#include <Arduino.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>
#include <string>
WebSocketsServer webSocket = WebSocketsServer(5657);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
void switchEXTICallback();
#define CLK D1
#define DT D2
#define SW D3

int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";
unsigned long lastButtonPress = 0;

void setup()
{
    pinMode(CLK, INPUT);
    pinMode(DT, INPUT);
    pinMode(SW, INPUT_PULLUP);
    Serial.begin(9600);
    String apName = "SuperDial-" + String(ESP.getChipId());
    WiFiManager wm;
    bool wmRes = false;
    wmRes = wm.autoConnect(apName.c_str());
    if (!wmRes) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } else {
        Serial.println("connected...yeey :)");
    }
    lastStateCLK = digitalRead(CLK);
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    attachInterrupt(SW, switchEXTICallback, CHANGE);
}

ICACHE_RAM_ATTR void switchEXTICallback()
{
    if (digitalRead(SW) == LOW) {
        String sendStr = "btnPressed";
        Serial.println(sendStr);
        webSocket.sendTXT(0, sendStr);
    } else if (digitalRead(SW) == HIGH) {
        String sendStr = "btnReleased";
        Serial.println(sendStr);
        webSocket.sendTXT(0, sendStr);
    }
}

void loop()
{
    webSocket.loop();
    // Read the current state of CLK
    currentStateCLK = digitalRead(CLK);

    // If last and current state of CLK are different, then pulse occurred
    // React to only 1 state change to avoid double count
    if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {

        // If the DT state is different than the CLK state then
        // the encoder is rotating CCW so decrement
        if (digitalRead(DT) != currentStateCLK) {
            counter--;
            currentDir = "CCW";
        } else {
            // Encoder is rotating CW so increment
            counter++;
            currentDir = "CW";
        }

        Serial.print("Direction: ");
        Serial.print(currentDir);
        webSocket.sendTXT(0, currentDir);
        Serial.print(" | Counter: ");
        Serial.println(counter);
    }

    // Remember last CLK state
    lastStateCLK = currentStateCLK;
    // Put in a slight delay to help debounce the reading
    delay(1);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length)
{

    switch (type) {
    case WStype_DISCONNECTED:
        Serial.printf("[%u] Disconnected!\n", num);
        break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
    } break;
    case WStype_TEXT:
        Serial.printf("[%u] get Text: %s\n", num, payload);

        // send message to client
        // webSocket.sendTXT(num, "message here");

        // send data to all connected clients
        // webSocket.broadcastTXT("message here");
        break;
    case WStype_BIN:
        Serial.printf("[%u] get binary length: %u\n", num, length);
        hexdump(payload, length);

        // send message to client
        // webSocket.sendBIN(num, payload, length);
        break;
    }
}