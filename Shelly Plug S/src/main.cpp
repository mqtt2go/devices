#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include "FS.h"
#include <time.h>
#include "HLW8012.h"
#include "ESP8266mDNS.h" 

#define MAX_TOPICS 7
#define MAX_TOPIC_LEN 42
#define BROKER_URL_LEN 20
#define SSID_LEN 32
#define PSWD_LEN 20
#define BUFF_LEN 384

#define EMPTY_STATE 0
#define START_STATE 1
#define START_STATE_I 2
#define WIFI_STATE 3
#define WIFI_STATE_I 4
#define JOINT_STATE 5
#define TOPIC_STATE 6
#define TOPIC_STATE_I 7
#define INITIALIZE_STATE 8
#define INITIALIZED_STATE 9

#define POWER_PIN 15
#define CF_PIN 5
#define CF1_PIN 14
#define SEL_PIN 12
#define BTN_PIN 13
#define LED_PIN 0
#define LED2_PIN 2

#define CURRENT_MODE LOW
#define CURRENT_RESISTOR 0.001
#define VOLTAGE_RESISTOR_UPSTREAM       ( 3 * 680000 ) // Real: 2280k
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 1000 ) // Real 1.009k

#define HJL01_CURRENT_RATIO             12000.5
#define HJL01_VOLTAGE_RATIO             301000.5
#define HJL01_POWER_RATIO               3414290

#define DOMAIN_RESOLVE (char*) "MQTT2GO.local"
#define REQ_SSID (char*) "MQTT2GO_Config"
#define TOPICS_FILES (char*) "/topics.txt"
#define LOGIN_FILES (char*) "/login.txt"
#define CERT_FILE (char*) "/cert.crt"
#define WIFI_FILE (char*) "/wifi.txt"

#define CLIENT_ID (char*) "SPLUG"
#define ACTIVATION_CODE (char*) "234567"

#define ACTIVATION_TOPIC ACTIVATION_CODE "/activation" 
#define WIFI_TOPIC ACTIVATION_CODE "/wifi" 
#define TOPICS_TOPIC CLIENT_ID "/topics" 


char mqttUser[20] = {0};
char mqttPswd[20] = {0};
char wifiSSID[SSID_LEN] = {0};
char wifiPswd[PSWD_LEN] = {0};

char cert[64] = {0};
uint16_t port = 0;
char mqttServer[BROKER_URL_LEN] = {0};
char hostString[16] = {0};
unsigned long lastMsg = 0;

StaticJsonDocument<BUFF_LEN> doc;
char buffer[BUFF_LEN];
char topics[MAX_TOPICS][MAX_TOPIC_LEN];
uint8_t topicsNum = 0;
uint8_t state = EMPTY_STATE;

HLW8012 hlw8012;
WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long pressStart = 0;
boolean pressed = 0;
boolean wifiConfigured = false;

static const char *fingerprint PROGMEM = "63 7B 21 A6 28 2E 5F 2C 46 C0 C7 E8 5D E2 B4 70 2F 74 4B 53";

void setPower(const char* command){
  if (strcmp(command, "off") == 0){
      digitalWrite(POWER_PIN, LOW);
  }
  if (strcmp(command, "on") == 0){
    digitalWrite(POWER_PIN, HIGH);
  }
}

void createCommandHeader(char* command_t, char *value){
  doc.clear();
  doc["type"] = "command";
  doc["timestamp"] = (uint32_t) time(nullptr);
  doc["command_type"] = command_t;
  doc["value"] = value;
}

