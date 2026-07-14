#include <Wire.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>
#include <Adafruit_SHT4x.h>

// Office/lab load-cell stability reference (see Tasks.md /
// ExperimentNotes.md in the main attic-experiment repo for background).
// Single NAU7802 load-cell amplifier + single SHT41, both directly on the
// main I2C bus - no TCA9548A mux needed, since their fixed addresses
// (0x2A and 0x44) don't collide. This is deliberately the simplest
// possible version of the attic rig's hardware: one channel of each
// sensor type, monitoring a fixed calibration mass in a conditioned
// indoor environment.
//
// Streams raw per-sample readings continuously; office_reference_logger.py
// batches them into 60-second averages, mirroring the attic experiment's
// on-PC averaging model (data_logger_w_Vibration.py) so the two datasets
// stay directly comparable.

NAU7802 scale;
Adafruit_SHT4x sht4x;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!sht4x.begin(&Wire)) {
    Serial.println("SHT41 not detected!");
    while (1);
  }

  if (!scale.begin(Wire)) {
    Serial.println("NAU7802 not detected!");
    while (1);
  }
  scale.setGain(NAU7802_GAIN_2);
  scale.setSampleRate(NAU7802_SPS_80);
  scale.calibrateAFE();

  Serial.println("Time_ms,T_C,RH_pct,mass_adc");
}

void loop() {
  sensors_event_t humidity, temp;
  sht4x.getEvent(&humidity, &temp);

  String row = "";
  row += millis();
  row += "," + String(temp.temperature);
  row += "," + String(humidity.relative_humidity);
  row += "," + String(scale.getReading());

  Serial.println(row);

  delay(1000);  // ~1 sample/second; office_reference_logger.py aggregates into 60 s windows
}
