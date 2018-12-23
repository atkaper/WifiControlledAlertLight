#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
 
//////////////////////////////////////////////////////////////////////////////////////////
// This code can be used inside a rotating flash-light, to get it controlled remotely.
// We will use it to monitor our kubernetes platform and api gateway at the office.
// It is just an extra bit of light, to get us to look up at the big status monitors ;-)
// On any real big issues, someone also get's the standard phone-call from the monitoring
// systems.
//
// Note: there's not much security in this code. But hey, it's just a flash-light...
// Anyone on the network will be able to control it, if they know the proper endpoints.
// However, not in our office, as the network guys have blocked client to client access
// in all Wifi access-points.
//
// The current version of this code will start a long-poll (http get) to an internet
// server. It waits for a command, or on time-out it will sleep for 2 seconds and start
// a new long-poll again. In between the long-poll's, a led will shortly flash, to
// indicate we are still polling. If there's a network disconnect error, the device will
// reboot, and keep the led on, until there's a connection again.
// On receipt of an invalid command, the led will flash a couple of times quickly.
// 
// To connect this device to the WLAN, it first starts up in access-point mode, you
// can then connect to it, to set up the proper Wifi credentials. It will re-enter this
// setup mode, if no Wifi connection can be made upon boot up.
// 
// 11/11/2018 Thijs Kaper
//////////////////////////////////////////////////////////////////////////////////////////
 
// serverPollingUrl will get the MAC address of this device added to it.
const String serverPollingUrl = "http://YOUR-SERVER.SOMEWERE/SOMEPATH/long-poll.php?user=";
 
// Access-point settings (and also OTA - over-the-air - ESP firmware update settings)
const char * accessPointName = "WifiAlertLight";
const char * accessPassword = "password";
 
// IO pin's (using WeMos D1 mini)
const int RELAY_PIN = D1;
const int LED_PIN = D0;
 
//////////////////////////////////////////////////////////////////////////////////////////
 
