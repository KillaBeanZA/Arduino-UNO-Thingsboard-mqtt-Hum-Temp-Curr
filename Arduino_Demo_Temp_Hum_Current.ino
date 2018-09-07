
#include <Wire.h>
#include "EmonLib.h"                   // Include Emon Library
#include <math.h>
#include <WiFiEspServer.h>
#include <DHT.h>
#include <DHT_U.h>
#include "DHT.h"
#include <WiFiEspClient.h>
#include <WiFiEsp.h>
#include <WiFiEspUdp.h>
#include <PubSubClient.h>
#include "SoftwareSerial.h"

#define WIFI_AP "yourAP"  //AP SSID
#define WIFI_PASSWORD "yourpassword"  //WIFI Password

#define TOKEN "yourdevicetoken"   //Thingsboard Device Token, Get from Dashboard aka (localhost:8080)

// DHT
#define DHTPIN 4  //Digital pin on Arduino
#define DHTTYPE DHT11  //Temp,Hum sensor type

//Initialize Analog Pin
int analogPin = 0;


//Variable to store the value read
int iRaw = 0;

//Start Emon
EnergyMonitor emon1;
String toPrint1, toPrint2;
double Irms, thePower;
char buffer1[30], buffer2[30];


char thingsboardServer[] = "192.168.1.4";

// Initialize the Ethernet client object
WiFiEspClient espClient;

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

PubSubClient client(espClient);

SoftwareSerial soft(2, 3); // RX, TX

int status = WL_IDLE_STATUS;
unsigned long lastSend;

void setup() {
  // initialize serial for debugging
  Serial.begin(9600);
  dht.begin();
  InitWiFi();
  client.setServer( thingsboardServer, 1883 );
  lastSend = 0;

  //Emon Stuff - Callibrated for 100Amp non invasive current clamp
   emon1.current(0, 60.6);  //pin for current measurement, calibration value
  
}

void loop() {
  status = WiFi.status();
  


  
  if ( status != WL_CONNECTED) {
    while ( status != WL_CONNECTED) {
      Serial.print("Attempting to connect to WPA SSID: ");
      Serial.println(WIFI_AP);
      // Connect to WPA/WPA2 network
      status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      delay(500);
    }
    Serial.println("Connected to AP");
  }

  if ( !client.connected() ) {
    reconnect();
  }

  if ( millis() - lastSend > 10000 ) { // Update and send only after 1 seconds
    getAndSendTemperatureAndHumidityData();
    lastSend = millis();
    
    
  }
 
  client.loop();
}

void getAndSendTemperatureAndHumidityData()
{
  Serial.println("Collecting temperature data.");

      // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
      // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
      
      //Scott edit//
      // Read Analoge Pin A0
      //iRaw = analogRead(0);
      //float i = iRaw;
  
    //Emon stuff
    double Irms = emon1.calcIrms(1480);  // Calculate Irms only
    //Serial.print(Irms*240.0);           // Apparent power
    // Serial.print(" ");
    //Serial.println(Irms);             // Irms
 
          

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  //Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("Watts: ");
  Serial.print(Irms*247.0); // Measured local 220-250vac
  Serial.print(" W ");
  Serial.print("Current: ");
  Serial.print(Irms);
  Serial.print(" A ");

  String temperature = String(t);
  String humidity = String(h);
  String current = String(Irms);
  String watts = String (Irms*247.0); // Measured local 220-250vac

  // Just debug messages
  Serial.print( "Sending temp, humidity, Current and Watts : [" );
  Serial.print( temperature ); Serial.print( "," );
  Serial.print( humidity );Serial.print( "," );
  Serial.print( current );Serial.print( "," );
  Serial.print( watts );
  Serial.print( "]   -> " );

  // Prepare a JSON payload string
  String payload = "{";
  payload += "\"temperature\":"; payload += temperature; payload += ",";
  payload += "\"humidity\":"; payload += humidity; payload += ",";
  payload += "\"Current\":"; payload += current; payload += ",";
  payload += "\"Watts\":"; payload += watts;
  payload += "}";

  // Send payload
  char attributes[100];
  payload.toCharArray( attributes, 100 );
  client.publish( "v1/devices/me/telemetry", attributes );
  Serial.println( attributes );
}

void InitWiFi()
{
  // initialize serial for ESP module
  soft.begin(9600);
  // initialize ESP module
  WiFi.init(&soft);
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(WIFI_AP);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    delay(500);
  }
  Serial.println("Connected to AP");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect("Arduino Uno Device", TOKEN, NULL) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

