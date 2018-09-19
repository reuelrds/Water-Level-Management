// Note:  This Sketch needs to be Uploaded to the Esp8266 Module

/*****
  Including Libraries
*****/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ThingSpeak.h>
#include <Ticker.h>


/*******************************************************************
  Declaring and initializing variables & Objects
********************************************************************/

/*****
  Declaring Ticker Object
*****/
Ticker reader;

/*****
  Declaring WiFi ssid and password
*****/
// Note:  This ssid and password are for my phones hotspot.
//        These values will need to be changed when using some other WiFi AP
const char* ssid = "ReuelRds";
const char* password = "reuelrds1234";

/*****
  Variables uesd to connect and upload data to ThingSpeak Cloud
*****/
// Note:  The private key and channel number are related to my ThingSpeak account
//        These will need to be changed when we deploy the prototype model
const char* host = "api.thingspeak.com";
const char* privateKey = "09E2POZ8LM87VIBT";
const int channelNumber = 570672;


/*****
  Variables for receiving data from Arduino
*****/
// str:   used to store the received string
// c:     used to store a single incomming character which is then appended to str
// flag:  used to signify if we have received a complete string or not

String str = "";
char c;
int flag = 0;

/*****
  Flags for Setting up connection
*****/
// isConn:  This flag tells if the Esp has connected to Internet
//          and avoids call connectWlan() method once again.
int isConn = 0;

/*****
  Variable to store data being sent to ThingSpeak
*****/
// val: is used to as a dummy data to be uploaded to ThingSpeak,
//      once connection to Internet has been established.
int val = 0;


/*******************************************************************
  Initial Setup
********************************************************************/

void setup() {
/*****
  Setting up Serial Connection with Arduino
*****/
// Initialize Serial Connection between Arduino and ESP at a baud rate of 9600bps
// Note:  The reason for choosing this particular baud rate was specified in the TimerOne Note in Arduino Sketch
//        And also lower baud rate ensures less errors.
  Serial.begin(9600);
  delay(100);

/*****
  Disconnect from WiFi AP
*****/
// Note:  We Don't need Esp sending data unless Arduino sends it after it knows Esp is Connected to Internet
  WiFi.disconnect();

/*****
  Starting Ticker Object
*****/
// fire readChar function every 1 millisecond to capture character received from Arduino.
// Note:  The reason for choosing this time interval was specified in the TimerOne Note in Arduino Sketch
  reader.attach_ms(1, readChar);
}



/*******************************************************************
  Main Loop
********************************************************************/

void loop() {

  if (flag == 1) {
    reader.detach();
    str.trim();
    if (str == "<ConnectToWlan>") {
      Serial.print("<Connecting>\r");
      delay(500);
    } else if (str == "<ConnectReqAck>") {
      if (isConn != 1) {
        connectWlan();  
      }
    } else {                // At this point the Esp is Connected to Internet and we can start uploading data
      val = str.toInt();
      uploadValues();
    }
    str = "";
    flag = 0;
    reader.attach_ms(1, readChar);
  }
}



/*******************************************************************
  Establish COnnection to Wifi Access Point
********************************************************************/
void connectWlan() {

  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print("\r");

  delay(1000);

  // Set Esp in Station mode (WiFi Client) mode, otherwise, it by default,
  // would try to act as both a client and an access-point and could cause
  // network-issues with your other WiFi-devices on your WiFi-network.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".\r");
    delay(500);
  }

  Serial.print("\r");
  delay(1000);
  Serial.print("WiFi connected\r");
  delay(1000);
  
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP().toString());
  Serial.print("\r");

  delay(1000);
  
  Serial.print("connecting to ");
  Serial.print(host);
  Serial.print("\r");

  delay(1000);
  
  isConn = 1;
  Serial.print("<Connected>");
  Serial.print('\r');
  delay(500);
}

/*******************************************************************
  Uploading values to ThingSpeak Client
********************************************************************/
void uploadValues() {

  // Note:  Need to test if all these statements except 
  //        setField and writeFields can be moved to setup block.
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.print("connection failed\r");
  }

  // Uploading the value received from Arduino
  ThingSpeak.begin(client);
  ThingSpeak.setField(1, val);
  ThingSpeak.writeFields(channelNumber, privateKey);
  Serial.print("Sent val: " + String(val) + "\r");
  delay(1000);
}

/*******************************************************************
  Callback function to read a Character received from Arduino
********************************************************************/
void readChar() {
  if (Serial.available() > 0) {
    c = (char)Serial.read();
    if (c == '\r') {
      flag = 1;
    } else {
      str.concat(c);
    }
  }
}
