#include <SDS198.h>
#include <SDS011.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#define OLED_SDA 21
//#define OLED_SCL 22
#define RGB_GREEN_1 14
#define RGB_RED_1 12
#define RGB_BLUE_1 13
#define RGB_GREEN_2 25
#define RGB_RED_2 27
#define RGB_BLUE_2 26

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//Adafruit_SH1106 display(21, 22);

//SDS
float p100 = 0;
byte id1, id2;
float p10 = 0;
float p25 = 0;
SDS198 sensor_pm100;
SDS011 sensor_pm10;

#ifdef ESP32
HardwareSerial puerto_100(0);
HardwareSerial puerto_10(2);
#endif

void setColor_pm100(int redValue, int greenValue, int blueValue) {
  ledcWrite(0, redValue);
  ledcWrite(1, greenValue);
  ledcWrite(2, blueValue);
}

void setColor_pm10(int redValue, int greenValue, int blueValue) {
  ledcWrite(3, redValue);
  ledcWrite(4, greenValue);
  ledcWrite(5, blueValue);
}

void setup() {
  Serial.begin(38400);

  sensor_pm100.begin(&puerto_100);
  sensor_pm10.begin(&puerto_10);
  Serial.println("Started");

  ledcSetup(0, 5000, 8);
  ledcSetup(1, 5000, 8);
  ledcSetup(2, 5000, 8);
  ledcSetup(3, 5000, 8);
  ledcSetup(4, 5000, 8);
  ledcSetup(5, 5000, 8);
  ledcAttachPin(RGB_RED_1, 0);
  ledcAttachPin(RGB_GREEN_1, 1);
  ledcAttachPin(RGB_BLUE_1, 2);
  ledcAttachPin(RGB_RED_2, 3);
  ledcAttachPin(RGB_GREEN_2, 4);
  ledcAttachPin(RGB_BLUE_2, 5);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) {}
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();
  display.clearDisplay();
}

void loop() {

  sensor_pm100.read(&p100, &id1, &id2);
  Serial.println("PM100:  " + String(p100));
  sensor_pm10.read(&p10, &p25);
  Serial.println("PM2.5: " + String(p25));
  Serial.println("PM10:  " + String(p10));

  String sds011_data = String(p10) + "/" + String(p25);
  String sds198_data = String(p100);

  if (p100 >= 0 && p100 <= 100) {
    setColor_pm100(0, 0, 255);
  } else if (p100 >= 100 && p100 <= 200) {
    setColor_pm100(255, 255, 0);
  } else if (p100 >= 200 && p100 <= 400) {
    setColor_pm100(255, 0, 0);
  } else {
    setColor_pm100(255, 0, 255);
  }

  if (p10 >= 0 && p10 <= 200) {
    setColor_pm10(0, 0, 255);
  } else if (p10 >= 200 && p10 <= 400) {
    setColor_pm10(255, 255, 0);
  } else if (p10 >= 400 && p10 <= 600) {
    setColor_pm10(255, 0, 0);
  } else {
    setColor_pm10(255, 0, 255);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("pmTester 100/10/2.5");
  display.println("pm100: " + String(p100));
  display.println("pm10: " + String(p10));
  display.println("pm2.5: " + String(p25));
  display.display();
  delay(250);
}
