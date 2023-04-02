# Garaje door
## Description
This is with ESP8266 NodeMCU v3.

## Configuration
You'll need your own variables for MQTT and WIFI connection. I use in this format:

WIFI connection file:
const char *ssid = "";
const char *password = "";

MQTT connection file:
const char *mqtt_broker = "";
const char *topicState = "";
const char *topicCommand = "";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = ;
const char *client_name = "";

## Connections
| Name                | Name in board | GPIO/IDE Arduino name |
|---------------------|---------------|-----------------------|
| Closed limit switch | D1            | 4                     |
| Open limit switch   | D2            | 5                     |
| Engine output       | D5            | 14                    |
| PZEM_RX pin         | D6            | 12                    |
| PZEM_TX pin         | D7            | 13                    |