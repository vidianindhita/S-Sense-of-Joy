/*
  This example creates a client object that connects and transfers
  data using always SSL.

  It is compatible with the methods normally related to plain
  connections, like client.connect(host, port).

  Written by Arturo Guadalupi
  last revision November 2015

*/

#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFi101.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 7
#define NUMPIXELS 8

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int delayval = 200; // delay for half a second

int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
char server[] = "staging.vidia.site";    // name address for Google (using DNS)

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiSSLClient client;

unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 10L * 50L; // delay between updates, in milliseconds

String deviceStatus;
String deviceMessage;
String lastDeviceStatus = "false";
String lastDeviceMessage = "false";

bool prevState = false;
bool currentState;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  pinMode(A5, OUTPUT);      // set the atomizer pin mode
  digitalWrite(A5, LOW);    // set atomizer to LOW

  pixels.begin();
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");
  printWiFiStatus();
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {
    httpRequest();
  }
}


// this method makes a HTTP connection to the server:
void httpRequest() {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(server, 443)) {
    Serial.println("connecting...");
    // send the HTTP GET request:
    client.println("GET /device-1 HTTP/1.1");
    client.println("Host: staging.vidia.site");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);

  // Parse JSON object
  JsonObject& root = jsonBuffer.parseObject(client);
  if (!root.success()) {
    Serial.println(F("Parsing failed!"));
    return;
  }

  deviceStatus = root["status"].as<char*>();
  deviceMessage = root["message"].as<char*>();

  if (lastDeviceStatus != deviceStatus || lastDeviceMessage != deviceMessage) {
    if (deviceStatus == "true") {
      if (deviceMessage == "false") {
        //turnOnLED();
      } else {
        //turnOnHumidifier();
        httpPutRequest();
      }
    } else {
      //turnOffLED();
    }
  }

  Serial.println(F("Response:"));
  Serial.print("Status: ");
  Serial.print(deviceStatus);
  Serial.print(" ");
  Serial.println(deviceMessage);
  Serial.print("Status Last: ");
  Serial.print(lastDeviceStatus);
  Serial.print(" ");
  Serial.println(lastDeviceMessage);

  lastDeviceStatus = deviceStatus;
  lastDeviceMessage = deviceMessage;
}

void turnOnHumidifier() {
  Serial.println("Humidifier ON");
  digitalWrite(A5, HIGH);
  
  Serial.println("Change LED");
  for(int i = 0; i < NUMPIXELS; i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0,0,150)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

  }
  
  delay(delayval);
}

void turnOnLED() {
  Serial.println("LED ON");
  // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.
  digitalWrite(A5, LOW);
  for(int i=0;i<NUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(100,0,100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

  }
}

void turnOffLED() {
  Serial.println("LED OFF");
  // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.

  digitalWrite(A5, LOW);
  
  for(int i = 0; i < NUMPIXELS; i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(0,0,0)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

  }
}

void httpPutRequest() {
  // send the HTTP PUT request:
  client.stop();
  if (client.connect(server, 443)) {
    // send the HTTP GET request:
    client.println("PUT /device-1/on HTTP/1.1");
    client.println("Host: staging.vidia.site");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
