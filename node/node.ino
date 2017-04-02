/**
 * ESP Firmeware for switch network
 * 
 * This Software provides: 
 *  - A captive Portal to setup wifi connection
 *  - a preference set route to store: 
 *      - a new hostname
 *      - a post url 
 *  - a route to get the current date
 *  - a route to reset the eeprom
 *  - routes to set the switch on and off
 *
 *
 */
 
 
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// Controll LED
const int led = 5;

// SIPO
const int SER = 2;
const int RCLK = 13;
const int SRCLK = 14;
const int SRCLR = 16;

// Storage EEPROM Adress
int eepromNameAddress = 0;
byte value;

// Timer
os_timer_t myTimer;
volatile bool tickOccured;
volatile bool toggle;

WiFiManager wifiManager;
WiFiClient client;
ESP8266WebServer server ( 80 );

// Captive Portal Callback
// =======================================================================================
void configModeCallback (WiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    //if you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());
}

// Timer Callback
// =======================================================================================
void timerCallback(void *pArg) {
    tickOccured = true;
}

// SIPO
// =======================================================================================
void storeValue(int16_t value) {
    digitalWrite(SRCLR, 0);
    delay(0.6);
    digitalWrite(SRCLR, 1);
    delay(0.6);
    for(int i=0; i<=16; i++){
      digitalWrite(SER, value >> i & 1);
      digitalWrite(SRCLK, 1);
      delay(0.6
      );
      digitalWrite(SRCLK, 0);
    }
    digitalWrite(RCLK, 1);
    delay(0.6);
    digitalWrite(RCLK, 0);
}


// ROUTE to /
// =======================================================================================
void handleRoot() {
    char temp[400];
    int sec = millis() / 1000;
    int min = sec / 60;
    int hr = min / 60;

    snprintf ( temp, 400,
"{ \
  \"title\": \"%s\", \
  \"uptime\": \"%02d:%02d:%02d\", \
  \"mac\": \"%s\" \
}",
    WiFi.hostname().c_str(), hr, min % 60, sec % 60, WiFi.macAddress().c_str());
    server.send ( 200, "application/json", temp );
}


// ROUTE to /set
// =======================================================================================
void setPreferences(){
    //digitalWrite ( led, 1 );
    char temp[400];
    String hostname = server.arg("hostname");
    if( hostname.length() > 0 ){
        Serial.printf("Setting new Hostname: %s\n", hostname.c_str());
        WiFi.hostname(hostname);
        for (int i = 0; i < hostname.length(); ++i) {
            EEPROM.write(32+i, hostname[i]);
        }
        EEPROM.write(32+hostname.length(), '\0');
        EEPROM.commit();      
    }
    // Send preferences back
    snprintf ( temp, 400,
"{ \
  \"hostname\": \"%s\" \
}",
    WiFi.hostname().c_str());
    server.send ( 200, "application/json", temp );
    //digitalWrite ( led, 0 );
    // Reboot
    ESP.reset();
    delay(1000);
}

// ROUTE to /switch
// =======================================================================================
void setSwitch(){
    char temp[400];
    String v = server.arg("v");
    if( v.length() > 0 ){
        int16_t f = atoi( v.c_str() );
         storeValue(f);
    } else {
      storeValue(0);
    }
    // Send preferences back
    snprintf ( temp, 400,
"{ \
  \"set\": \"%s\" \
}",
    v.c_str());
    server.send ( 200, "application/json", temp );
}

// 404
// =======================================================================================  
void handleNotFound() {
    //digitalWrite ( led, 1 );
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for ( uint8_t i = 0; i < server.args(); i++ ) {
        message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
    }
    server.send ( 404, "text/plain", message );
    //digitalWrite ( led, 0 );
}

// System Setup
// ======================================================================================= 
void setup() {
    Serial.begin(115200);
    pinMode ( led, OUTPUT );

    // SIPO
    pinMode ( SRCLR, OUTPUT );
    pinMode ( SRCLK, OUTPUT );
    pinMode ( RCLK, OUTPUT );
    pinMode ( SER, OUTPUT );
    storeValue(0);  // shut down all switches
    
    EEPROM.begin(4096);
    
    // Hostname
    String hostname = "";
    for (int i = 32; i < 96; ++i) {
        hostname += char(EEPROM.read(i));
    }
    Serial.print("EEPROM Hostname: ");
    Serial.println( hostname.c_str() );
    if(hostname.length() < 0){
        hostname = "switch";
    }
    WiFi.hostname(hostname.c_str());
    if ( MDNS.begin ( hostname.c_str() ) ) {
        Serial.println ( "MDNS responder started" );
    }  
    Serial.printf("Hostname: %s\n", WiFi.hostname().c_str());
        
    // set callback that gets called when connecting to previous WiFi fails, and enters 
    // Access Point mode
    wifiManager.setAPCallback(configModeCallback);

    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect()) {
        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    } 

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    // Timer
    os_timer_setfn(&myTimer, timerCallback, NULL);
    os_timer_arm(&myTimer, 1000 * 1, true);

    // Routes
    server.on ( "/", handleRoot );
    
    server.on ( "/on", []() {
        digitalWrite ( led, 1 );
        server.send(200, "application/json", "{\"led\": true}");
    });
    
    server.on ( "/off", []() {
        digitalWrite ( led, 0 );
        server.send(200, "application/json", "{\"led\": false}");
    });

  server.on ( "/switch", setSwitch );
    
    server.on("/reset", []() {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 4096; ++i) { EEPROM.write(i, 0); Serial.print("x"); }
        Serial.println("clearing done.");
        EEPROM.commit();
        wifiManager.resetSettings();
        server.send(200, "application/json", "{\"reset\": true}");
        delay(1000);
        ESP.reset();
        delay(1000);
    }); 
    
    server.on ("/set", setPreferences );
    
    server.onNotFound ( handleNotFound );
    
    server.begin();
    Serial.println ( "HTTP server started" );

    Serial.println( "Switch started." );
    
    tickOccured = true;
}

// MAINLOOP
// ======================================================================================= 
void loop() {
    server.handleClient();
    if (tickOccured == true) {
      toggle = (toggle ^ 1) & 1;
      digitalWrite ( led, toggle );
      tickOccured = false;
    }
    yield();
}