void reportValues(){
  doc.clear();
  doc["type"] = "report";
  doc["priority_level"] = 1;
  doc["report_type"] = "periodic_report";
  doc["timestamp"] = (uint32_t) time(nullptr);

  for (uint8_t i = 0; i < topicsNum; i++)
  {
    if(strstr(topics[i], "/consumption") != NULL){
      doc["report_name"] = "consumption";
      doc["report"] = hlw8012.getActivePower();
      serializeJson(doc, buffer);
      client.publish(topics[i], buffer);
    }

    if(strstr(topics[i], "/voltage") != NULL){
      doc["report_name"] = "voltage";
      doc["report"] = hlw8012.getVoltage();
      serializeJson(doc, buffer);
      client.publish(topics[i], buffer);
    }

    if(strstr(topics[i], "/current") != NULL){
      doc["report_name"] = "current";
      doc["report"] = hlw8012.getCurrent();
      serializeJson(doc, buffer);
      client.publish(topics[i], buffer);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length){
  payload[length] = 0;
  doc.clear();
  DeserializationError error = deserializeJson(doc, (char *) payload);
  if(!error){
    if (doc["type"] == "command"){
      if(strstr(topic, "/power") != NULL && doc["command_type"] == "set"){
        setPower(doc["value"]);
      }
      if (strstr(topic, "/remove") != NULL && doc["command_type"] == "remove"){
        client.disconnect();
        SPIFFS.format();
        delay(1000);
        ESP.restart();
      }
    }
  }
}

void reconnect(bool isAnonyme){
  while (!client.connected())
  {
    if (isAnonyme){
      client.connect(CLIENT_ID);
    } else {
      client.connect(CLIENT_ID, mqttUser, mqttPswd);
    }
    if (!client.connected()){
        delay(5000);
    }
  }
}

void loadTopics(){
  File f = SPIFFS.open(TOPICS_FILES, "r");
  uint8_t i = 0;
  while (f.available())
  {
    buffer[0] = '\0';
    uint16_t l = f.readBytesUntil('\n', buffer, sizeof(buffer));
    buffer[l] = '\0';
    strcpy(topics[i], buffer);
    i++;
  }
  topicsNum = i;
  f.close();
}

void storeTopics(){
 File f = SPIFFS.open(TOPICS_FILES, "w+");
  for (uint8_t i = 0; i < topicsNum; i++)
  {
    f.print(topics[i]);
    f.print('\n');
  }
  f.close();
}

void loadCredentials(char* name, char* password, char* file){
  File f = SPIFFS.open(file, "r");
  buffer[0] = '\0';
  uint16_t l = f.readBytesUntil('\n', buffer, sizeof(buffer));
  buffer[l] = '\0';
  strcpy(name, buffer);
  buffer[0] = '\0';
  l = f.readBytesUntil('\n', buffer, sizeof(buffer));
  buffer[l] = '\0';
  strcpy(password, buffer);
  f.close();
}

void storeCredentials(char* name, char* password, char* file){
  File f = SPIFFS.open(file, "w+");
  f.print(name);
  f.print('\n');
  f.print(password);
  f.print('\n');
  f.close();
}

void storeCert(){
  File f = SPIFFS.open(CERT_FILE, "w+");
  f.print(cert);
  f.print('\n');
  f.close();
}

void loadCert(){
  File f = SPIFFS.open(CERT_FILE, "r");
  uint16_t l = f.readBytesUntil('\n', buffer, sizeof(buffer));
  buffer[l] = '\0';
  strcpy(cert, buffer);
  f.close();
}

void firstCallback(char* topic, byte* payload, unsigned int length){
  if (strcmp(topic, ACTIVATION_TOPIC) == 0 || strcmp(topic, WIFI_TOPIC) == 0 || strcmp(topic, TOPICS_TOPIC) == 0){
    payload[length] = 0;
    doc.clear();
    DeserializationError error = deserializeJson(doc, (char*)payload);
    if (!error){
      if(strcmp(doc["type"], "report") == 0 && doc["report_name"] == "mqtt_credentials"){
        strcpy(mqttUser, doc["value"]["user"]);
        strcpy(mqttPswd, doc["value"]["password"]);
        strcpy(cert, doc["value"]["cert"]);
        storeCredentials(mqttUser, mqttPswd, LOGIN_FILES);
        storeCert();
        if (wifiConfigured != true){
            state = WIFI_STATE;
        } else {
          state = JOINT_STATE;
        }
        lastMsg = 0;
      }
      else if(doc["type"] == "report" && doc["report_name"] == "wifi_credentials"){
        strcpy(wifiSSID, doc["value"]["SSID"]);
        strcpy(wifiPswd, doc["value"]["password"]);
        storeCredentials(wifiSSID, wifiPswd, WIFI_FILE);
        state = JOINT_STATE;
        lastMsg = 0;
      }
      else if(doc["type"] == "report" && doc["report_name"] == "topics"){
        client.unsubscribe(TOPICS_TOPIC);
        client.setCallback(callback);
        uint8_t i = 0;
        JsonArray arr = doc["value"].as<JsonArray>();
        for (JsonVariant value : arr){
          strcpy(topics[i], value.as<char*>());
          i++;
        }
        topicsNum = i;
        storeTopics();
        client.disconnect();
        ESP.restart();
      }
    }
  }
}

void getTime(){
  configTime(8 * 3600, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 1000) {
    delay(500);
    now = time(nullptr);
  }
}

void findIP(){
  while (port == 0){
    int n = MDNS.queryService("mqtt", "tcp");
    for (uint8_t i = 0; i < n; i++){
      MDNS.hostname(i).toCharArray(buffer, BROKER_URL_LEN);
      if (strcmp(buffer, DOMAIN_RESOLVE) == 0){
        MDNS.IP(i).toString().toCharArray(buffer, BROKER_URL_LEN);
        strcpy(mqttServer, buffer);
        port = MDNS.port(i);
      }
    }
  }
  Serial.println(mqttServer);
}

void findWifi(){
  while (strlen(wifiSSID) == 0)
  {
    uint8_t wifiNum = WiFi.scanNetworks();
    for (uint8_t i = 0; i < wifiNum; i++){
      WiFi.SSID(i).toCharArray(buffer, SSID_LEN);
      if(strcmp(buffer, REQ_SSID) == 0){
        strcpy(wifiSSID, buffer);
        break;
      }
    }
  }
}

bool connectWPS(){
  bool wpsConnected = false;
  digitalWrite(LED2_PIN, LOW);
  for (uint8_t i = 0; i < 3; i++){
      wpsConnected = WiFi.beginWPSConfig();
      WiFi.SSID().toCharArray(buffer, SSID_LEN);
      if(wpsConnected == true && strlen(buffer) > 0){
        strcpy(wifiSSID, buffer);
        WiFi.psk().toCharArray(buffer, PSWD_LEN);
        strcpy(wifiPswd, buffer);
        storeCredentials(wifiSSID, wifiPswd, WIFI_FILE);
        Serial.println(wifiSSID);
        Serial.println(wifiPswd);
        wifiConfigured = true;
        break;
      } else {
        wpsConnected = false;
      }
      delay(5000);
  }
  digitalWrite(LED2_PIN, HIGH);
  return wpsConnected;
}

void ICACHE_RAM_ATTR pressHandler(){
    if (digitalRead(BTN_PIN) == LOW){
      pressStart = millis();
      pressed = 1;
    } else {
        if (millis() - pressStart > 10000 && pressed == 1){
          Serial.println("Factory reset");
          digitalWrite(POWER_PIN, LOW);
          digitalWrite(LED_PIN, LOW);
          SPIFFS.format();
          ESP.restart();
        }
    }
}

void ICACHE_RAM_ATTR hlw8012_cf1_interrupt() {
    hlw8012.cf1_interrupt();
}
void ICACHE_RAM_ATTR hlw8012_cf_interrupt() {
    hlw8012.cf_interrupt();
}

void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(POWER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(LED2_PIN, HIGH);
  Serial.begin(9600);
  delay(100);
  SPIFFS.begin();
  

  boolean configured = SPIFFS.exists(TOPICS_FILES);
  attachInterrupt(BTN_PIN, pressHandler, CHANGE);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  if (configured){
    loadCredentials(wifiSSID, wifiPswd, WIFI_FILE);
    WiFi.begin(wifiSSID, wifiPswd);
  } else
  {
    if (connectWPS() == false){
      findWifi();
      WiFi.begin(wifiSSID);
    }
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }
  delay(1000);
  for (uint8_t i = 0; i < 3; i++)
  {
    getTime();
    delay(500);
  }

  MDNS.begin(hostString);

  findIP();

  randomSeed(micros());
  delay(200);
  
  client.setServer(mqttServer, port);

  if (configured == false){
    SPIFFS.format();
    espClient.setFingerprint(fingerprint);
    client.setCallback(firstCallback);
    reconnect(true);
    state = START_STATE;
  } else {
    loadCredentials(mqttUser, mqttPswd, LOGIN_FILES);
    loadTopics();
    loadCert();
    espClient.setFingerprint(cert);
    state = INITIALIZE_STATE;
    reconnect(false);
    client.setCallback(callback);
    delay(500);

    hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, true);
    hlw8012.setCurrentMultiplier(HJL01_CURRENT_RATIO);
    hlw8012.setVoltageMultiplier(HJL01_VOLTAGE_RATIO);
    hlw8012.setPowerMultiplier(HJL01_POWER_RATIO);
    attachInterrupt(digitalPinToInterrupt(CF_PIN), hlw8012_cf_interrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(CF1_PIN), hlw8012_cf1_interrupt, CHANGE);
  }
}

void loop() {
  switch (state)
    {
    case START_STATE:
      client.subscribe(ACTIVATION_TOPIC);
      state = START_STATE_I;
      break;
    case START_STATE_I:
      if (millis() - lastMsg > 10000){
        createCommandHeader((char *) "mqtt_credentials", (char *) "GET_CREDENTIALS");
        serializeJson(doc, buffer);
        client.publish(ACTIVATION_TOPIC, buffer);
        lastMsg = millis();
      }
      break;
    case WIFI_STATE:
      client.subscribe(WIFI_TOPIC);
      state = WIFI_STATE_I;
      break;
    case WIFI_STATE_I:
      if (millis() - lastMsg > 10000){
        createCommandHeader((char *) "wifi_credentials", (char *) "GET_WIFI_CREDENTIALS");
        serializeJson(doc, buffer);
        client.publish(WIFI_TOPIC, buffer);
        lastMsg = millis();
      }
      break;
    case JOINT_STATE:
      client.disconnect();
      WiFi.disconnect();
      WiFi.begin(wifiSSID, wifiPswd);
      while (WiFi.status() != WL_CONNECTED){
        delay(1000);
      }
      reconnect(false);
      client.subscribe(TOPICS_TOPIC);
      state = TOPIC_STATE;
      break;
    case TOPIC_STATE:
      if (millis() - lastMsg > 10000){
        createCommandHeader((char *) "topics", (char *) "GET_DEVICE_TOPICS");
        serializeJson(doc, buffer);
        client.publish(TOPICS_TOPIC, buffer);
        lastMsg = millis();
      }
      break;
    case INITIALIZE_STATE:
      for (uint8_t i = 0; i < topicsNum; i++)
      {
        client.subscribe(topics[i]);
      }
      state = INITIALIZED_STATE;
      break;
    case INITIALIZED_STATE:
      if (millis() - lastMsg > 10000){
          reportValues();
          lastMsg = millis();
      }
    break;
    default:
      break;
    }
    client.loop();
}