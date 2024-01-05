/***
Program: Termostat za sušilnco
Author: Denis Leskovar
Date: 26.12.2023
***/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>;

//Constants
#define DHTPIN 6           // what pin we're connected to
#define DHTTYPE DHT22      // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);  //// Initialize DHT sensor for normal 16mhz Arduino

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels


// Data wire is conntec to the Arduino digital pin 4
#define ONE_WIRE_BUS 4
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin defines
#define relay1 7
#define relay2 8
#define relay1OFF 1
#define relay1ON 0
#define relay2OFF 1
#define relay1ON 0
#define ledStatus 9
#define ledRelay1 3
#define ledRelay2 2
#define potRead1 A0
#define potRead2 A1


// Global vars

int lowTemp = 22;
int highTemp = 34;
int errTemp = -25;
char tempCstr[4];
String tempStr;
String tempHumStr;
String lowTempStr;
String highTempStr;
float tempC;
int chk;
float hum;   //Stores humidity value
float temp;  //Stores temperature value

/*
    1 - Ascending (needs to heat the room)
    2 - Descending (needs to shutdown heating)
    3 - Nothing (we just wait, everything is perfect)
*/

int iterCounter = 0;
int iterMax = 20;
int tempStatus = 3;  // Selects in which mode it is
// GPIO's

int relay1INT = 0;
int relay2INT = 0;
int ledStatusINT = 0;
int ledRelay1INT = 0;

// Function defines
void helloOled(void);
void writeOledStatus(String relay1Status, String relay2Status);
void writeDataOled(String tempC, String hum, String lowTempStr, String highTempStr);
void setGPIO();
void setInitial();
void setOperation();
void readMinMaxTemp();
void serialPrintData();
void readSensors();

void setup(void) {
  // Start serial communication for debugging purposes
  Serial.begin(9600);

  // Start up the library
  sensors.begin();
  Serial.begin(9600);
  dht.begin();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  helloOled();
  setInitial();
  setGPIO();
}

void loop(void) {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  if (iterCounter >= iterMax) {
    readSensors();
    iterCounter = 0;
  }

  iterCounter += 1;
  setOperation();
  setGPIO();
  readMinMaxTemp();
  // Convert for OLED
  tempStr = String(tempC, 0);
  tempHumStr = String(hum, 0);
  lowTempStr = String(lowTemp);
  highTempStr = String(highTemp);
  writeDataOled(tempStr, tempHumStr, lowTempStr, highTempStr);
  serialPrintData();

  delay(100);
}

// OPERATIONS DECIDER
void setOperation() {

  if ((tempC > highTemp)) {
    relay1INT = relay1OFF;
    relay2INT = relay2OFF;
    tempStatus = 1;
  }
  /* check the boolean condition */
  else if (((tempC < highTemp) && (tempC > lowTemp)) && tempStatus != 2) {
    relay1INT = relay1OFF;
    relay2INT = relay2OFF;
    tempStatus = 1;
  } else if (((tempC < highTemp) && (tempC > lowTemp)) && tempStatus == 2) {
    relay1INT = relay1ON;
    relay2INT = relay2OFF;
    tempStatus = 2;
  } else if ((tempC < lowTemp) && tempC > errTemp) {
    relay1INT = relay1ON;
    relay2INT = relay2OFF;
    tempStatus = 2;
  }


  ledStatusINT = digitalRead(relay1);
}


void setInitial() {

  // Init Pins
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(ledRelay1, OUTPUT);
  pinMode(ledRelay2, OUTPUT);
  // pinMode(ledStatus, OUTPUT);


  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);
  hum = dht.readHumidity();
  relay1INT = 1;
  relay2INT = 1;
  ledStatusINT = 0;

  if (tempC > highTemp) {
    relay1INT = relay1OFF;
    relay2INT = relay2OFF;
    ledStatusINT = 0;
    tempStatus = 3;
  }
  /* check the boolean condition */
  else if ((tempC < highTemp) && (tempC > lowTemp)) {
    relay1INT = relay1OFF;
    relay2INT = relay2OFF;
    ledStatusINT = 0;
    tempStatus = 3;
  } else
    relay1INT = relay1ON;
  relay2INT = relay2OFF;
  ledStatusINT = 1;
  tempStatus = 1;
}

void readMinMaxTemp() {

  int lowPot = analogRead(A0);
  lowPot = lowPot * (30.0 / 1023.0);

  int highPot = analogRead(A1);
  highPot = highPot * (30.0 / 1023.0);

  if (lowPot >= highPot) {
    highPot = lowPot + 3;
  }

  lowTemp = lowPot;
  highTemp = highPot;
}

void writeDataOled(String tempCOled, String humOled, String lowTempStr, String highTempStr) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("T:");
  display.print(tempCOled);
  display.setCursor(0, 16);
  display.print("V:");
  display.print(humOled);
  display.println("%");

  // SET text
  String relay1txt = "OFF";
  String relay2txt = "OFF";
  if (relay1INT == 0) {
    relay1txt = "@";
  } else {
    relay1txt = "O";
  }
  if (relay2INT == 0) {
    relay2txt = "@";
  } else {
    relay2txt = "O";
  }

  // High low temps
  // char lowTempChar[2] = lowTempStr;
  display.setCursor(54, 0);
  // display.print("L:");
  display.print(lowTempStr);
  display.setCursor(82, 0);
  display.print("-");
  // display.print(" H:");
  display.setCursor( 96, 0);
  display.print(highTempStr);

  // // RElay status
  display.setCursor(80, 16);
  display.print("");
  display.print(relay1txt);
  display.setCursor(100, 16);
  display.print(" ");
  display.print(relay2txt);

  display.display();
}

void setGPIO() {
  digitalWrite(relay1, relay1INT);
  digitalWrite(relay2, relay2INT);
  //digitalWrite(ledStatus, digitalRead(relay1));
  digitalWrite(ledRelay1, !digitalRead(relay1));
  digitalWrite(ledRelay2, !digitalRead(relay2));
}

void readSensors() {
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);
  hum = dht.readHumidity();
}

void serialPrintData() {
  Serial.print("Low Temp :");
  Serial.print(lowTemp);
  Serial.print("    |     High Temp: ");
  Serial.println(highTemp);
  Serial.print("Temp: ");
  Serial.print(tempC);
  Serial.print("    |     Hum: ");
  Serial.println(hum);
}

void helloOled(void) {

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  // Clear the buffer
  display.clearDisplay();
  display.setCursor(0, 8);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("KLOBASAR");
  display.println("3000 TDI");
  display.display();
  delay(300);
  display.clearDisplay();
}
