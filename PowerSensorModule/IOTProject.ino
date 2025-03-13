#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> 
#include <ZMPT101B.h>
 
#define NUMFLAKES     10 // Number of snowflakes in the animation example
#define SENSITIVITY 500.0f
#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };

#define PWM_FREQ 5000
#define PWM_RES 10
#define RED_GPIO       42
#define YELLOW_GPIO    41
#define GREEN_GPIO     40
#define LDR_GPIO       4
#define SW 2
#define TOPIC_VOLT_NOW  TOPIC_PREFIX "/volt/now"
#define TOPIC_VOLT_NORM  TOPIC_PREFIX "/volt/norm"
#define TOPIC_VOLT_STAGE  TOPIC_PREFIX "/volt/stage"
#define TOPIC_LED_RED  TOPIC_PREFIX "/led/red"
#define TOPIC_LED_YELLOW  TOPIC_PREFIX "/led/yellow"
#define TOPIC_LED_GREEN  TOPIC_PREFIX "/led/green"
#define DISPLAY_TEXT TOPIC_PREFIX "/display/text"
#define TOPIC_SW   TOPIC_PREFIX "/SW"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int SW_br;
ZMPT101B voltageSensor(4, 50.0);

WiFiClient wifiClient;
PubSubClient mqtt(MQTT_BROKER, 1883, wifiClient);
uint32_t last_publish;


void connect_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  printf("WiFi MAC address is %s\n", WiFi.macAddress().c_str());
  printf("Connecting to WiFi %s.\n", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    printf(".");
    fflush(stdout);
    delay(500);
  }
  printf("\nWiFi connected.\n");
}


void connect_mqtt() {
  printf("Connecting to MQTT broker at %s.\n", MQTT_BROKER);
  if (!mqtt.connect("", MQTT_USER, MQTT_PASS)) {
    printf("Failed to connect to MQTT broker.\n");
    for (;;) {} // wait here forever
  }
  mqtt.setCallback(mqtt_callback);
  mqtt.subscribe(TOPIC_VOLT_NORM);
  // mqtt.subscribe(TOPIC_LED_RED);
  // mqtt.subscribe(TOPIC_LED_YELLOW);
  // mqtt.subscribe(TOPIC_LED_GREEN);
  // mqtt.subscribe(DISPLAY_TEXT);
  printf("MQTT broker connected.\n");
}

int VOLTnorm;
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, TOPIC_VOLT_NORM) == 0) {
    payload[length] = 0; // null-terminate the payload to treat it as a string
    int value = atoi((char*)payload); 
    VOLTnorm = value;
  }
//   if (strcmp(topic, TOPIC_LED_YELLOW) == 0) {
//     payload[length] = 0; // null-terminate the payload to treat it as a string
//     int value = atoi((char*)payload); 
//     if (value == 0) {
//       digitalWrite(YELLOW_GPIO, LOW);
//     }
//     else if (value == 1) {
//       digitalWrite(YELLOW_GPIO, HIGH);
//     }
//     else {
//       printf("Invalid payload received.\n");
//     }
//   }
//   if (strcmp(topic, TOPIC_LED_GREEN) == 0) {
//     payload[length] = 0; // null-terminate the payload to treat it as a string
//     int value = atoi((char*)payload); 
//     if (value == 0) {
//       digitalWrite(GREEN_GPIO, LOW);
//     }
//     else if (value == 1) {
//       digitalWrite(GREEN_GPIO, HIGH);
//     }
//     else {
//       printf("Invalid payload received.\n");
//     }
//   }
//   if (strcmp(topic, DISPLAY_TEXT) == 0) {
//     display.clearDisplay();
//     payload[length] = 0; // null-terminate the payload to treat it as a string
//     String message = String((char*)payload);
//     printf("ffff %s\n",message.c_str());
//     display.setCursor(0,0);
//     display.setTextSize(1);
//     display.setTextColor(SSD1306_WHITE);
//     display.println((char*)payload);
//     display.display();
//   }
}
void setupPwm(int pin){
  ledcAttach(pin,PWM_FREQ,PWM_RES);
}

void setup() {
  connect_wifi();
  connect_mqtt();
  Wire.begin(48, 47);
  Serial.begin(115200);
  voltageSensor.setSensitivity(SENSITIVITY);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(2000); // Will be chage
  display.clearDisplay();
  display.setTextColor(WHITE);
  last_publish = 0;
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  display.clearDisplay();

  analogSetAttenuation(ADC_11db);
}
int switchState=0;
void loop() {
  // check for incoming subscribed topics
  float voltage = voltageSensor.getRmsVoltage();
  Serial.println(voltage);
  display.clearDisplay();
  display.setCursor(10,0);  
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.print("Voltage");
  display.setCursor(10,25);  
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.print(String(voltage));
  display.display();
  String payloadBytes(voltage);
  mqtt.publish(TOPIC_VOLT_NOW, payloadBytes.c_str());
  mqtt.loop();
  
  // publish light value periodically (without using delay)
  uint32_t now = millis();
  if (now - last_publish >= 2000) {
    if (voltage>=VOLTnorm*1.1){
      int VOLTstg = 2;
      String payload(VOLTstg);
      mqtt.publish(TOPIC_VOLT_STAGE, payload.c_str());
      last_publish = now;
    }
    else if(voltage<=VOLTnorm*0.9){
      int VOLTstg = 0;
      String payload(VOLTstg);
      mqtt.publish(TOPIC_VOLT_STAGE, payload.c_str());
      last_publish = now;
    }
    else{
      int VOLTstg = 1;
      String payload(VOLTstg);
      mqtt.publish(TOPIC_VOLT_STAGE, payload.c_str());
      last_publish = now;
    }
  }
}
