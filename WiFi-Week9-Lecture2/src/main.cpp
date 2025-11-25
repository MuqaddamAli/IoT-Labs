// // Connecting to a network //
#include <WiFi.h>

const char* ssid     = "";
const char* password = "";

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);   // Start connection

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Nothing here — stays connected
}

// // Scanning //
// #include <WiFi.h>

// void setup() {
//   Serial.begin(115200);
//   WiFi.mode(WIFI_STA);   // Station mode
//   WiFi.disconnect();     // Not connected to any network
//   delay(1000);
// }

// void loop() {
//   Serial.println("Scanning WiFi networks...");

//   int count = WiFi.scanNetworks();  // Scan request

//   if (count <= 0) {
//     Serial.println("No networks found");
//   } else {
//     Serial.print("Found ");
//     Serial.print(count);
//     Serial.println(" networks:");
//   }

//   for (int i = 0; i < count; i++) {
//     Serial.print(i + 1);
//     Serial.print(": ");
//     Serial.print(WiFi.SSID(i));
//     Serial.print(" (");
//     Serial.print(WiFi.RSSI(i));
//     Serial.println(" dBm)");
//   }

//   Serial.println();
//   delay(5000);   // Scan every 5 seconds
// }