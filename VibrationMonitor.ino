#include <Wire.h>
#include <math.h>

#define MMA8452_ADDR 0x1D

#define OUT_X_MSB    0x01
#define XYZ_DATA_CFG 0x0E
#define CTRL_REG1    0x2A
#define WHO_AM_I     0x0D

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

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  uint8_t id = readReg(WHO_AM_I);

  Serial.print("WHO_AM_I = 0x");
  Serial.println(id, HEX);

  // Standby
  writeReg(CTRL_REG1, 0x00);

  // +/-2 g
  writeReg(XYZ_DATA_CFG, 0x00);

  // Active
  writeReg(CTRL_REG1, 0x01);

  Serial.println("Running vibration monitor...");
}

void loop()
{
  const int N = 200;   // ~1 sec at 5 ms/sample

  double sumsq = 0;

  for (int i = 0; i < N; i++)
  {
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

  Serial.print("Vibration RMS = ");
  Serial.print(rmsG, 5);
  Serial.println(" g");
}