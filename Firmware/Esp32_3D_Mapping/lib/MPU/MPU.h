#pragma once

// Minimal, dependency-free I2C driver for the InvenSense MPU9250 / MPU6500 /
// MPU9255 (and MPU6050-compatible accel/gyro core). Works for both chips:
// the accelerometer/gyroscope register map is identical, and the AK8963
// magnetometer (present only on the MPU9250) is auto-detected.

#include <Arduino.h>
#include <Wire.h>

enum AcclRange { AFS_2G, AFS_4G, AFS_8G, AFS_16G };
enum GyroRange { GFS_250, GFS_500, GFS_1000, GFS_2000 };

class MPU {
 public:
  struct Data {
    float ax, ay, az;  // g
    float gx, gy, gz;  // degrees per second
    float tempC;       // degrees Celsius
    bool  magOK;       // true if an AK8963 magnetometer was found
    float mx, my, mz;  // micro-tesla (only valid if magOK)
  };

  // Registers address + accel/gyro full-scale settings.
  // Returns false if the chip is unreachable or has an unknown WHO_AM_I.
  bool begin(uint8_t addr = 0x68, AcclRange afs = AFS_4G,
             GyroRange gfs = GFS_250);

  // Reads the latest sample into `d`. Returns true on success.
  bool read(Data& d);

  uint8_t     whoAmI() const { return _who; }
  bool        magDetected() const { return _mag; }
  const char* chipName() const;

 private:
  uint8_t _addr = 0x68;
  uint8_t _who = 0x00;
  bool    _ok = false;
  bool    _mag = false;
  float   _aRes = 8192.0f;   // LSB per g
  float   _gRes = 131.0f;    // LSB per deg/s
  float   _mRes = 0.15f;     // uT per LSB (16-bit AK8963)
  uint8_t _gfsBits = 0;      // GYRO_CONFIG FS_SEL value
  uint8_t _afsBits = 0;      // ACCEL_CONFIG FS_SEL value

  bool readBytes(uint8_t dev, uint8_t reg, uint8_t* buf, uint8_t n);
};

// ---------------------------------------------------------------------------
// Register map
// ---------------------------------------------------------------------------
#define MPU_REG_ACCEL_XOUT_H 0x3B
#define MPU_REG_TEMP_OUT_H   0x41
#define MPU_REG_GYRO_XOUT_H  0x43
#define MPU_REG_SMPLRT_DIV   0x19
#define MPU_REG_CONFIG       0x1A
#define MPU_REG_GYRO_CONFIG  0x1B
#define MPU_REG_ACCEL_CONFIG 0x1C
#define MPU_REG_ACCEL_CONFIG2 0x1D
#define MPU_REG_PWR_MGMT_1   0x6B
#define MPU_REG_WHO_AM_I     0x75

#define AK8963_ADDR          0x0C
#define AK8963_WIA           0x00
#define AK8963_CNTL1         0x0A
#define AK8963_ST1           0x02
#define AK8963_HXL           0x03