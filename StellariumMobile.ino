#include <WiFi.h>
#include <WiFiClient.h>

WiFiServer server(10001);  // Default LX200 Port (23) 10001?
String incoming = "";
double RA, Dec;
String localTime;
// String decimalToRAString(float raDecimal);
// String decimalToDecString(float decDecimal);
// float raStringToDecimal(String raString);
// float decStringToDecimal(String decString);

void setup() {
  // Start the serial connection for debugging
  Serial.begin(9600);  
  RA = 0;
  Dec = 1;
  localTime = "";
  // Set up WiFi in Access Point mode
  WiFi.softAP("ESP32_Telescope", "");
  Serial.print("Access Point started. IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("ESP32 in AP mode. Waiting for client to connect...");

  // Start server
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
 
  if (client) {
    Serial.print("Access Point started. IP: ");
    Serial.println(WiFi.softAPIP());
    
    while (client.connected()) {
      while (client.available()) {
        char c = client.read();
        incoming += c;  // gather all characters from a single command
      }
      if (incoming != "") {
        Serial.println(incoming);  // Print the entire string for debugging
      }
      // Check for Echo request from Stellarium. Respond with "A" if the command is #[6]
      if (incoming.length() == 2) {
        char firstChar = incoming.charAt(0);  // Get the first character
        char secondChar = incoming.charAt(1);  // Get the second character
        if ((firstChar == '#') && (secondChar != '#')) {// Check if the first character is '#' and the second one is not '#'
          client.print("A"); // Connect to the ESP32! Phew that sucked. No documentation.
        }
      }
      if (incoming=="#:GR#"){
        // String ra = "+12:30.0"; //Return Right Ascension
        // client.print(ra + "#");
        client.print(decimalToRAString(RA));
      }
      if (incoming=="#:GD#"){

        client.print(decimalToDecString(Dec));
      }
      if (incoming==":D#"){
        client.print("0"); // Return Slew Status
      }
      if (incoming==":GVP#"){
        client.print("Shawn's Tracking#"); //Return Product ID
      }
      if (incoming==":GVN#"){
        client.print("Version 1#"); //Return Version Number
      }
      if (incoming.startsWith("#:Sr")){ // Set RA to Stellarium Value
        String raTime = incoming.substring(4, incoming.length() - 1);
        Serial.println(raTime);
        RA = raStringToDecimal(raTime);
        Serial.println("setting RA to: " + String(RA));
        client.print("1");
      }

      if (incoming.startsWith("#:Sd")){ // Set Dec to Stellarium Value
        String decTime = incoming.substring(4, incoming.length() - 1);
        Dec = decStringToDecimal(decTime);
        Serial.println("setting Dec to: " + String(Dec));
        client.print("1");
      }
      if (incoming.startsWith("#:SG")){ // Sync Hours from UTC (time zone)
        client.print("1");
      }
      if (incoming.startsWith("#:SL")){ // Sync Local Time
        localTime = incoming.substring(4, incoming.length() - 1);
        client.print("1");
        
      }
      if (incoming=="#:GG#"){ //get hours from UTC
        client.print("+07#");
      }
      if (incoming.startsWith("#:St")){ // Set Latitude
        client.print("1");
      }
      if (incoming.startsWith("#:SC")){ // Sync Calendar
        client.print("1");
        client.print("Updating planetary data#");
        client.print(String(' ', 30) + "#");
      }
      if (incoming.startsWith("#:GC")){ // Get Calendar Date
        client.print("04/05/25#");
      }
      if (incoming.startsWith("#:GL")){ // Get Local Time
        client.print(localTime +"#");
      }

      incoming = "";
    }
  }
}

// HELPER FUNCTIONS 

// DECIMAL TO LX200 FORMAT
String decimalToDecString(float decDecimal) {
  int sign = (decDecimal < 0) ? -1 : 1;  // Determine if positive or negative
  decDecimal = abs(decDecimal);  // Make the decimal value positive for calculation
  
  int degrees = (int)decDecimal;         // Degrees is the whole number part
  float remaining = decDecimal - degrees; // Get the fractional part
  int minutes = (int)(remaining * 60);   // Multiply by 60 to get minutes, and cast to int to remove fractions
  float seconds = (remaining * 3600) - (minutes * 60); // Multiply remaining by 3600 (60 * 60) to get total seconds

  // Format to DD:MM:SS
  char decString[20];
  snprintf(decString, sizeof(decString), "%02d*%02d", degrees, minutes);
  String response = String(sign == -1 ? "-" : "+") + decString + "#";  // Format for LX200
  return response;
}

String decimalToRAString(float raDecimal) {
  // Convert decimal degrees to hours
  int hours = raDecimal / 15;  // 1 hour = 15 degrees
  float remainingDegrees = raDecimal - (hours * 15);  // Calculate remaining degrees after hours
  int minutes = remainingDegrees * 60 / 15;  // Convert remaining degrees to minutes
  float remainingMinutes = remainingDegrees * 60 - minutes * 15;  // Calculate remaining minutes
  int seconds = remainingMinutes * 60 / 15;  // Convert remaining minutes to seconds

  // Format to HH:MM:SS
  char raString[20];
  snprintf(raString, sizeof(raString), "%02d:%02d:%02d", hours, minutes, seconds);
  return String(raString) + "#";  // Format for LX200
}

// LX200 FORMAT TO DECIMAL
float raStringToDecimal(String raString) {
  // Remove the #:Sr prefix and # suffix
  // raString = raString.substring(4, raString.length() - 1);
  
  // Split the string into hours, minutes, and seconds
  int hours = raString.substring(0, 2).toInt();
  int minutes = raString.substring(3, 5).toInt();
  int seconds = raString.substring(6, 8).toInt();

  // Convert to decimal degrees
  float raDecimal = (hours + (minutes / 60.0) + (seconds / 3600.0)) * 15.0;
  return raDecimal;
}

float decStringToDecimal(String decString) {
  // Remove the #:Sd prefix and # suffix
  // decString = decString.substring(4, decString.length() - 1);
  
  // Split the string into degrees, minutes, and seconds
  int degrees = decString.substring(0, 3).toInt();
  int minutes = decString.substring(4, 6).toInt();
  int seconds = decString.substring(7, 9).toInt();

  // Determine sign from the prefix (e.g. "-")
  int sign = (decString[0] == '-') ? -1 : 1;
  
  // Convert to decimal degrees
  float decDecimal = (degrees + (minutes / 60.0) + (seconds / 3600.0)) * sign;
  return decDecimal;
}
