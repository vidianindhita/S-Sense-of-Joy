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
#include <WiFiUdp.h>
#include <RTCZero.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN 6
#define NUMPIXELS 20

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

RTCZero rtc;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int delayval = 500; // delay for half a second

const int GMT = 0; //change this to adapt it to your time zone

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
String lastDeviceStatus = "false";

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  pinMode(A5, OUTPUT);      // set the atomizer pin mode
  digitalWrite(A5, LOW);    // set atomizer to LOW

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
  rtc.begin();
  pixels.begin();
//  ledClockOff();
  unsigned long epoch;
  int numberOfTries = 0, maxTries = 6;
  do {
    epoch = WiFi.getTime();
    numberOfTries++;
  }
  while ((epoch == 0) && (numberOfTries < maxTries));

  if (numberOfTries > maxTries) {
    Serial.print("NTP unreachable!!");
    while (1);
  }
  else {
    Serial.print("Epoch received: ");
    Serial.println(epoch);
    rtc.setEpoch(epoch);

    Serial.println();
  }
}

void loop() {
  //  printTime();
  //    ledClock();
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
    client.println("GET /device-2 HTTP/1.1");
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

  //  if (lastDeviceStatus != deviceStatus) {
  //    if (deviceStatus == "true") {
  //      Serial.println("ON");
  //      printTime();
  //      // turn on device
  //      //      rtc.enableAlarm(rtc.MATCH_SS);
  //      //      rtc.attachInterrupt(ledClock);
  //      //      printTime();
  ////      ledClock();
  //    } else {
  //      Serial.println("OFF");
  //      // turn off device
  //      //      rtc.disableAlarm();
  //      ledClockOff();
  //    }
  //  }

  if (deviceStatus == "true") {
    printTime();
    Serial.println("ON");
//    ledClockOn();
  } else {
//    ledClockOff();
    Serial.println("OFF");
  }

  Serial.println(F("Response:"));
  Serial.print("Status: ");
  Serial.println(deviceStatus);
  Serial.print("Status Last: ");
  Serial.println(lastDeviceStatus);

  lastDeviceStatus = deviceStatus;
}

void ledClock() {
  if (rtc.getSeconds() == 0 || rtc.getSeconds() == 10 || rtc.getSeconds() == 20 || rtc.getSeconds() == 30 || rtc.getSeconds() == 40 || rtc.getSeconds() == 50) {
    diffuseScentMorning();
    pixels.setPixelColor(18, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(19, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(0, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(1, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(2, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(3, pixels.Color(100, 0, 100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
  if (rtc.getSeconds() == 1 || rtc.getSeconds() == 11 || rtc.getSeconds() == 21 || rtc.getSeconds() == 31 || rtc.getSeconds() == 41 || rtc.getSeconds() == 51) {
    pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(1, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(2, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(3, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(4, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(5, pixels.Color(100, 0, 100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
  if (rtc.getSeconds() == 2 || rtc.getSeconds() == 12 || rtc.getSeconds() == 22 || rtc.getSeconds() == 32 || rtc.getSeconds() == 42 || rtc.getSeconds() == 52) {
    pixels.setPixelColor(2, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(3, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(4, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(5, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(6, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(7, pixels.Color(100, 0, 100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
  if (rtc.getSeconds() == 3 || rtc.getSeconds() == 13 || rtc.getSeconds() == 23 || rtc.getSeconds() == 33 || rtc.getSeconds() == 43 || rtc.getSeconds() == 53) {
    pixels.setPixelColor(4, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(5, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(6, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(7, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(8, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(9, pixels.Color(100, 0, 100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
  if (rtc.getSeconds() == 4 || rtc.getSeconds() == 14 || rtc.getSeconds() == 24 || rtc.getSeconds() == 34 || rtc.getSeconds() == 44 || rtc.getSeconds() == 54) {
    pixels.setPixelColor(6, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(7, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(8, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(9, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(10, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(11, pixels.Color(100, 0, 100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
  if (rtc.getSeconds() == 5 || rtc.getSeconds() == 15 || rtc.getSeconds() == 25 || rtc.getSeconds() == 35 || rtc.getSeconds() == 45 || rtc.getSeconds() == 55) {
    diffuseScentMorning();
    pixels.setPixelColor(8, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(9, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(10, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(11, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(12, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(13, pixels.Color(100, 0, 100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
  if (rtc.getSeconds() == 6 || rtc.getSeconds() == 16 || rtc.getSeconds() == 26 || rtc.getSeconds() == 36 || rtc.getSeconds() == 46 || rtc.getSeconds() == 56) {
    pixels.setPixelColor(10, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(11, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(12, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(13, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(14, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(15, pixels.Color(100, 0, 100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
  if (rtc.getSeconds() == 7 || rtc.getSeconds() == 17 || rtc.getSeconds() == 27 || rtc.getSeconds() == 37 || rtc.getSeconds() == 47 || rtc.getSeconds() == 57) {
    pixels.setPixelColor(12, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(13, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(14, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(15, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(16, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(17, pixels.Color(100, 0, 100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
  if (rtc.getSeconds() == 8 || rtc.getSeconds() == 18 || rtc.getSeconds() == 28 || rtc.getSeconds() == 38 || rtc.getSeconds() == 48 || rtc.getSeconds() == 58) {
    pixels.setPixelColor(14, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(15, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(16, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(17, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(18, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(19, pixels.Color(100, 0, 100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
  if (rtc.getSeconds() == 9 || rtc.getSeconds() == 19 || rtc.getSeconds() == 29 || rtc.getSeconds() == 39 || rtc.getSeconds() == 49 || rtc.getSeconds() == 59) {
    pixels.setPixelColor(16, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(17, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.setPixelColor(18, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(19, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(0, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.setPixelColor(1, pixels.Color(100, 0, 100)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.
  }
}

void ledClockOff() {
  Serial.println("OFF");
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0)); // Moderately bright green color.
    pixels.show(); // This sends the updated pixel color to the hardware.
  }
}

void ledClockOn() {
  Serial.println("ON");
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(100, 0, 100)); // Moderately bright green color.
    pixels.show(); // This sends the updated pixel color to the hardware.
  }
}

void printTime()
{
  print2digits(rtc.getHours() + GMT);
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
  Serial.println();
}

void printDate()
{
  Serial.print(rtc.getDay());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.print(rtc.getYear());

  Serial.print(" ");
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

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}

void diffuseScentMorning() {
  digitalWrite(A5, HIGH);
  delay(500); // this makes the led stop for 1 sec
  digitalWrite(A5, LOW);
  Serial.println("SPRAY");
}
