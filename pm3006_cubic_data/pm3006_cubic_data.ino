#include "PM3006S.h"

float pm1;
float pm2_5;
float pm10;
float tsp;
float pq0_3;
float pq0_5;
float pq1;
float pq2_5;
float pq5;
float pq10;
int err_cubic;

PM3006S cubic;

#ifdef ESP32
HardwareSerial puerto_pm10(2);
#endif

void setup() {
  Serial.begin(9600);
  cubic.begin(&puerto_pm10);
  
  Serial.println("Iniciando...");
}

void loop() {
  String Data = "data";
  
  cubic.set_read();
  err_cubic = cubic.read(&pm1, &pm2_5, &pm10, &tsp, &pq0_3, &pq0_5, &pq1, &pq2_5, &pq5, &pq10);

  if (!err_cubic) {
    Data += ";pm1:" + String(pm1) + ";pm2_5:" + String(pm2_5) + ";pm10:" + String(pm10) + ";tsp:" + String(tsp) 
          + ";pq0_3:" + String(pq0_3) + ";pq0_5:" + String(pq0_5) 
          + ";pq1:" + String(pq1) + ";pq2_5:" + String(pq2_5) 
          + ";pq5:" + String(pq5) + ";pq10:" + String(pq10);
  }

  // Imprime los datos
  Serial.println(Data);
  delay(1000);
}
