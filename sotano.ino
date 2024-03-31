#include "WIFIconfig.h"
#include "MQTTconfig.h"
#include "Portal.h"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Timer.h>
#include <PZEM004Tv30.h>

// CONSTANTS:
const boolean START = true;
const boolean RESET = false;
char *DOOR_CLOSED = "cerrado";
char *DOOR_OPENED = "abierto";
char *DOOR_AJAR = "intermedio";
char *LIMIT_SWITCHES_ERROR = "Error finales carrera";
char *OPEN = "abrir";
char *CLOSE = "cerrar";
char *STOP = "parar";

// TIMERS:
TON *tPublishInfoDoor;
TON *tCheckConnection;
TON *tPressButton;
TON *tPublishInfoDiferential;
TON *tPublishInfoConsumoCasa;

const unsigned long ONE_SECOND = 1000;
const unsigned long FIVE_SECOND = 5000;
const unsigned long ONE_MINUTE = 60000;

// INPUTS
#define LIMIT_SWITCH_OPENED 4
#define LIMIT_SWITCH_CLOSED 5
#define PZEM_RX_PIN D6
#define PZEM_TX_PIN D7

// OUTPUTS
#define PORTAL_PIN 14

// Objects
WiFiClient espClient;
PubSubClient client(espClient);

PZEM004Tv30 pzemDiferencial(PZEM_TX_PIN, PZEM_RX_PIN, 0x01);
PZEM004Tv30 pzemConsumoCasa(PZEM_TX_PIN, PZEM_RX_PIN, 0x02);

Portal *PortalSotano;

// STATE VARIABLES
boolean commandReceived = false;
char *state = "";

void setup()
{
  // Serial.begin(115200);

  WIFIConnection();

  OTAConfig();

  MQTTConnection();

  Serial.println("Connected to the WiFi network");

  tPublishInfoDoor = new TON(ONE_SECOND);
  tPublishInfoDiferential = new TON(FIVE_SECOND);
  tPublishInfoConsumoCasa = new TON(FIVE_SECOND);
  tCheckConnection = new TON(ONE_MINUTE);
  tPressButton = new TON(ONE_SECOND);

  PortalSotano = new Portal(PORTAL_PIN, LIMIT_SWITCH_OPENED, LIMIT_SWITCH_CLOSED);
}

char *getState()
{
  if (PortalSotano->getLsClosed() && !PortalSotano->getLsOpened())
  {
    return DOOR_CLOSED;
  }

  if (PortalSotano->getLsOpened() && !PortalSotano->getLsClosed())
  {
    return DOOR_OPENED;
  }

  if (PortalSotano->getLsOpened() && PortalSotano->getLsClosed())
  {
    return DOOR_AJAR;
  }

  if (!PortalSotano->getLsClosed() && !PortalSotano->getLsOpened())
  {
    return LIMIT_SWITCHES_ERROR;
  }
}

float wattsToMilliamps(float watts)
{
  return (43 * watts) / 10.4;
}

void publishInfo()
{
  if (tPublishInfoDiferential->IN(START))
  {
    StaticJsonDocument<80> jsonDoc;
    String payload = "";

    float watts = pzemDiferencial.power();
    float mAmps = wattsToMilliamps(watts);
    int volts = pzemDiferencial.voltage();

    jsonDoc["corriente"] = mAmps;
    jsonDoc["voltaje"] = volts;
    jsonDoc["potencia"] = watts;

    serializeJson(jsonDoc, payload);
    client.publish(topicDiferential, (char *)payload.c_str());

    tPublishInfoDiferential->IN(RESET);
  }

  if (tPublishInfoConsumoCasa->IN(START))
  {
    StaticJsonDocument<80> jsonDoc;
    String payload = "";

    float watts = pzemConsumoCasa.power();
    float amps = pzemConsumoCasa.current();
    int volts = pzemConsumoCasa.voltage();

    jsonDoc["corriente"] = amps;
    jsonDoc["voltaje"] = volts;
    jsonDoc["potencia"] = watts;

    serializeJson(jsonDoc, payload);
    client.publish(topicConsumoCasaSotano, (char *)payload.c_str());

    tPublishInfoConsumoCasa->IN(RESET);
  }

  if (state == getState())
  {
    tPublishInfoDoor->IN(RESET);
    return;
  }

  if (tPublishInfoDoor->IN(START))
  {
    Serial.println("Publishing info portal");
    StaticJsonDocument<100> jsonDoc;
    JsonObject portalJO = jsonDoc.createNestedObject("portal");
    String payload = "";

    state = getState();

    portalJO["estado"] = state;

    serializeJson(jsonDoc, payload);
    client.publish(topicState, (char *)payload.c_str());

    tPublishInfoDoor->IN(RESET);
  }
}

void checkMqttConnection()
{
  if (tCheckConnection->IN(START))
  {
    if (!client.connected())
    {
      ESP.restart();
    }

    tCheckConnection->IN(RESET);
  }
}

void loop()
{
  ArduinoOTA.handle();
  client.loop();
  yield();
  checkMqttConnection();
  publishInfo();

  if (commandReceived)
  {
    PortalSotano->press();

    if (tPressButton->IN(START))
    {
      PortalSotano->release();
      tPressButton->IN(RESET);
      Serial.println("realeaseButton");
      commandReceived = false;
    }
    else
    {
      Serial.println("pressButton");
    }
  }
}

void WIFIConnection()
{
  // connecting to a WiFi network
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi..");
    delay(5000);
    ESP.restart();
  }

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void OTAConfig()
{
  ArduinoOTA.setHostname(client_name);
  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    } });
  ArduinoOTA.begin();
}

void MQTTConnection()
{
  // connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  while (!client.connected())
  {
    String client_id = client_name;
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Public emqx mqtt broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
      ESP.restart();
    }
  }

  client.subscribe(topicCommand);
}

void callback(char *topicCommand, byte *payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topicCommand);
  Serial.print("Message:");
  String payload_n;

  for (int i = 0; i < length; i++)
  {
    payload_n += (char)payload[i];
  }

  Serial.println(payload_n);
  Serial.println("-----------------------");

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload_n);

  if (error)
    return;

  String command = doc["portal"]["orden"];

  if (command == OPEN || command == CLOSE || command == STOP)
  {
    commandReceived = true;
  }
}