ESP8266WebServer server ( 80 );
 
 
// The setup method is similar to the C-language "main" method. It handles initialization.
// After setup, the ESP will keep the loop() method running forever, for further (event) processing.
void setup() {
 
  // Set pin modes, turn relay off, and led on
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  handleSetPin(RELAY_PIN, LOW);
  handleSetPin(LED_PIN, HIGH);
 
  // debug connection (uses serial port on the usb connector)
  Serial.begin(115200);
  Serial.println();
 
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
 
  // exit after config instead of connecting
  wifiManager.setBreakAfterConfig(true);
 
  // tries to connect to last known settings
  // if it does not connect it starts an access point with the specified name
  // and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(accessPointName, accessPassword)) {
    Serial.println("failed to connect, we should reset to see if it re-connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
 
  // if you get here you have connected to the WiFi
  handleSetPin(LED_PIN, LOW);
  Serial.println("connected...yeey :)");
  WiFi.printDiag(Serial);
 
  Serial.print("local ip: ");
  Serial.println(WiFi.localIP());
 
  // over-the-air updates allowed (note, during long-poll, OTA will hang, so send some commands to the light to get it going)
  ArduinoOTA.setHostname(accessPointName);
  ArduinoOTA.setPassword(accessPassword);
  ArduinoOTA.begin();
 
  // some build-in web-server path's, to simplify testing if there's no active long-poll server.
  server.on ( "/", handleRoot );
 
  server.on ( "/reboot", handleReboot );
  server.on ( "/reset", handleReboot );
 
  server.on ( "/setup", handleSetup );
  server.on ( "/config", handleSetup );
 
  server.on ( "/led/on", []()      { handleSetPin(LED_PIN, HIGH);     server.send ( 200, "text/plain", "OK" ); } );
  server.on ( "/led/off", []()     { handleSetPin(LED_PIN, LOW);      server.send ( 200, "text/plain", "OK" ); } );
  server.on ( "/led/pulse", []()   { handlePulsePin(LED_PIN, 1000);   server.send ( 200, "text/plain", "OK" ); } );
 
  server.on ( "/light/on", []()    { handleSetPin(RELAY_PIN, HIGH);   server.send ( 200, "text/plain", "OK" ); } );
  server.on ( "/light/off", []()   { handleSetPin(RELAY_PIN, LOW);    server.send ( 200, "text/plain", "OK" ); } );
  server.on ( "/light/pulse", []() { handlePulsePin(RELAY_PIN, 2000); server.send ( 200, "text/plain", "OK" ); } );
 
  server.onNotFound ( handleNotFound );
  server.begin();
}
 
void handleSetPin(int pin, int value) {
  Serial.println("handleSetPin; pin " + String(pin) + " set to " + String(value));
  digitalWrite(pin, value);
}
 
void handlePulsePin(int pin, int ms) {
  handleSetPin(pin, HIGH);
  delay(ms);
  handleSetPin(pin, LOW);
}
 
void handleReboot() {
  Serial.println("handleReboot");
  server.send ( 200, "text/plain", "rebooting..." );
  delay(3000);
  ESP.reset();
  delay(5000);
}
 
void handleRoot() {
  Serial.println("handleRoot");
  server.send ( 200, "text/plain", "This page intentionally left blank...\nThis is the WifiAlertLight, don't call us, we call a server for light on/off status..." );
}
 
void handleSetup() {
  Serial.println("handleSetup --> AP=192.168.4.1");
 
  WiFiManager wifiManager;
  wifiManager.resetSettings();
 
  server.send ( 200, "text/plain", "resetting wifi credentials, going into accesspoint mode, connect to it, and browse to 192,168.4.1 to config ssid details..." );
 
  delay(3000);
  ESP.reset();
  delay(5000);
}
 
void handleNotFound() {
  String message = "This is the WifiAlertLight, don't call us, we call a server for light on/off status...\n\nFile Not Found\n\n";
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
  Serial.println("handleNotFound; " + message);
}
 
// Main loop - the ESP keeps endlessly calling this after setup.
void loop() {
  // Check for OTA update's
  ArduinoOTA.handle();
 
  // Check for incoming web request
  server.handleClient();
 
  // The rest of the lines in this method: Poll the internet gateway server for command's to execute.
  // If you do NOT require this device to poll a server, you can remove the rest of the
  // lines in this method, because the lines above this comment are acting as a web server, receiving
  // API call's to turn on/off the lamp and/or LED. The only difficult bit is finding the IP address of
  // the flash light in that case ;-) I hope you have access to the DHCP server in your Wifi router in
  // that situation. Or you can use the mDNS broadcasted info. On ubuntu use this command: "avahi-browse -r --all"
 
 
  // Add mac address, in case our internet server servers more than one device ;-)
  String url = serverPollingUrl + WiFi.macAddress();
  Serial.println("Start reading data... " + url);
 
  // short LED flash, to indicate we are still alive and online.
  handlePulsePin(LED_PIN, 500);
 
  // On Wifi connection loss, just reboot to try to reconnect.
  // During reboot, we turn ON the LED, so you have an indication the device is not alive any more.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected? Reboot...");
    handleSetPin(LED_PIN, HIGH);
    delay(3000);
    ESP.reset();
    delay(5000);
  }
 
  HTTPClient http;
  http.begin(url);
  // User-Agent: Let's the server know who we are, and what our local IP address is. Handy for debugging...
  http.setUserAgent("ESP8266 WifiControlledAlertLight " + WiFi.macAddress() + " " + WiFi.localIP().toString());
  // Give up after 62 seconds (a bit longer than the server side time out).
  http.setTimeout(62000);
 
  int httpCode = http.GET();
  if (httpCode == 404) {
    Serial.println("Code 404, no data (yet)...");
 
    // if someone enabled the relay, let's disable it after the one minute time out.
    handleSetPin(RELAY_PIN, LOW);
  } else if (httpCode == 200) {
    String payload = http.getString();
    Serial.print("Got some data: ");
    Serial.println(payload);
 
    // Try converting input to number, if so, it's a pulse duration
    int pulseTime = payload.toInt();
    if (pulseTime == 0) {
      // not a number, set default to 2 seconds
      pulseTime = 2000;
    } else {
      // set command to pulse
      payload = "pulse";
    }
 
    if (payload == "on") {
      handleSetPin(RELAY_PIN, HIGH);
    } else if (payload == "off") {
      handleSetPin(RELAY_PIN, LOW);
    } else if (payload == "pulse") {
      handlePulsePin(RELAY_PIN, pulseTime);
    } else if (payload == "setup" || payload == "config") {
      handleSetup();
    } else if (payload == "reboot" || payload == "reset") {
      handleReboot();
    } else {
      // unrecognized command, flash the led a couple of times
      handlePulsePin(LED_PIN, 100);
      delay(100);
      handlePulsePin(LED_PIN, 100);
      delay(100);
      handlePulsePin(LED_PIN, 100);
      delay(100);
      handlePulsePin(LED_PIN, 100);
      delay(100);
      handlePulsePin(LED_PIN, 100);
      delay(100);
    }
  } else {
    Serial.print(httpCode);
    Serial.println(" Error...");
    handleSetPin(LED_PIN, HIGH);
    handleSetPin(RELAY_PIN, LOW);
  }
  http.end(); //Close connection
 
  Serial.println("Sleep 2 sec...");
  delay(2000);
}
