#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "LittleFS.h"
#include <time.h>
#include "ESP8266mDNS.h"

//External libraries
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <CSE7766.h>

#define BROKER_URL_LEN 20
#define SSID_LEN 32
#define PSWD_LEN 20
#define GW_ID_LEN 16
#define HOME_ID_LEN 16
#define BUFF_LEN 300
#define TOPIC_LEN 42

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
#define BTN_PIN 0
#define RELAY_PIN 12
#define LED_PIN 13

#define DOMAIN_RESOLVE (char*) "MQTT2GO.local"
#define REQ_SSID (char*) "MQTT2GO_Config"
#define HOME_FILES (char*) "./home.txt"
#define LOGIN_FILES (char*) "./login.txt"
#define CERT_FILE (char*) "./cert.crt"
#define WIFI_FILE (char*) "./wifi.txt"

#define CLIENT_ID (char*) "powr2"
#define ACTIVATION_CODE (char*) "456789"

#define ACTIVATION_TOPIC_OUT ACTIVATION_CODE "/activation/out" 
#define WIFI_TOPIC_OUT ACTIVATION_CODE "/wifi/out"
#define HOME_TOPIC_OUT CLIENT_ID "/home/out"

#define ACTIVATION_TOPIC_IN ACTIVATION_CODE "/activation/in" 
#define WIFI_TOPIC_IN ACTIVATION_CODE "/wifi/in"
#define HOME_TOPIC_IN CLIENT_ID "/home/in"

#define DEV_LEN 5
#define DEV_VOL 0
#define DEV_CUR 1
#define DEV_CONS 2
#define DEV_ENERG 3
#define DEV_POW 4

typedef struct info {
  char name[16];
  unsigned int direction : 1;
  char unit[3];
  char type[2];
} info;

struct info device[] = {{"voltage", 0, "V", "f"}, {"current", 0, "A", "f"}, {"consumption", 0, "W", "f"}, {"energy", 0, "Ws", "f"}, {"switch", 1, "", "s"}};

char mqttUser[20] = {0};
char mqttPswd[PSWD_LEN] = {0};
char wifiSSID[SSID_LEN] = {0};
char wifiPswd[PSWD_LEN] = {0};
char topic_buff[42] = {0};

char cert[64] = {0};
uint16_t port = 0;
char mqttServer[BROKER_URL_LEN] = {0};
char hostString[16] = {0};

StaticJsonDocument<BUFF_LEN + 100> doc;
char buffer[BUFF_LEN] = {0};
char home_id[HOME_ID_LEN] = {0};
char gateway_id[GW_ID_LEN] = {0};
uint8_t state = EMPTY_STATE;
unsigned long lastMsg = 0;
uint8_t power = 0;

WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long pressStart = 0;
boolean pressed = 0;

CSE7766 myCSE7766;

static const char *fingerprint PROGMEM = "7c 2b 4b ed 40 7f ec 7a 68 b1 e4 f9 26 6d f9 ef 88 c2 01 46";

void setPower(const char* command){
  if (strcmp(command, "off") == 0){
    digitalWrite(RELAY_PIN, LOW);
    power = 0;
  }
  if (strcmp(command, "on") == 0){
    digitalWrite(RELAY_PIN, HIGH);
    power = 1;
  }
}

void factoryReset(){
  Serial.println("Factory reset");
  digitalWrite(LED_PIN, LOW);
  pinMode(BTN_PIN, OUTPUT);
  delay(200);
  LittleFS.format();
  delay(1000);
  ESP.restart();
}

void createSimpleCommand(char* command_type, char *value){
  doc.clear();
  doc["timestamp"] = (uint32_t) time(nullptr);
  doc["type"] = command_type;
  doc["value"] = value;
}

void createHomeCommand(struct info device[]){
  doc.clear();
  doc["timestamp"] = (uint32_t) time(nullptr);
  doc["type"] = "home_prefix";
  JsonArray arr = doc.createNestedArray("value");
  for(int i = 0; i < DEV_LEN; i++){
    JsonObject obj = arr.createNestedObject();
    obj["name"] = device[i].name;
    obj["unit"] = device[i].unit;
    obj["type"] = device[i].type;
  }
}

