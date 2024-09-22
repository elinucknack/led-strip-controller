/* IMPORTS */

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Regexp.h>
#include <time.h>

/* CONFIGURATION */

// Static IP configuration

const IPAddress ip(192, 168, 1, 3);
const IPAddress gateway(192, 168, 1, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress primaryDns(192, 168, 1, 1);
const IPAddress secondaryDns(192, 168, 1, 2);

// WiFi client configuration

const String wifiSsid = "ssid";
const String wifiPassword = "password";

// MQTT client configuration

const String mqttBroker = "mqtt.mysite.com";
const bool mqttAnonymous = false;
const String mqttUsername = "mqttuser";
const String mqttPassword = "mqttpassword";
const int mqttPort = 1883;
const String mqttClientId = "clientid";
const String mqttTopic = "mysite/device/devicename";
const bool mqttUseSsl = false;
const String mqttCaCert = R"EOF(
-----BEGIN CERTIFICATE-----
cacert
-----END CERTIFICATE-----
)EOF";
const String mqttClientCert = R"EOF(
-----BEGIN CERTIFICATE-----
clientcert
-----END CERTIFICATE-----
)EOF";
const String mqttClientKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
privatekey
-----END RSA PRIVATE KEY-----
)EOF";

// NTP client configuration

const IPAddress ntpServer(217, 31, 202, 100);

// LED pin configuration

const int redPin = 14;
const int greenPin = 12;
const int bluePin = 16;

/* PROGRAM */

int wiFiConnectionAttemptTimeout = 30000;
int wiFiConnectionAttemptStart = 0;

int ntpServerConnectionAttemptTimeout = 30000;
int ntpServerConnectionAttemptStart = 0;

WiFiClient wiFiClient;
BearSSL::WiFiClientSecure wiFiClientSecure;
PubSubClient mqttClient(mqttUseSsl ? wiFiClientSecure : wiFiClient);
int mqttBrokerConnectionAttemptTimeout = 60000;
int mqttBrokerConnectionAttemptStart = 0;
int statePublishInterval = 15000;
int stateLastPublished = 0;

bool ledStripOn = false;
int ledStripBrightness = 100;
int ledStripRed = 255;
int ledStripGreen = 255;
int ledStripBlue = 255;

void setup();

void loop();

void configureStaticIp();

bool connectedToWiFi();

void setWiFiConnectionAttemptStart();

bool wiFiConnectionAttemptTimedOut();

String getLocalIp();

void connectToWiFi();

void reconnectToWiFi();

bool connectedToNtpServer();

void setNtpServerConnectionAttemptStart();

bool ntpServerConnectionAttemptTimedOut();

void connectToNtpServer();

void configureSsl();

bool connectMqttClient();

bool connectedToMqttBroker();

void setLedStripOn(String payload);

void setLedStripBrightness(String payload);

void setLedStripColor(String payload);

void mqttCallback(char *topic, byte *payload, unsigned int length);

void setMqttBrokerConnectionAttemptStart();

bool mqttBrokerConnectionAttemptTimedOut();

void subscribeMqttClient();

int getMqttClientState();

void printSslError();

void connectToMqttBroker();

void reconnectToMqttBroker();

String getUptime();

String getState();

bool timeToPublishState();

void setStateLastPublished();

void publishState();

void setUpLedStrip();

void setup() {
  delay(5000);
  Serial.begin(115200);
  configureStaticIp();
  connectToWiFi();
  connectToNtpServer();
  connectToMqttBroker();
  publishState();
}

void loop() {
  if (!connectedToWiFi()) {
    reconnectToWiFi();
  }
  if (!connectedToMqttBroker()) {
    reconnectToMqttBroker();
    publishState();
  }
  if (timeToPublishState()) {
    publishState();
  }
  mqttClient.loop();
  setUpLedStrip();
  delay(100);
}

void configureStaticIp() {
  Serial.println("Configuring static IP...");
  if (WiFi.config(ip, gateway, subnet, primaryDns, secondaryDns)) {
    Serial.println("Static IP configuration succeeded");
  } else {
    Serial.println("Static IP configuration failed");
  }
}

