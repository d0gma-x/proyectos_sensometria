#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESP32Time.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <SD.h>
#include <FS.h>

#define OLED_SDA 21
#define OLED_SCL 22
#define SD_CS 5

// Wi-Fi ESP32
ESP32Time rtc;
const char* ssid = "your_ssid";
const char* password = "your_pass";
const char* ntpServer = "pool.ntp.org";
String utcTimeString;
const long gmtOffset_sec = -4 * 3600;
const int daylightOffset_sec = 0;
WiFiServer server(80);

// Pantalla OLED
Adafruit_SH1106 display(21, 22);

// SHT31
Adafruit_SHT31 sht31 = Adafruit_SHT31();
float sht31_temp, sht31_hum;

// GPS
TinyGPSPlus gps;
float latitude, longitude;
float speed_kmph = 0;
float course_degrees = 0;
String lat_str, lon_str;

#ifdef ESP32
HardwareSerial SerialGPS(2);
#endif

String timeToString(time_t time) {
  struct tm timeInfo;
  gmtime_r(&time, &timeInfo);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeInfo);
  return String(buffer);
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

String obtenerDireccionCardinal(float grados) {
  if (grados >= 337.5 || grados < 22.5) return "Norte";
  if (grados >= 22.5 && grados < 67.5) return "Noreste";
  if (grados >= 67.5 && grados < 112.5) return "Este";
  if (grados >= 112.5 && grados < 157.5) return "Sureste";
  if (grados >= 157.5 && grados < 202.5) return "Sur";
  if (grados >= 202.5 && grados < 247.5) return "Suroeste";
  if (grados >= 247.5 && grados < 292.5) return "Oeste";
  if (grados >= 292.5 && grados < 337.5) return "Noroeste";
  return "Desconocido";
}

void setup() {
  Serial.begin(115200);
  SerialGPS.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  server.begin();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  if (!sht31.begin(0x44)) {
    Serial.println("ERROR SHT31");
    while (1);
  }

  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.display();
  display.clearDisplay();

  SD.begin(SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
  }

  File file = SD.open("/datalog.json");
  if (!file) {
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/datalog.json", "");
  } else {
    Serial.println("File already exists");
  }
  file.close();
}

void loop() {
  time_t utcTime = rtc.getEpoch();
  utcTimeString = String(timeToString(utcTime));
  sht31_temp = sht31.readTemperature();
  sht31_hum = sht31.readHumidity();

  while (SerialGPS.available() > 0) {
    if (gps.encode(SerialGPS.read())) {
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        lat_str = String(latitude, 6);
        longitude = gps.location.lng();
        lon_str = String(longitude, 6);
        speed_kmph = gps.speed.kmph();
        course_degrees = gps.course.deg();
      }
    }
  }

  // Mostrar en pantalla OLED
  String display_data_1 = String(sht31_temp) + "/" + String(sht31_hum);
  String display_data_2 = lat_str + "/" + lon_str;
  String display_speed = String(speed_kmph) + " km/h";
  String display_course = String(course_degrees) + "g/ " + obtenerDireccionCardinal(course_degrees);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(display_data_1);
  display.setCursor(0, 10);
  if (gps.location.isValid()) {
    display.println(display_data_2);
    display.setCursor(0, 20);
    display.println(display_speed);
    display.setCursor(0, 30);
    display.println(display_course);
  } else {
    display.println("GPS no disponible");
  }
  display.setCursor(0, 40);
  display.println(WiFi.localIP());
  display.setCursor(0, 50);
  display.println(utcTimeString + " UTC");
  display.display();

  // Guardar datos en JSON
  const size_t bufferSize = JSON_OBJECT_SIZE(200);
  DynamicJsonDocument doc(bufferSize);
  doc["temp"] = sht31_temp;
  doc["hum"] = sht31_hum;
  doc["lat"] = lat_str;
  doc["lon"] = lon_str;
  doc["speed"] = speed_kmph;
  doc["course"] = course_degrees;
  doc["courseDir"] = obtenerDireccionCardinal(course_degrees);
  doc["date"] = rtc.getDate();
  doc["timeUTC"] = utcTimeString;

  String json;
  serializeJson(doc, json);
  json += "\n";

  appendFile(SD, "/datalog.json", json.c_str());

  // Servidor web
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Nueva conexion");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head><title>Weather ESP32</title><style>h2 { text-align: center; }</style></head>");
    client.println("<body><h2>Datos JSON Weather v4</h2>");
    client.print("<pre>");
    client.print(json);
    client.print("</pre>");
    client.println("<ul>");
    client.println("<li>Temperatura: " + String(sht31_temp) + "</li>");
    client.println("<li>Humedad: " + String(sht31_hum) + "</li>");
    client.println("<li>Latitud: " + lat_str + "</li>");
    client.println("<li>Longitud: " + lon_str + "</li>");
    client.println("<li>Velocidad: " + String(speed_kmph) + " km/h</li>");
    client.println("<li>Rumbo: " + String(course_degrees) + "° " + obtenerDireccionCardinal(course_degrees) + "</li>");
    client.println("<li>Fecha: " + rtc.getDate() + "</li>");
    client.println("<li>Hora: " + rtc.getTime() + "</li>");
    client.println("<li>Hora UTC: " + utcTimeString + "</li>");
    client.println("</ul></body></html>");
    client.println("<meta http-equiv=\"refresh\" content=\"5\">"); // Actualizar automáticamente cada 5 segundos
  }

  delay(2000);
}
