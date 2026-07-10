#include <Wire.h>
#include <SparkFun_I2C_Mux_Arduino_Library.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>
#include <Adafruit_SHT4x.h>

QWIICMUX myMux; 
NAU7802 scale_1, scale_2; 
Adafruit_SHT4x sht4x_1, sht4x_2; 

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!myMux.begin(0x70, Wire)) {
    Serial.println("MUX not detected!");
    while (1);
  }

  auto selectMuxChannel = [](uint8_t channel) {
    myMux.setPort(channel);
  };

  // Initialize SHT4x Sensors
  selectMuxChannel(0);
  sht4x_1.begin(&Wire);

  selectMuxChannel(1);
  sht4x_2.begin(&Wire);

  // Initialize Scales
  selectMuxChannel(2);
  scale_1.begin(Wire);
  scale_1.setGain(NAU7802_GAIN_2);
  scale_1.setSampleRate(NAU7802_SPS_80);
  scale_1.calibrateAFE();  // Always follow gain/sample changes with this
  //scale_1.tare();


  selectMuxChannel(3);
  scale_2.begin(Wire);
  scale_2.setGain(NAU7802_GAIN_2);
  scale_2.setSampleRate(NAU7802_SPS_80);
  scale_2.calibrateAFE();  // Always follow gain/sample changes with this
  //scale_2.tare();


  Serial.println("Time_ms,T1_C,RH1_pct,T2_C,RH2_pct,M1_g,M2_g");
}

void loop() {
  auto selectMuxChannel = [](uint8_t channel) {
    myMux.setPort(channel);
  };

  String row = "";
  row += millis();

  sensors_event_t humidity, temp;

  selectMuxChannel(0);
  sht4x_1.getEvent(&humidity, &temp);
  row += "," + String(temp.temperature) + "," + String(humidity.relative_humidity);

  selectMuxChannel(1);
  sht4x_2.getEvent(&humidity, &temp);
  row += "," + String(temp.temperature) + "," + String(humidity.relative_humidity);

  selectMuxChannel(2);
  long adc1 = scale_1.getReading(); // Load Cell on Channel 2
  row += "," + String(adc1);

  selectMuxChannel(3);
  long adc2 = scale_2.getReading(); // Load Cell on Channel 3
  row += "," + String(adc2);

  Serial.println(row);

  delay(5000);
}