#include <WiFi.h>

// -------- WiFi credentials --------
const char* ssid     = "Pixel :3";
const char* password = "6fm82ifndqs22ck";

// -------- Static IP configuration --------
IPAddress local_IP(10, 141, 172, 100); // Static IP
IPAddress gateway(10, 141, 172, 140);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);        // Optional
IPAddress secondaryDNS(8, 8, 4, 4);      // Optional

// -------- Web server & LED pin --------
WiFiServer server(80);       // HTTP server on port 80
const int LED_PIN = 2;       // Change if your LED is on another pin

void setup() {
  Serial.begin(115200);

  // LED setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Set Static IP (optional, but you wanted it)
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Static IP Failed!");
  }

  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  // Start web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;  // No client, exit loop()

  Serial.println("New Client connected");

  String requestLine = "";
  // Read only the first request line (e.g. "GET /LED=ON HTTP/1.1")
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (c == '\r') continue;   // ignore carriage return
      if (c == '\n') break;      // end of first line
      requestLine += c;
    }
  }
  Serial.println(requestLine);

  // ----- LED CONTROL -----
  if (requestLine.indexOf("GET /LED=ON") != -1) {
    digitalWrite(LED_PIN, HIGH);
  } else if (requestLine.indexOf("GET /LED=OFF") != -1) {
    digitalWrite(LED_PIN, LOW);
  }

  // ----- RESPONSE PAGE -----
  String htmlPage =
    "<!DOCTYPE html><html>"
    "<head><meta charset='UTF-8'><title>ESP32 LED Control</title></head>"
    "<body>"
    "<h1>ESP32 LED Control</h1>"
    "<p><a href=\"/LED=ON\"><button>LED ON</button></a></p>"
    "<p><a href=\"/LED=OFF\"><button>LED OFF</button></a></p>"
    "</body></html>";

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println(htmlPage);

  delay(1);
  client.stop();
  Serial.println("Client disconnected");
}