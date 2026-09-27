#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MPU.h"

// ---------------------------------------------------------------------------
// Wiring
// ---------------------------------------------------------------------------
// MPU9250 / MPU6500 IMU  -> I2C (default ESP32 pins: SDA = GPIO21, SCL = GPIO22)
// HC-SR04 ultrasonic     -> TRIG = D5  (GPIO5), ECHO = D23 (GPIO23)
// OLED SSD1306 128x64    -> I2C (same bus, address 0x3C)
// ---------------------------------------------------------------------------

#define TRIG_PIN 5   // D5  = GPIO5   (trigger output)
#define ECHO_PIN 23  // D23 = GPIO23  (echo input)

#define SOUND_SPEED_CM_US 0.0343f  // sound speed in cm per microsecond
#define ULTRASONIC_TIMEOUT_US 30000UL  // 30 ms -> ~5 m max range
#define ULTRASONIC_INTERVAL_MS 100      // read every 100 ms

#define MPU_ADDR 0x68  // AD0 pulled low (default)

// --- OLED ------------------------------------------------------------------
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1           // not wired on the common 0.96" modules
#define OLED_INTERVAL_MS 250    // refresh rate

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MPU mpu;

unsigned long lastUltrasonicMs = 0;
unsigned long lastOledMs = 0;

// Scan the whole I2C bus (0x01..0x7F) and print every device that ACKs.
// Returns the count of devices found so we can detect wiring issues.
uint8_t scanI2C() {
  Serial.println("I2C scanner...");
  uint8_t found = 0;
  for (uint8_t addr = 1; addr <= 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {          // device ACKed
      Serial.printf("  I2C device found at 0x%02X\n", addr);
      found++;
    } else if (error == 4) {   // other error
      Serial.printf("  Unknown error at 0x%02X\n", addr);
    }
  }
  if (found == 0) {
    Serial.println("  No I2C devices found (check wiring / pull-ups / power).");
  } else {
    Serial.printf("  -> %u device(s) found.\n", found);
  }
  return found;
}

// Returns distance in cm, or -1 on timeout / out of range.
float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_US);
  if (duration <= 0) {
    return -1.0f;
  }
  return (float)duration * SOUND_SPEED_CM_US / 2.0f;
}

// Repaint the OLED with live sensor data plus a random value.
void drawOLED(float dist, const MPU::Data& d) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Title bar
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Sensor Monitor");

  display.setTextSize(1);
  display.setCursor(0, 14);
  display.print("ACC "); display.print(d.ax, 2); display.print(",");
  display.print(d.ay, 2); display.print(",");
  display.print(d.az, 2); display.print("g");

  display.setCursor(0, 24);
  display.print("GYR "); display.print(d.gx, 1); display.print(",");
  display.print(d.gy, 1); display.print(",");
  display.print(d.gz, 1); display.print("d/s");

  display.setCursor(0, 34);
  display.print("TMP ");
  display.print(d.tempC, 1);
  display.print(" C");

  display.setCursor(0, 44);
  display.print("DST ");
  if (dist < 0.0f) {
    display.print("n/a");
  } else {
    display.print(dist, 1);
    display.print(" cm");
  }

  display.setCursor(0, 54);
  display.print("RND ");
  display.print((uint32_t)random(0, 100000));

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  Wire.begin();  // SDA = GPIO21, SCL = GPIO22

  // Scan the bus first to confirm the sensor is reachable and at which address.
  scanI2C();
  delay(100);

  // Init the OLED (SSD1306, internal charge pump).
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.printf("OLED not found at 0x%02X (check wiring/pull-ups).\n", OLED_ADDR);
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("OLED OK 0x3C");
    display.display();
    Serial.printf("OLED initialized at 0x%02X.\n", OLED_ADDR);
  }

  randomSeed(esp_random());  // seed for the random value shown on the OLED

  if (!mpu.begin(MPU_ADDR)) {
    Serial.printf("MPU not found on I2C 0x%02X (check wiring/pull-ups).\n"
                  "If the scanner found it at 0x69, tie AD0 high and set MPU_ADDR to 0x69.\n",
                  MPU_ADDR);
  } else {
    Serial.printf("MPU detected: %s (WHO_AM_I = 0x%02X), magnetometer %s\n",
                  mpu.chipName(), mpu.whoAmI(),
                  mpu.magDetected() ? "present" : "not present (MPU6500)");
  }

  delay(500);
}

void loop() {
  // IMU: read + print every loop iteration.
  MPU::Data d;
  bool newImu = mpu.read(d);
  if (newImu) {
    Serial.printf("TEMP=%5.2f C | ACC X=%6.3f Y=%6.3f Z=%6.3f g | "
                  "GYRO X=%7.2f Y=%7.2f Z=%7.2f deg/s",
                  d.tempC, d.ax, d.ay, d.az, d.gx, d.gy, d.gz);
    if (d.magOK) {
      Serial.printf(" | MAG X=%7.1f Y=%7.1f Z=%7.1f uT", d.mx, d.my, d.mz);
    }
    Serial.println();
  }

  unsigned long now = millis();

  // Ultrasonic on its own timer so the IMU loop speed isn't throttled.
  float dist = -1.0f;
  if (now - lastUltrasonicMs >= ULTRASONIC_INTERVAL_MS) {
    lastUltrasonicMs = now;

    dist = readDistanceCM();
    Serial.print("DIST=");
    if (dist < 0.0f) {
      Serial.println("(out of range / timeout) cm");
    } else {
      Serial.printf("%.1f cm\n", dist);
    }
  }

  // OLED refresh on its own slow timer (screen is only 64px tall).
  if (now - lastOledMs >= OLED_INTERVAL_MS) {
    lastOledMs = now;
    drawOLED(dist, d);
  }
}