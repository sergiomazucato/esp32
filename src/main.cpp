#include <Arduino.h>
#include <WiFiClient.h>
#include <ESP32WebServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "ArduinoJson.h"
#include <Preferences.h>
#include <TaskScheduler.h>

// TODO: https://github.com/me-no-dev/ESPAsyncWebServer/

#define AP_SSID  "SA_CONFIG"

// Callback methods prototypes
void wifiOnDisconnect();
void reboot();

ESP32WebServer server(80);
Preferences preferences;
String wifi_ssid, wifi_password;

Task tryToConnect(6000, TASK_FOREVER, &wifiOnDisconnect);
Scheduler runner;

////////////////////////////////////////////////////////
/// WIFI
////////////////////////////////////////////////////////

void wifiOnConnect()
{
  WiFi.mode(WIFI_MODE_STA);
}

void wifiOnDisconnect()
{
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
}

void handleWifi() {

    // StaticJsonBuffer<200> jsonBuffer;
    // JsonObject& root = jsonBuffer.parseObject("{ssid\": \"mzk\",\"password\": \"as9df1AS9DF1AS(DF!\"}");
    //
    // wifi_ssid = "";
    // wifi_password = "";
    // root["ssid"].printTo(wifi_ssid);
    // root["password"].printTo(wifi_password);
    //
    //
    // preferences.clear();
    // preferences.begin("wifi", false);
    // preferences.putString("ssid", wifi_ssid);
    // preferences.putString("password", wifi_password);
    // preferences.end();
    //
    // ESP.restart();
  //
  if (server.method() == HTTP_POST) {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(server.arg(0));

    wifi_ssid = "";
    wifi_password = "";
    root["ssid"].printTo(wifi_ssid);
    root["password"].printTo(wifi_password);

    wifi_ssid.replace("\"", "");
    wifi_password.replace("\"", "");

    preferences.clear();
    preferences.begin("wifi", false);
    preferences.putString("ssid", wifi_ssid);
    preferences.putString("password", wifi_password);
    preferences.end();

    server.send(200, "application/json", "{\"ssid\": \"" + wifi_ssid + "\", \"password\": \"" + wifi_password + "\"}");
    ESP.restart();
  } else {
    server.send(200, "application/json", "{\"ssid\": \"" + wifi_ssid + "\", \"password\": \"hidden\"}");
  }
}

void WiFiEvent(WiFiEvent_t event)
{
  Serial.print("WiFi Event: ");
  Serial.println(event);

  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());

  switch (event) {

    case SYSTEM_EVENT_AP_START:
      //can set ap hostname here
      WiFi.softAPsetHostname(AP_SSID);
      //enable ap ipv6 here
      WiFi.softAPenableIpV6();
      break;

    case SYSTEM_EVENT_STA_START:
      //set sta hostname here
      WiFi.setHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      //enable sta ipv6 here
      WiFi.enableIpV6();
      break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
      //both interfaces get the same event
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      wifiOnConnect();
      tryToConnect.disable();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      if (tryToConnect.isEnabled() == false) {
        tryToConnect.enable();
      }
      break;
    default:
      break;
  }
}

////////////////////////////////////////////////////////
/// CONTROLERS
////////////////////////////////////////////////////////

void handleRoot() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["port"] = 2;

  JsonObject& current_state = root.createNestedObject("current");
  current_state["o_dimmer"] = 0.2;
  current_state["i_switch"] = 1;

  JsonObject& options = root.createNestedObject("options");
  options["o_dimmer"] = "0..1";
  options["i_switch"] = "0,1";

  String response;
  root.printTo(response);

  server.send(200, "application/json", response);
}

void handleLed() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(server.arg(0));

  String nome = root["nome"];
  server.send(200, "text/plain", nome);

  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());
}

void handleNotFound(){
  String message = "{\"error\": \"page not found\"}";
  server.send(404, "application/json", message);
}

////////////////////////////////////////////////////////
/// MAIN
////////////////////////////////////////////////////////

void setup(void){
  Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_APSTA);

  WiFi.softAP(AP_SSID);
  preferences.begin("wifi", false);
  wifi_ssid =  preferences.getString("ssid", "none");
  wifi_password =  preferences.getString("password", "none");
  // wifi_ssid =  "mzk";
  // wifi_password =  "as9df1AS9DF1AS(DF!";
  preferences.end();

  runner.init();
  runner.addTask(tryToConnect);
  tryToConnect.enable();

  MDNS.begin("smartautomation");

  server.on("/", handleRoot);
  server.on("/api/v1/wifi", handleWifi);
  server.on("/api/v1/led", handleLed);

  server.onNotFound(handleNotFound);
  server.begin();
}

void loop(void){
  runner.execute();
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.handleClient();
}
