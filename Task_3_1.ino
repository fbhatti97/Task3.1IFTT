#include <WiFiNINA.h>
#include <Wire.h>
#include "secrets.h" // Make sure this file has your WiFi credentials

// WiFi credentials
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASSWORD;

WiFiClient client;
char HOST_NAME[] = "maker.ifttt.com";
String YOUR_IFTTT_KEY = "dGWDPYeZ0gcTCz-vetzuuP"; // Replace with your IFTTT key

// BH1750 sensor address
const int BH1750_address = 0x23;

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Initialize I2C for the BH1750 sensor
  WiFi.begin(ssid, pass); // Connect to WiFi

  // Check for WiFi connection
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Initialize BH1750 sensor
  Wire.beginTransmission(BH1750_address);
  Wire.write(0x01); // Power on
  Wire.endTransmission();
  Wire.beginTransmission(BH1750_address);
  Wire.write(0x10); // Continuous high resolution mode
  Wire.endTransmission();
}

void loop() {
  int lightLevel = readLightSensor(); // Get the current light level from the sensor
  
  if (lightLevel > 300) {
    // If light level is above 300 lux, trigger the "sunlight_start" event
    triggerIFTTT("sunlight_start", lightLevel);
  } else if (lightLevel < 30) {
    // If light level is below 30 lux, trigger the "sunlight_end" event
    triggerIFTTT("sunlight_end", lightLevel);
  }

  delay(10000); // Delay to prevent constant triggering/checking (adjust as needed)
}

int readLightSensor() {
  Wire.beginTransmission(BH1750_address);
  Wire.requestFrom(BH1750_address, 2);
  if (Wire.available() == 2) {
    int reading = Wire.read() << 8 | Wire.read();
    return reading / 1.2; // Convert to lux
  }
  return -1; // Error reading sensor
}

void triggerIFTTT(String eventName, int lightLevel) {
  if (client.connect(HOST_NAME, 80)) {
    // Construct the query string with the event name and light level
    String queryString = "/trigger/" + eventName + "/with/key/" + YOUR_IFTTT_KEY 
                         + "?value1=" + String(lightLevel);
    // Make the HTTP request to IFTTT
    client.println("GET " + queryString + " HTTP/1.1");
    client.println("Host: " + String(HOST_NAME));
    client.println("Connection: close");
    client.println(); // End HTTP header
    
    while (client.connected() || client.available()) {
      char c = client.read();
      Serial.write(c);
    }
    client.stop(); // Disconnect after sending the request
    Serial.println("IFTTT event '" + eventName + "' triggered with light level: " + String(lightLevel));
  } else {
    Serial.println("Failed to connect to IFTTT");
  }
}
