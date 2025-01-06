#include <SoftwareSerial.h>

const int rxSensorPin = 14;
const int txSensorPin = 12;

uint8_t buffer[12] = {0};
SoftwareSerial mySerial(rxSensorPin, txSensorPin);

float o2Concentration, o2Flow, o2Temperature;

bool readMeasurement(float & o2Concentration, float & o2Flow, float & o2Temperature) {
  // Comando para leer el flujo
  uint8_t command[] = {0x11, 0x01, 0x01, 0xED};

  mySerial.write(command, sizeof(command));

  delay(20);

  if (mySerial.available() >= 12) {
    int len = mySerial.readBytes(buffer, sizeof(buffer));
    if (len == 12 && buffer[0] == 0x16) {
      o2Concentration = (buffer[3] * 256 + buffer[4]) / 10.0;
      o2Flow = (buffer[5] * 256 + buffer[6]) / 100.0;
      o2Temperature = (buffer[7] * 256 + buffer[8]) / 10.0;

      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  while (!Serial);
}

void loop() {
  if (readMeasurement(o2Concentration, o2Flow, o2Temperature)) {
    Serial.printf("Concentración de O2: %.2f %%\n", o2Concentration);
    Serial.printf("Flujo de O2: %.2f L/min\n", o2Flow);
    Serial.printf("Temperatura de O2: %.2f °C\n", o2Temperature);
  } else {
    Serial.println("Error en la lectura FlowmeterCubic");
  }
  delay(1000);
}