void createReport(char* report_type, char* report_name, float value, char* unit){
  doc.clear();
  doc["priority_level"] = 2;
  doc["type"] = report_type;
  doc["timestamp"] = (uint32_t) time(nullptr);
  JsonObject val = doc.createNestedObject("report");
  val["unit"] = unit;
  val["value"] = value;
}

void getTopic(char* name, char* direction){
  memset(topic_buff, 0, TOPIC_LEN);
  sprintf(topic_buff, "%s/%s/%s/%s/%s", home_id, gateway_id, CLIENT_ID, name, direction);
}

void reportValues(){
  myCSE7766.handle();

  createReport((char*) "periodic_report", device[DEV_VOL].name, myCSE7766.getVoltage(), device[DEV_VOL].unit);
  serializeJson(doc, buffer);
  getTopic(device[DEV_VOL].name, (char*) "out");
  client.publish(topic_buff, buffer);

  createReport((char*) "periodic_report", device[DEV_CUR].name, myCSE7766.getCurrent(), device[DEV_CUR].unit);
  serializeJson(doc, buffer);
  getTopic(device[DEV_CUR].name, (char*) "out");
  client.publish(topic_buff, buffer);

  createReport((char*) "periodic_report", device[DEV_CONS].name, myCSE7766.getActivePower(), device[DEV_CONS].unit);
  serializeJson(doc, buffer);
  getTopic(device[DEV_CONS].name, (char*) "out");
  client.publish(topic_buff, buffer);

  createReport((char*) "periodic_report", device[DEV_ENERG].name, myCSE7766.getEnergy(), device[DEV_ENERG].unit);
  serializeJson(doc, buffer);
  getTopic(device[DEV_ENERG].name, (char*) "out");
  client.publish(topic_buff, buffer);
}

void commandResponse(int variable){
  doc["type"] = "command_response";
  doc["timestamp"] = (uint32_t) time(nullptr);
  serializeJson(doc, buffer);
  getTopic(device[variable].name, (char*) "out");
  client.publish(topic_buff, buffer);
}

