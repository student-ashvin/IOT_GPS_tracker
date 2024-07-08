#include <WiFi.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// WiFi credentials
const char* ssid = "POCO";
const char* password = "123456789";

// Initialize web server on port 80
WiFiServer server(80);

// GPS module pins and serial
const int RXPin = 16;  // Connect GPS TX to ESP32 RXPin
const int TXPin = 17;  // Connect GPS RX to ESP32 TXPin
HardwareSerial neo6m(1); // Use hardware serial 1 for GPS
TinyGPSPlus gps;

void setup() {
  Serial.begin(115200);
  neo6m.begin(9600, SERIAL_8N1, RXPin, TXPin); // Initialize GPS module serial

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();  // Listen for incoming clients

  if (client) {  // If a new client connects,
    Serial.println("New Client.");  // Print a message out in the serial port
    String currentLine = "";  // Make a String to hold incoming data from the client
    while (client.connected()) {  // Loop while the client's connected
      if (client.available()) {  // If there's bytes to read from the client,
        char c = client.read();  // Read a byte, then
        Serial.write(c);  // Echo it out to the serial monitor
        if (c == '\n') {  // If the byte is a newline character
          // If the current line is blank, you got two newline characters in a row.
          // That's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // Send HTTP headers
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // GPS data update
            smartdelay_gps(1000);
            
            // Check if we have valid GPS data before sending HTML
            if (gps.location.isValid()) {
              // Add your HTML content here
              client.println("<!DOCTYPE HTML>");
              client.println("<html>");
              client.print("<head><title>ESP32 GPS Location</title>");
              // Include the Google Maps JavaScript API
              client.print("<script src='https://maps.googleapis.com/maps/api/js?key=YOUR_API_KEY'></script>");
              client.print("<script>");
              client.print("function initMap() {");
              client.print("var location = {lat: ");
              client.print(gps.location.lat(), 6);
              client.print(", lng: ");
              client.print(gps.location.lng(), 6);
              client.println("};");
              client.println("var map = new google.maps.Map(document.getElementById('map'), {zoom: 15, center: location});");
              client.println("var marker = new google.maps.Marker({position: location, map: map});");
              client.println("}");
              client.println("</script>");
              client.println("</head>");
              client.println("<body onload='initMap()'>");
              client.println("<h1>GPS Location</h1>");
              client.println("<div id='map' style='width:100%;height:400px;'></div>");
              client.println("</body></html>");
            } else {
              // GPS data not valid, send simple page
              client.println("<!DOCTYPE HTML>");
              client.println("<html>");
              client.println("GPS signal not found.");
              client.println("</html>");
            }

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else {  // If you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // If you got anything else but a carriage return character,
          currentLine += c;  // Add it to the end of the currentLine
        }
      }
    }
    // Clear the client's request and close the connection
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

// Add the smartdelay_gps function here as it was in your original code
static void smartdelay_gps(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (neo6m.available()) {
      gps.encode(neo6m.read());
    }
  } while (millis() - start < ms);
}
