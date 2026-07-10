#include <Wire.h>
#include <SparkFun_I2C_Mux_Arduino_Library.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>
#include <Adafruit_SHT4x.h>

// MUX channel assignments:
//   0, 1 -> SHT41 sensors in attic (T1/RH1, T2/RH2)
//   2, 3 -> NAU7802 load cell amplifiers (M1, M2)
//   4    -> SHT41 sensor in electronics enclosure (T3/RH3)

QWIICMUX myMux;
NAU7802 scale_1, scale_2;
Adafruit_SHT4x sht4x_1, sht4x_2, sht4x_3;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!myMux.begin(0x70, Wire)) {
    Serial.println("MUX not detected!");
    while (1);
  }

  myMux.setPort(0);
  sht4x_1.begin(&Wire);

  myMux.setPort(1);
  sht4x_2.begin(&Wire);

  myMux.setPort(4);
  sht4x_3.begin(&Wire);

  myMux.setPort(2);
  scale_1.begin(Wire);
  scale_1.setGain(NAU7802_GAIN_2);
  scale_1.setSampleRate(NAU7802_SPS_80);
  scale_1.calibrateAFE();

  myMux.setPort(3);
  scale_2.begin(Wire);
  scale_2.setGain(NAU7802_GAIN_2);
  scale_2.setSampleRate(NAU7802_SPS_80);
  scale_2.calibrateAFE();

  Serial.println("Time_ms,T1_C,RH1_pct,T2_C,RH2_pct,M1_adc,M2_adc,T3_C,RH3_pct");
}

void loop() {
  String row = "";
  row += millis();

  sensors_event_t humidity, temp;

  myMux.setPort(0);
  sht4x_1.getEvent(&humidity, &temp);
  row += "," + String(temp.temperature) + "," + String(humidity.relative_humidity);

  myMux.setPort(1);
  sht4x_2.getEvent(&humidity, &temp);
  row += "," + String(temp.temperature) + "," + String(humidity.relative_humidity);

  myMux.setPort(2);
  row += "," + String(scale_1.getReading());

  myMux.setPort(3);
  row += "," + String(scale_2.getReading());

  myMux.setPort(4);
  sht4x_3.getEvent(&humidity, &temp);
  row += "," + String(temp.temperature) + "," + String(humidity.relative_humidity);

  Serial.println(row);

  delay(5000);
}