void callback(char* topic, byte* payload, unsigned int length){
  payload[length] = 0;
  doc.clear();
  DeserializationError error = deserializeJson(doc, (char*) payload);
  if(!error){
      if (strstr(topic, device[DEV_POW].name)){
          setPower(doc["value"]);
          commandResponse(DEV_POW);
      }
      if (strstr(topic, "remove")){
        client.disconnect();
        factoryReset();
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
  Serial.println("Connected to mqtt");
  delay(100);
}

void storeValues(char* file, char *values[], uint8_t arr_size){
  File f = LittleFS.open(file, "w+");
  for (uint8_t i = 0; i < arr_size; i++)
  {
    f.print(values[i]);
    f.print('\n');
  }
  f.close();
}

void loadValues(char* file, char* values[], uint8_t arr_size){
  uint16_t l;
  File f = LittleFS.open(file, "r");
  for(uint8_t i = 0; i < arr_size; i++){
    buffer[0] = '\0';
    l = f.readBytesUntil('\n', buffer, BUFF_LEN);
    buffer[l] = '\0';
    strcpy(values[i], buffer);
  }
  f.close();
}

void setupCallback(char* topic, byte* payload, unsigned int length){
  payload[length] = 0;
  doc.clear();
  char* values[2] = {};
  DeserializationError error = deserializeJson(doc, (char*)payload);
  if (!error){
    if (strcmp(topic, ACTIVATION_TOPIC_IN) == 0 && doc["type"] == "mqtt_credentials"){
      strcpy(mqttUser, doc["value"]["user"]);
      strcpy(mqttPswd, doc["value"]["password"]);
      strcpy(cert, doc["value"]["cert"]);
      values[0] = mqttUser;
      values[1] = mqttPswd;
      //Store MQTT credentials
      storeValues(LOGIN_FILES, values, 2);
      values[0] = cert;
      //Store Cert
      storeValues(CERT_FILE, values, 1);
      state = WIFI_STATE;
      lastMsg = 0;
    }
    else if (strcmp(topic, WIFI_TOPIC_IN) == 0 && doc["type"] == "wifi_credentials"){
      strcpy(wifiSSID, doc["value"]["SSID"]);
      strcpy(wifiPswd, doc["value"]["password"]);
      values[0] = wifiSSID;
      values[1] = wifiPswd;
      //Store WiFi credentials
      storeValues(WIFI_FILE, values, 2);
      state = JOINT_STATE;
      lastMsg = 0;
    }
    else if (strcmp(topic, HOME_TOPIC_IN) == 0 && doc["type"] == "home_prefix"){
      client.unsubscribe(HOME_TOPIC_OUT);
      client.disconnect();
      strcpy(home_id, doc["value"]["home_id"]);
      strcpy(gateway_id, doc["value"]["gateway_id"]);
      values[0] = home_id;
      values[1] = gateway_id;
      storeValues(HOME_FILES, values, 2);
      pinMode(BTN_PIN, OUTPUT);
      delay(200);
      ESP.restart();
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
  MDNS.begin(hostString);
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
  Serial.println("Looking for WIFI");
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
  Serial.println("WIFI Found");
}

void ICACHE_RAM_ATTR pressHandler(){
    if (digitalRead(BTN_PIN) == LOW){
      pressStart = millis();
      pressed = 1;
    }
    if (digitalRead(BTN_PIN) == HIGH && millis() - pressStart > 5000)
    {
      factoryReset();
    } 
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), pressHandler, CHANGE);
  LittleFS.begin();
  delay(200);

  char* values[2] = {};
  boolean configured = LittleFS.exists(HOME_FILES);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  if (configured){
    values[0] = wifiSSID;
    values[1] = wifiPswd;
    loadValues(WIFI_FILE, values, 2);
    Serial.println("Wifi");
    Serial.println(wifiSSID);
    Serial.println(wifiPswd);
    WiFi.begin(wifiSSID, wifiPswd);
    myCSE7766.setRX(1);
    myCSE7766.begin();
  }else{
    findWifi();
    WiFi.begin(wifiSSID);
  }
  
  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
  }

  delay(1000);
  for (uint8_t i = 0; i < 3; i++)
  {
    getTime();
    delay(500);
  }

  findIP();
 
  randomSeed(micros());
  delay(200);

  client.setServer(mqttServer, port);
  client.setBufferSize(BUFF_LEN);
  
  if (configured == false){
      LittleFS.format();
      espClient.setFingerprint(fingerprint);
      client.setCallback(setupCallback);
      reconnect(true);
      state = START_STATE;
  } else {
      //Load MQTT credentials
      values[0] = mqttUser;
      values[1] = mqttPswd;
      loadValues(LOGIN_FILES, values, 2);
      //Load WiFi credentials
      values[0] = home_id;
      values[1] = gateway_id;
      loadValues(HOME_FILES, values,2);
      //Load certificate
      values[0] = cert;
      loadValues(CERT_FILE, values, 1);                                                                                                         
      espClient.setFingerprint(cert);
      state = INITIALIZE_STATE;
      reconnect(false);
      client.setCallback(callback);
  }
}

void loop() {
    switch (state)
    {
    case START_STATE:
      client.subscribe(ACTIVATION_TOPIC_IN);
      state = START_STATE_I;
      break;
    case START_STATE_I:
      if (millis() - lastMsg > 10000){
        createSimpleCommand((char *) "mqtt_credentials", (char *) "GET_CREDENTIALS");
        serializeJson(doc, buffer);
        client.publish(ACTIVATION_TOPIC_OUT, buffer);
        lastMsg = millis();
      }
      break;
    case WIFI_STATE:
      client.subscribe(WIFI_TOPIC_IN);
      state = WIFI_STATE_I;
      break;
    case WIFI_STATE_I:
      if (millis() - lastMsg > 10000){
        createSimpleCommand((char *) "wifi_credentials", (char *) "GET_WIFI_CREDENTIALS");
        serializeJson(doc, buffer);
        client.publish(WIFI_TOPIC_OUT, buffer);
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
      client.subscribe(HOME_TOPIC_IN);
      state = TOPIC_STATE;
      break;
    case TOPIC_STATE:
      if (millis() - lastMsg > 10000){
        createHomeCommand(device);
        serializeJson(doc, buffer);
        Serial.println(buffer);
        client.publish(HOME_TOPIC_OUT, buffer);
        lastMsg = millis();
      }
      break;
    case INITIALIZE_STATE:
      for (uint8_t i = 0; i < DEV_LEN; i++)
      {
        if(device[i].direction == 1){
          getTopic(device[i].name, (char *) "in");
          client.subscribe(topic_buff);
        }
      }
      getTopic((char *) "remove", (char *) "in");
      client.subscribe(topic_buff);
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