#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------------------------------------------------------------------
// Standalone I2C bus scanner + random-data OLED demo.
//
// Scans the ENTIRE I2C bus (addresses 1..127) looking for every connected
// device, prints the results to Serial, and renders them on an SSD1306 OLED
// together with a live "random data" value that changes each refresh.
//
// Wiring (default ESP32 I2C pins):
//   OLED SSD1306  -> SDA = GPIO21, SCL = GPIO22, addr 0x3C
//   Any I2C device -> connect to the same SDA/SCL (full bus is scanned).
// ---------------------------------------------------------------------------

// --- Pins -------------------------------------------------------------------
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // not wired on the common 0.96" modules

// Default I2C pins. Override in platformio.ini build_flags if your wiring
// differs, e.g.: -D I2C_SDA=16 -D I2C_SCL=17
#ifndef I2C_SDA
  #define I2C_SDA 21
#endif
#ifndef I2C_SCL
  #define I2C_SCL 22
#endif

// Optionally scan additional hardwired pin pairs isn't built in by default;
// the scanner scans the full ADDRESS range on the active bus, which is what
// "read all I2C" means here. See scanBus() if you want multi-pin support.

// --- Timers -----------------------------------------------------------------
#define RESCAN_INTERVAL_MS 3000   // full re-scan every 3 s
#define OLED_INTERVAL_MS 500      // random-data / render refresh every 0.5 s

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const uint8_t kScanLo = 1;   // skip 0x00 (general call)
const uint8_t kScanHi = 127;

char found[SCREEN_HEIGHT / 8][24];  // up to 8 lines of detected addresses
uint8_t foundCount = 0;

unsigned long lastScanMs = 0;
unsigned long lastOledMs = 0;

// Scan one I2C (sda, scl) pair across all addresses. Returns devices found.
// If sda/scl is NULL-pair (-1), just scans the already-initialized bus.
uint8_t scanBus(int sda, int scl) {
  if (sda >= 0 && scl >= 0) {
    Serial.printf("Scanning I2C bus SDA=%d SCL=%d...\n", sda, scl);
    Wire.begin(sda, scl, 100000);  // re-init on the requested pins
  } else {
    Serial.println("Scanning I2C bus (current pins)...");
  }

  uint8_t count = 0;
  for (uint8_t addr = kScanLo; addr <= kScanHi; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("  device 0x%02X\n", addr);
      if (count < SCREEN_HEIGHT / 8) {
        snprintf(found[count], sizeof(found[count]), "  0x%02X", addr);
      }
      count++;
    }
  }
  return count;
}

// Refresh the OLED: detected devices + a random data value.
void renderOLED(uint8_t total) {
  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("I2C Scan: ");
  display.print(total);
  display.print(" dev");

  // List up to 7 devices (line 1 is the header)
  uint8_t rows = foundCount < 7 ? foundCount : 7;
  for (uint8_t i = 0; i < rows; i++) {
    display.setCursor(0, 8 + i * 8);
    display.print(found[i]);
  }

  // Random data value at the bottom
  uint32_t rnd = esp_random();  // hardware RNG
  display.setCursor(0, SCREEN_HEIGHT - 8);
  display.print("RND ");
  display.print(rnd);

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(200);

  randomSeed(esp_random());

  // OLED lives on the primary pins (SDA=21, SCL=22 by default).
  Wire.begin(I2C_SDA, I2C_SCL, 100000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.printf("OLED not found at 0x%02X on SDA=%d SCL=%d! Check wiring.\n",
                  OLED_ADDR, I2C_SDA, I2C_SCL);
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("I2C Scanner");
    display.setCursor(0, 16);
    display.print("SDA=");
    display.print(I2C_SDA);
    display.print(" SCL=");
    display.print(I2C_SCL);
    display.display();
  }

  Serial.printf("OLED ready at 0x%02X (SDA=%d SCL=%d).\n", OLED_ADDR, I2C_SDA, I2C_SCL);

  // First scan immediately.
  foundCount = scanBus(-1, -1);
  delay(200);
}

void loop() {
  unsigned long now = millis();

  // Periodically re-scan the whole bus.
  if (now - lastScanMs >= RESCAN_INTERVAL_MS) {
    lastScanMs = now;
    foundCount = 0;
    foundCount = scanBus(-1, -1);
  }

  // Refresh the OLED (updates the random value too).
  if (now - lastOledMs >= OLED_INTERVAL_MS) {
    lastOledMs = now;
    renderOLED(foundCount);
  }
}