#pragma once
#include <Arduino.h>
#ifdef ESP32
    #include <WiFi.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>

#include <PubSubClient.h>

#include "wled.h"

#ifndef MC3DPRINTERLED_ENABLED
  #define MC3DPRINTERLED_ENABLED
#endif
typedef enum {
  EVT_WAIT_WIFI = 0,
  EVT_OFF,
  EVT_CONNECTED,
  EVT_IDLE,
  EVT_PRINT_START,
  EVT_BED_HEATING,
  EVT_NOZZLE_HEATING,
  EVT_HOMING,
  EVT_PROGRESS,
  EVT_PAUSE,
  EVT_ERROR,
  EVT_COOLING
} PrinterStates;
WiFiClientSecure wifiSecureClient;
PubSubClient mqttClient(wifiSecureClient);
unsigned long mqttattempt = (millis()-3000);
String device_topic;
String report_topic;
String clientId = "BLLED-";
/*
 * Usermods allow you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * 
 * This is an example for a v2 usermod.
 * v2 usermods are class inheritance based and can (but don't have to) implement more functions, each of them is shown in this example.
 * Multiple v2 usermods can be added to one compilation easily.
 * 
 * Creating a usermod:
 * This file serves as an example. If you want to create a usermod, it is recommended to use usermod_v2_empty.h from the usermods folder as a template.
 * Please remember to rename the class and file to a descriptive name.
 * You may also use multiple .h and .cpp files.
 * 
 * Using a usermod:
 * 1. Copy the usermod into the sketch folder (same folder as wled00.ino)
 * 2. Register the usermod by adding #include "usermod_filename.h" in the top and registerUsermod(new MyUsermodClass()) in the bottom of usermods_list.cpp
 */

//class name. Use something descriptive and leave the ": public Usermod" part :)
class MC3DPrinterLED : public Usermod {

  private:

    PrinterStates CurrentState = EVT_WAIT_WIFI;
    // Private class members. You can declare variables and functions only accessible to your usermod here
    bool enabled = true;
    bool initDone = false;
    unsigned long lastTime = 0;

    // string that are used multiple time (this will save some flash memory)
    static const char _name[];
    static const char _enabled[];


  public:

    // non WLED related methods, may be used for data exchange between usermods (non-inline methods should be defined out of class)

    /**
     * Enable/Disable the usermod
     */
    inline void enable(bool enable) { enabled = enable; }

    /**
     * Get usermod enabled/disabled state
     */
    inline bool isEnabled() { return enabled; }

    // methods called by WLED (can be inlined as they are called only once but if you call them explicitly define them out of class)

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * readFromConfig() is called prior to setup()
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() {
      // do your set-up here
      //Serial.println("Hello from my usermod!");
      initDone = true;
      Serial.begin(115200);
      Serial.println("Setup Called");
    }


    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() {
      CurrentState = EVT_OFF;
      setupMqtt();
    }
    void connectMqtt(){
      Serial.println(F("Connected Called:"));

      device_topic = String("device/00M00A2B0800731");
      report_topic = device_topic + String("/report");

      if (!mqttClient.connected() && (millis() - mqttattempt) >= 3000){   
          if (mqttClient.connect(clientId.c_str(),"bblp","392b9566")){
              Serial.println(F("Connected to mqtt"));
              Serial.println(report_topic);
              mqttClient.subscribe(report_topic.c_str());
          }else{
              switch (mqttClient.state())
              {
              case -4: // MQTT_CONNECTION_TIMEOUT
                  Serial.println(F("MQTT TIMEOUT"));
                  break;
              case -2: // MQTT_CONNECT_FAILED
                  Serial.println(F("MQTT CONNECT_FAILED"));
                  break;
              case -3: // MQTT_CONNECTION_LOST
                  Serial.println(F("MQTT CONNECTION_LOST"));
                  break;
              case -1: // MQTT_DISCONNECTED
                  Serial.println(F("MQTT DISCONNECTEDT"));
                  break;
              case 1:
                  break;
              case 2:
                  break;
              case 3:
                  break;
              case 4:
                  break;
              case 5: // MQTT UNAUTHORIZED
                  Serial.println(F("MQTT UNAUTHORIZED"));
                  ESP.restart();
                  break;
              }
          }
      }
    }