bool connectedToWiFi() {
  return WiFi.status() == WL_CONNECTED;
}

void setWiFiConnectionAttemptStart() {
  wiFiConnectionAttemptStart = millis();
}

bool wiFiConnectionAttemptTimedOut() {
  return millis() - wiFiConnectionAttemptStart >= wiFiConnectionAttemptTimeout;
}

String getLocalIp() {
  return WiFi.localIP().toString();
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(wifiSsid, wifiPassword);
  setWiFiConnectionAttemptStart();
  while (!connectedToWiFi()) {
    if (wiFiConnectionAttemptTimedOut()) {
      Serial.print("\nConnection attempt to WiFi timed out. Device restart...");
      ESP.restart();
    }
    Serial.print(".");
    delay(1000);
  }
  WiFi.setAutoReconnect(true);
  Serial.print("\nConnected to WiFi. Local IP address: ");
  Serial.println(getLocalIp());
}

void reconnectToWiFi() {
  Serial.println("Connection to WiFi lost, reconnecting to WiFi");
  setWiFiConnectionAttemptStart();
  while (!connectedToWiFi()) {
    if (wiFiConnectionAttemptTimedOut()) {
      Serial.print("\nReconnection attempt to WiFi timed out. Device restart...");
      ESP.restart();
    }
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\nReconnected to WiFi. Local IP address: ");
  Serial.println(getLocalIp());
}

bool connectedToNtpServer() {
  return (bool) time(nullptr);
}

void setNtpServerConnectionAttemptStart() {
  ntpServerConnectionAttemptStart = millis();
}

bool ntpServerConnectionAttemptTimedOut() {
  return millis() - ntpServerConnectionAttemptStart >= ntpServerConnectionAttemptTimeout;
}

void connectToNtpServer() {
  Serial.print("Connecting to NTP server");
  configTime(0, 0, ntpServer.toString());
  setNtpServerConnectionAttemptStart();
  while (!connectedToNtpServer()) {
    if (wiFiConnectionAttemptTimedOut()) {
      Serial.print("\nConnection attempt to NTP server timed out. Device restart...");
      ESP.restart();
    }
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nConnected to NTP server. NTP server IP address: ");
  Serial.println(ntpServer.toString());
}

void configureSsl() {
  BearSSL::X509List *caCertList = new BearSSL::X509List(mqttCaCert.c_str());
  BearSSL::X509List *clientCertList = new BearSSL::X509List(mqttClientCert.c_str());
  BearSSL::PrivateKey *clientKey = new BearSSL::PrivateKey(mqttClientKey.c_str());
  wiFiClientSecure.setTrustAnchors(caCertList);
  wiFiClientSecure.setClientRSACert(clientCertList, clientKey);
}

void setLedStripOn(String payload) {
  JsonDocument document;
  DeserializationError error = deserializeJson(document, payload);
  if (error) {
    Serial.print("LED strip on setting failed: Invalid JSON payload \"");
    Serial.print(payload);
    Serial.print("\": ");
    Serial.println(error.c_str());
  } else {
    bool on = document["value"];
    ledStripOn = on;
    Serial.print("LED strip on set to \"");
    Serial.print(on);
    Serial.println("\"");
  }
}

void setLedStripBrightness(String payload) {
  JsonDocument document;
  DeserializationError error = deserializeJson(document, payload);
  if (error) {
    Serial.print("LED strip brightness setting failed: Invalid JSON payload \"");
    Serial.print(payload);
    Serial.print("\": ");
    Serial.println(error.c_str());
  } else {
    int brightness = document["value"];
    if (brightness < 0 || brightness > 100) {
      Serial.print("LED strip brightness setting failed: Invalid brightness value \"");
      Serial.print(brightness);
      Serial.println("\": Must be from 0 to 100.");
    } else {
      ledStripBrightness = brightness;
      Serial.print("LED strip brightness set to \"");
      Serial.print(brightness);
      Serial.println("\"");
    }
  }
}

void setLedStripColor(String payload) {
  JsonDocument document;
  DeserializationError error = deserializeJson(document, payload);
  if (error) {
    Serial.print("LED strip color setting failed: Invalid JSON payload \"");
    Serial.print(payload);
    Serial.print("\": ");
    Serial.println(error.c_str());
  } else {
    String color = document["value"];
    MatchState ms;
    ms.Target((char *)color.c_str());
    if (ms.Match("^#%x%x%x$") == REGEXP_MATCHED) {
      String red = color.substring(1, 2);
      red = red + red;
      String green = color.substring(2, 3);
      green = green + green;
      String blue = color.substring(3, 4);
      blue = blue + blue;
      ledStripRed = strtoul(red.c_str(), NULL, 16);
      ledStripGreen = strtoul(green.c_str(), NULL, 16);
      ledStripBlue = strtoul(blue.c_str(), NULL, 16);
      Serial.print("LED strip color set to \"");
      Serial.print(color);
      Serial.println("\"");
    } else if (ms.Match("^#%x%x%x%x%x%x$") == REGEXP_MATCHED) {
      String red = color.substring(1, 3);
      String green = color.substring(3, 5);
      String blue = color.substring(5, 7);
      ledStripRed = strtoul(red.c_str(), NULL, 16);
      ledStripGreen = strtoul(green.c_str(), NULL, 16);
      ledStripBlue = strtoul(blue.c_str(), NULL, 16);
      Serial.print("LED strip color set to \"");
      Serial.print(color);
      Serial.println("\"");
    } else {
      Serial.print("LED strip color setting failed: Invalid color value \"");
      Serial.print(color);
      Serial.println("\": Must be a hex color value (#xxx/#xxxxxx).");
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String strTopic = String(topic);
  String strPayload = String((char *)payload).substring(0, length);
  if (strTopic.equals(mqttTopic + "/on")) {
    setLedStripOn(strPayload);
  } else if (strTopic.equals(mqttTopic + "/brightness")) {
    setLedStripBrightness(strPayload);
  } else if (strTopic.equals(mqttTopic + "/color")) {
    setLedStripColor(strPayload);
  } else {
    Serial.print("Topic \"");
    Serial.print(topic);
    Serial.println("\" ignored");
  }
}

bool connectedToMqttBroker() {
  return mqttClient.connected();
}

bool connectMqttClient() {
  const char *cStrMqttClientId = mqttClientId.c_str();
  if (mqttAnonymous) {
    return mqttClient.connect(cStrMqttClientId);
  }
  return mqttClient.connect(cStrMqttClientId, mqttUsername.c_str(), mqttPassword.c_str());
}

void setMqttBrokerConnectionAttemptStart() {
  mqttBrokerConnectionAttemptStart = millis();
}

bool mqttBrokerConnectionAttemptTimedOut() {
  return millis() - mqttBrokerConnectionAttemptStart >= mqttBrokerConnectionAttemptTimeout;
}

void subscribeMqttClient() {
  mqttClient.subscribe((mqttTopic + "/#").c_str());
}

int getMqttClientState() {
  return mqttClient.state();
}

void printSslError() {
  char errorBuffer[128];
  wiFiClientSecure.getLastSSLError(errorBuffer, sizeof(errorBuffer));
  Serial.print("SSL error: ");
  Serial.println(errorBuffer);
}

void connectToMqttBroker() {
  if (mqttUseSsl) {
    configureSsl();
  }
  mqttClient.setServer(mqttBroker.c_str(), mqttPort);
  mqttClient.setBufferSize(512);
  mqttClient.setCallback(mqttCallback);
  setMqttBrokerConnectionAttemptStart();
  while (!connectedToMqttBroker()) {
    Serial.println("Connecting to MQTT broker");
    if (connectMqttClient()) {
      Serial.print("Connected to MQTT broker. MQTT broker host: ");
      Serial.print(mqttBroker);
      Serial.print(":");
      Serial.println(mqttPort);
      subscribeMqttClient();
    } else {
      if (mqttBrokerConnectionAttemptTimedOut()) {
        Serial.print("\nConnection attempt to MQTT broker timed out. Device restart...");
        ESP.restart();
      }
      Serial.print("Failed to connect to MQTT broker. Return code: ");
      Serial.println(getMqttClientState());
      if (mqttUseSsl) {
        printSslError();
      }
      delay(5000);
    }
  }
}

void reconnectToMqttBroker() {
  Serial.println("Connection to MQTT broker lost");
  setMqttBrokerConnectionAttemptStart();
  while (!connectedToMqttBroker()) {
    Serial.println("Reconnecting to MQTT broker");
    if (connectMqttClient()) {
      Serial.print("Reconnected to MQTT broker. MQTT broker host: ");
      Serial.print(mqttBroker);
      Serial.print(":");
      Serial.println(mqttPort);
      subscribeMqttClient();
    } else {
      if (mqttBrokerConnectionAttemptTimedOut()) {
        Serial.print("\nConnection attempt to MQTT broker timed out. Device restart...");
        ESP.restart();
      }
      Serial.print("Failed to reconnect to MQTT broker. Return code: ");
      Serial.println(getMqttClientState());
      if (mqttUseSsl) {
        printSslError();
      }
      delay(5000);
    }
  }
}

String getUptime() {
  int uptime = millis() / 1000;
  int seconds = uptime % 60;
  String strSeconds = String(seconds < 10 ? "0" : "") + seconds;
  uptime /= 60;
  int minutes = uptime % 60;
  String strMinutes = String(minutes < 10 ? "0" : "") + minutes;
  uptime /= 60;
  int hours = uptime % 24;
  String strHours = String(hours < 10 ? "0" : "") + hours;
  uptime /= 24;
  int days = uptime % 7;
  String strDays = days == 0 ? String("") : String(days) + "d";
  uptime /= 7;
  int weeks = uptime % 7;
  String strWeeks = weeks == 0 ? String("") : String(weeks) + "w";
  return strWeeks + strDays + strHours + ":" + strMinutes + ":" + strSeconds;
}

String getState() {
  String coreVersion = ESP.getCoreVersion();
  int cpuFrequency = ESP.getCpuFreqMHz();
  int flashChipSize = ESP.getFlashChipSize();
  int flashChipFrequency = ESP.getFlashChipSpeed();
  int programSize = ESP.getSketchSize();
  int freeStackSize = ESP.getFreeContStack();
  int freeHeapSize = ESP.getFreeHeap();
  int maxFreeHeapBlockSize = ESP.getMaxFreeBlockSize();
  int heapFragmentation = ESP.getHeapFragmentation();
  String uptime = getUptime();
  int timestamp = time(nullptr);
  return String("{\n") +
    "  \"coreVersion\": \"" + coreVersion + "\",\n" +
    "  \"cpuFrequency\": " + cpuFrequency + ",\n" +
    "  \"flashChipSize\": " + flashChipSize + ",\n" +
    "  \"flashChipFrequency\": " + flashChipFrequency + ",\n" +
    "  \"programSize\": " + programSize + ",\n" +
    "  \"freeStackSize\": " + freeStackSize + ",\n" +
    "  \"freeHeapSize\": " + freeHeapSize + ",\n" +
    "  \"maxFreeHeapBlockSize\": " + maxFreeHeapBlockSize + ",\n" +
    "  \"heapFragmentation\": " + heapFragmentation + ",\n" +
    "  \"uptime\": \"" + uptime + "\",\n" +
    "  \"timestamp\": " + timestamp + "\n" +
    "}";
}

bool timeToPublishState() {
  return millis() - stateLastPublished >= statePublishInterval;
}

void setStateLastPublished() {
  stateLastPublished = millis();
}

void publishState() {
  mqttClient.publish((mqttTopic + "/state").c_str(), getState().c_str(), true);
  setStateLastPublished();
}

void setUpLedStrip() {
  analogWrite(redPin, ledStripOn ? ledStripRed * ledStripBrightness / 100 : 0);
  analogWrite(greenPin, ledStripOn ? ledStripGreen * ledStripBrightness / 100 : 0);
  analogWrite(bluePin, ledStripOn ? ledStripBlue * ledStripBrightness / 100 : 0);
}
