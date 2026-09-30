#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 15
#define NUM_LEDS 300

// WiFi credentials (placeholder, update before flashing)
const char* ssid = "Airtel_Node";
const char* password = "air66343";

// Network
WiFiUDP udp_discovery;
WiFiUDP udp_stream;
const unsigned int discovery_port = 5000;
const unsigned int stream_port = 5001;

// LED Strip
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Timing for discovery beacon
unsigned long last_beacon_time = 0;
const unsigned long BEACON_INTERVAL_MS = 2000;

uint8_t packet_buffer[1024];

void setup() {
    Serial.begin(115200);
    
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    
    udp_discovery.begin(discovery_port);
    udp_stream.begin(stream_port);
    
    Serial.println("UDP listening on ports 5000 (discovery) and 5001 (stream)");
}

void loop() {
    // 1. Send Discovery Beacon
    unsigned long now = millis();
    if (now - last_beacon_time >= BEACON_INTERVAL_MS) {
        last_beacon_time = now;
        
        // Broadcast to 255.255.255.255
        udp_discovery.beginPacket(IPAddress(255, 255, 255, 255), discovery_port);
        udp_discovery.print("ESP32_LED_NODE");
        udp_discovery.endPacket();
    }
    
    // 2. Process Incoming Stream Data
    int packet_size = udp_stream.parsePacket();
    if (packet_size > 0) {
        int bytes_read = udp_stream.read(packet_buffer, sizeof(packet_buffer));
        
        // Expected payload is 3 bytes per LED (R, G, B)
        int num_received_leds = bytes_read / 3;
        int leds_to_update = min(num_received_leds, NUM_LEDS);
        
        for (int i = 0; i < leds_to_update; i++) {
            uint8_t r = packet_buffer[i * 3 + 0];
            uint8_t g = packet_buffer[i * 3 + 1];
            uint8_t b = packet_buffer[i * 3 + 2];
            strip.setPixelColor(i, strip.Color(r, g, b));
        }
        
        strip.show();
    }
}