    void setupMqtt(){
        clientId += String(random(0xffff), HEX);
        Serial.println(F("Setting up MQTT with ip: "));
        Serial.println("10.0.0.47");
        wifiSecureClient.setInsecure();
        mqttClient.setBufferSize(2096); //4096
        mqttClient.setServer("10.0.0.47", 8883);
        // mqttClient.setCallback(mqttCallback);
        //mqttClient.setSocketTimeout(20);
        Serial.println(F("Finished setting up MQTT, Attempting to connect"));
        connectMqtt();
    }
    void mqttCallback(char *topic, byte *payload, unsigned int length){
      DynamicJsonDocument messageobject(2096);
      auto deserializeError = deserializeJson(messageobject, payload, length);
      if (!deserializeError){
          if (!messageobject.containsKey("print")) {
              return;
          }
          serializeJson(messageobject, Serial);
          // ParseCallback(messageobject);
      }else{
          Serial.println(F("Deserialize error while parsing mqtt"));
      }
    }
    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     * 
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     * 
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */
    void loop() {
      // if usermod is disabled or called during strip updating just exit
      // NOTE: on very long strips strip.isUpdating() may always return true so update accordingly
      if (!enabled || strip.isUpdating()) return;

      // do your magic here
      if (millis() - lastTime > 1000) {
        //Serial.println("I'm alive!");
        lastTime = millis();
      }

      //USE currentPreset to set preset.
    }

    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject& root)
    {
      if (!initDone) return;  // prevent crash on boot applyPreset()

      JsonObject usermod = root[FPSTR(_name)];
      if (!usermod.isNull()) {
        // expect JSON usermod data in usermod name object: {"ExampleUsermod:{"user0":10}"}
        userVar0 = usermod["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
      }
      // you can as well check WLED state JSON keys
      //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
    }

    /*
     * handleOverlayDraw() is called just before every show() (LED strip update frame) after effects have set the colors.
     * Use this to blank out some LEDs or set them to a different color regardless of the set effect mode.
     * Commonly used for custom clocks (Cronixie, 7 segment)
     */
    void handleOverlayDraw()
    {
      //strip.setPixelColor(0, RGBW32(0,0,0,0)) // set the first pixel to black
    }  

    /**
     * handling of MQTT message
     * topic only contains stripped topic (part after /wled/MAC)
     */
    bool onMqttMessage(char* topic, char* payload) {
      // DynamicJsonDocument messageobject(2096);
      // auto deserializeError = deserializeJson(messageobject, payload);
      // if (!deserializeError){
      //     if (!messageobject.containsKey("print")) {
      //         return;
      //     }
      //     HandleMessage(messageobject);
      // }else{
      //     Serial.println(F("Deserialize error while parsing mqtt"));
      // }

      //M104 Hotend heating
      return false;
    }

    /**
     * onMqttConnect() is called when MQTT connection is established
     */
    void onMqttConnect(bool sessionPresent) {
      CurrentState = EVT_CONNECTED;
    }
    void HandleMessage(JsonDocument &message){

    }

    /**
     * onStateChanged() is used to detect WLED state change
     * @mode parameter is CALL_MODE_... parameter used for notifications
     */
    void onStateChange(uint8_t mode) {
      // do something if WLED state changed (color, brightness, effect, preset, etc)
    }


    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
      return USERMOD_ID_3DPRINTER_LED;
    }

   //More methods can be added in the future, this example will then be extended.
   //Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};


// add more strings here to reduce flash memory usage
const char MC3DPrinterLED::_name[]    PROGMEM = "MC3DPrinterLED";
const char MC3DPrinterLED::_enabled[] PROGMEM = "enabled";

