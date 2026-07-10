#include <Wire.h>
#include <math.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>
#include <Adafruit_SHT4x.h>

// MUX channel assignments:
//   ACCEL_MUX_PORT   -> MMA8452Q accelerometer (replaces one attic SHT41)
//   ATTIC_SHT_PORT   -> remaining attic SHT41 (T_attic/RH_attic)
//   2, 3             -> NAU7802 load cell amplifiers (M1, M2)
//   4                -> SHT41 sensor in electronics enclosure (T3/RH3)
//
// Ch0/Ch1 are not physically labeled on the enclosure. If the accelerometer
// turns out to be on the wrong port after installation, flip this one line.
#define ACCEL_MUX_PORT 0
#define ATTIC_SHT_PORT (1 - ACCEL_MUX_PORT)

#define MMA8452_ADDR 0x1D
#define OUT_X_MSB    0x01
#define XYZ_DATA_CFG 0x0E
#define CTRL_REG1    0x2A
#define WHO_AM_I     0x0D

// TCA9548A mux: no library dependency, just a one-byte channel-select write.
#define TCA9548A_ADDR 0x70

void muxSelect(uint8_t channel)
{
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

bool muxDetected()
{
  Wire.beginTransmission(TCA9548A_ADDR);
  return (Wire.endTransmission() == 0);
}

NAU7802 scale_1, scale_2;
Adafruit_SHT4x sht4x_attic, sht4x_3;

float xavg = 0;
float yavg = 0;
float zavg = 0;

void writeReg(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(MMA8452_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readReg(uint8_t reg)
{
  Wire.beginTransmission(MMA8452_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(MMA8452_ADDR, 1);

  if (Wire.available())
    return Wire.read();

  return 0;
}

void readXYZ(int16_t &x, int16_t &y, int16_t &z)
{
  Wire.beginTransmission(MMA8452_ADDR);
  Wire.write(OUT_X_MSB);
  Wire.endTransmission(false);

  Wire.requestFrom(MMA8452_ADDR, 6);

  x = ((Wire.read() << 8) | Wire.read()) >> 4;
  y = ((Wire.read() << 8) | Wire.read()) >> 4;
  z = ((Wire.read() << 8) | Wire.read()) >> 4;

  if (x > 2047) x -= 4096;
  if (y > 2047) y -= 4096;
  if (z > 2047) z -= 4096;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!muxDetected()) {
    Serial.println("MUX not detected!");
    while (1);
  }

  muxSelect(ATTIC_SHT_PORT);
  sht4x_attic.begin(&Wire);

  muxSelect(4);
  sht4x_3.begin(&Wire);

  muxSelect(2);
  scale_1.begin(Wire);
  scale_1.setGain(NAU7802_GAIN_2);
  scale_1.setSampleRate(NAU7802_SPS_80);
  scale_1.calibrateAFE();

  muxSelect(3);
  scale_2.begin(Wire);
  scale_2.setGain(NAU7802_GAIN_2);
  scale_2.setSampleRate(NAU7802_SPS_80);
  scale_2.calibrateAFE();

  muxSelect(ACCEL_MUX_PORT);
  uint8_t id = readReg(WHO_AM_I);
  Serial.print("Accelerometer WHO_AM_I = 0x");
  Serial.println(id, HEX);

  // Standby
  writeReg(CTRL_REG1, 0x00);
  // +/-2 g
  writeReg(XYZ_DATA_CFG, 0x00);
  // Active
  writeReg(CTRL_REG1, 0x01);

  Serial.println("Time_ms,T_attic_C,RH_attic_pct,M1_adc,M2_adc,T3_C,RH3_pct,vibration_rms_g");
}

void loop() {
  String row = "";
  row += millis();

  sensors_event_t humidity, temp;

  muxSelect(ATTIC_SHT_PORT);
  sht4x_attic.getEvent(&humidity, &temp);
  row += "," + String(temp.temperature) + "," + String(humidity.relative_humidity);

  muxSelect(2);
  row += "," + String(scale_1.getReading());

  muxSelect(3);
  row += "," + String(scale_2.getReading());

  muxSelect(4);
  sht4x_3.getEvent(&humidity, &temp);
  row += "," + String(temp.temperature) + "," + String(humidity.relative_humidity);

  // Vibration RMS: reuses the validated VibrationMonitor.ino algorithm.
  // 200 samples @ 5 ms ~= 1 second, consuming part of the original 5 s
  // loop cadence in place of dead delay time.
  muxSelect(ACCEL_MUX_PORT);
  const int N = 200;
  double sumsq = 0;
  for (int i = 0; i < N; i++) {
    int16_t xr, yr, zr;
    readXYZ(xr, yr, zr);

    // remove DC / gravity
    xavg = 0.99 * xavg + 0.01 * xr;
    yavg = 0.99 * yavg + 0.01 * yr;
    zavg = 0.99 * zavg + 0.01 * zr;

    float xv = xr - xavg;
    float yv = yr - yavg;
    float zv = zr - zavg;

    float vib = sqrt(xv * xv + yv * yv + zv * zv);
    sumsq += vib * vib;

    delay(5);
  }
  float rmsCounts = sqrt(sumsq / N);
  float rmsG = rmsCounts / 1024.0;
  row += "," + String(rmsG, 5);

  Serial.println(row);

  delay(4000);  // remaining time to approximate the original ~5 s cadence
}
