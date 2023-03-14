/*
  Esp32 BLE tracker (mqtt)
  author: Vörös Ákos, redman
*/

#include <cstddef>
#include "Arduino.h"
#include "Callback.h"
#include "definitions.h"
#include "utilities.cpp"
#include "log.cpp"
#include "bluetooth.cpp"
#include "led.cpp"
#include "database.cpp"
#include "wifi.cpp"
#include "webserver.cpp"
#include "mqtt.cpp"
#include "webhook.cpp"
#include "esp_log.h"

Log rlog;
Led led(rlog);
Database database(rlog);
Wifi wifi(rlog);
Webserver webserver(rlog);
BlueTooth blueTooth(rlog, led);
Mqtt mqtt(rlog);
Webhook webhook(rlog);

// This signal will be emitted when we process characters
// https://github.com/tomstewart89/Callback
Signal<boolean> wifiStatusChanged;
Signal<int> errorCodeChanged;
Signal<String> messageArrived;
Signal<MQTTMessage> mqttMessageSend;
Signal<Device> deviceChanged;
Signal<String> ipAddressChanged;

String log_prefix = "[MAIN]";

int rebootAfterHours = 0;

void setup() {
  // callback(s)

  // Wifi status changed
  MethodSlot<Mqtt, boolean> wifiChangedForMqtt(&mqtt,&Mqtt::setConnected);
  MethodSlot<BlueTooth, boolean> wifiChangedForBluetooth(&blueTooth,&BlueTooth::setConnected);
  MethodSlot<Webserver, boolean> wifiChangedForWebserver(&webserver,&Webserver::setConnected);
  wifiStatusChanged.attach(wifiChangedForMqtt);
  wifiStatusChanged.attach(wifiChangedForBluetooth);
  wifiStatusChanged.attach(wifiChangedForWebserver);
  
  // Emit an error code for led
  MethodSlot<Led, int> errorCodeChangedForLed(&led,&Led::setMessage);
  errorCodeChanged.attach(errorCodeChangedForLed);

  // MQTT and Blutooth command handleing
  // Arrive  
  MethodSlot<Database, String> messageSendForDatabase(&database,&Database::receiveCommand);
  messageArrived.attach(messageSendForDatabase);

  // Send
  MethodSlot<Mqtt, MQTTMessage> mqttMessageSendForMqtt(&mqtt,&Mqtt::sendMqttMessage);
  mqttMessageSend.attach(mqttMessageSendForMqtt);

  MethodSlot<Webhook, Device> deviceChangedForWebhook(&webhook,&Webhook::callWebhook);
  deviceChanged.attach(deviceChangedForWebhook);

  MethodSlot<Mqtt, String> ipAddressChangedForMqtt(&mqtt,&Mqtt::ipAddressChanged);
  ipAddressChanged.attach(ipAddressChangedForMqtt);

  rlog.setup();
  led.setup();
  database.setup();
  wifi.setup(database, wifiStatusChanged, errorCodeChanged, ipAddressChanged);
  blueTooth.setup(database, mqttMessageSend, deviceChanged);  
  // Must be after Wifi setup
  webserver.setup(database);
  webhook.setup(database);
  
  mqtt.setup(database, errorCodeChanged, messageArrived);
  // Connect to WiFi
  wifi.connectWifi();

  // Workaround for stuc after some days
  rebootAfterHours = database.getValueAsInt(DB_REBOOT_TIMEOUT);
}

void loop() {  
  // Object loops
  rlog.loop();
  led.loop();
  database.loop();
  wifi.loop();
  blueTooth.loop();  
  webserver.loop();
  mqtt.loop();
  webhook.loop();

  if ((rebootAfterHours > 0) && (millis() > (rebootAfterHours * 60 * 60 * 1000))) {
    ESP.restart();
  }
}
