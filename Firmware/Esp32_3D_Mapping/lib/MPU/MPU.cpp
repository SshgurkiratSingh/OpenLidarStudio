#include "MPU.h"

bool MPU::readBytes(uint8_t dev, uint8_t reg, uint8_t* buf, uint8_t n) {
  Wire.beginTransmission(dev);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)dev, (int)n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

const char* MPU::chipName() const {
  switch (_who) {
    case 0x71: return "MPU9250";
    case 0x70: return "MPU6500";
    case 0x73: return "MPU9255";
    default:   return "unknown/MPU6050";
  }
}

bool MPU::begin(uint8_t addr, AcclRange afs, GyroRange gfs) {
  _addr = addr;

  // Identify the chip.
  uint8_t who = 0;
  if (!readBytes(_addr, MPU_REG_WHO_AM_I, &who, 1)) return false;
  _who = who;

  // Set full-scale resolutions and the FS_SEL register bits.
  switch (afs) {
    case AFS_2G:  _aRes = 16384.0f; _afsBits = 0b0000; break;
    case AFS_4G:  _aRes =  8192.0f; _afsBits = 0b0001; break;
    case AFS_8G:  _aRes =  4096.0f; _afsBits = 0b0010; break;
    case AFS_16G: _aRes =  2048.0f; _afsBits = 0b0011; break;
  }
  switch (gfs) {
    case GFS_250:  _gRes = 131.0f;  _gfsBits = 0b0000; break;
    case GFS_500:  _gRes = 65.5f;   _gfsBits = 0b0001; break;
    case GFS_1000: _gRes = 32.8f;   _gfsBits = 0b0010; break;
    case GFS_2000: _gRes = 16.4f;   _gfsBits = 0b0011; break;
  }

  // Wake the device (clear SLEEP bit, use PLL for clock source).
  Wire.beginTransmission(_addr);
  Wire.write(MPU_REG_PWR_MGMT_1);
  Wire.write(0x01);
  Wire.endTransmission();
  delay(10);

  // Reset -> interrupts/signal path config.
  // (0x00 wakes; 0x01 keeps clock source set above.)
  Wire.beginTransmission(_addr);
  Wire.write(MPU_REG_PWR_MGMT_1);
  Wire.write(0x01);
  Wire.endTransmission();

  // Sample rate divider + DLPF config (gyro/accel bandwidth ~92 Hz).
  Wire.beginTransmission(_addr);
  Wire.write(MPU_REG_SMPLRT_DIV);
  Wire.write(0x00);
  Wire.endTransmission();

  Wire.beginTransmission(_addr);
  Wire.write(MPU_REG_CONFIG);
  Wire.write(0x02);
  Wire.endTransmission();

  Wire.beginTransmission(_addr);
  Wire.write(MPU_REG_GYRO_CONFIG);
  Wire.write(_gfsBits << 3);
  Wire.endTransmission();

  Wire.beginTransmission(_addr);
  Wire.write(MPU_REG_ACCEL_CONFIG);
  Wire.write(_afsBits << 3);
  Wire.endTransmission();

  Wire.beginTransmission(_addr);
  Wire.write(MPU_REG_ACCEL_CONFIG2);
  Wire.write(0x02);
  Wire.endTransmission();

  // Detect + start the AK8963 magnetometer (only on MPU9250).
  uint8_t wia = 0;
  Wire.beginTransmission(AK8963_ADDR);
  Wire.write(AK8963_WIA);
  Wire.endTransmission(false);
  if (Wire.requestFrom((int)AK8963_ADDR, (int)1) == 1) {
    wia = Wire.read();
  }
  _mag = (wia == 0x48);
  if (_mag) {
    Wire.beginTransmission(AK8963_ADDR);
    Wire.write(AK8963_CNTL1);
    Wire.write(0x16);  // continuous mode, 100 Hz, 16-bit
    Wire.endTransmission();
  }

  _ok = true;
  return true;
}

bool MPU::read(Data& d) {
  if (!_ok) return false;

  uint8_t buf[7];
  if (!readBytes(_addr, MPU_REG_ACCEL_XOUT_H, buf, 6)) return false;

  int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
  int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
  int16_t az = (int16_t)((buf[4] << 8) | buf[5]);
  d.ax = ax / _aRes;
  d.ay = ay / _aRes;
  d.az = az / _aRes;

  if (!readBytes(_addr, MPU_REG_TEMP_OUT_H, buf, 2)) return false;
  int16_t tc = (int16_t)((buf[0] << 8) | buf[1]);
  d.tempC = (float)tc / 333.87f + 21.0f;

  if (!readBytes(_addr, MPU_REG_GYRO_XOUT_H, buf, 6)) return false;
  int16_t gx = (int16_t)((buf[0] << 8) | buf[1]);
  int16_t gy = (int16_t)((buf[2] << 8) | buf[3]);
  int16_t gz = (int16_t)((buf[4] << 8) | buf[5]);
  d.gx = gx / _gRes;
  d.gy = gy / _gRes;
  d.gz = gz / _gRes;

  d.magOK = false;
  if (_mag) {
    buf[0] = 0;
    if (readBytes(AK8963_ADDR, AK8963_ST1, buf, 7)) {
      uint8_t st1 = buf[0];
      uint8_t st2 = buf[6];
      if ((st1 & 0x01) && !(st2 & 0x08)) {  // data ready and not overflow
        int16_t mx = (int16_t)((buf[2] << 8) | buf[1]);
        int16_t my = (int16_t)((buf[4] << 8) | buf[3]);
        int16_t mz = (int16_t)((buf[6] << 8) | buf[5]);
        d.mx = mx * _mRes;
        d.my = my * _mRes;
        d.mz = mz * _mRes;
        d.magOK = true;
      }
    }
  }

  return true;
}