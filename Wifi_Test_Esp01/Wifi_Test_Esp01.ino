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
// This calls an ISR 1 msec to read data from arduino. 
// (See comment for starting Ticker object in setup method)
Ticker reader;

/*****
  Declaring WiFi ssid and password
*****/
// Note:  These values will need to be changed when using some other WiFi AP
const char* ssid = "ReuelRds";
const char* password = "reuelrds1234";

/*****
  Variables uesd to connect and upload data to ThingSpeak Cloud
*****/
// Note:  The private key and channel number are related to the ThingSpeak account
//        These will need to be changed if we use a different thingspeak account
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

/*****
  Variable to store type of connection mode
*****/
// Note:  Type 1 indicates the connection is to be setup using SSID and Password provided in the sketch
//        Type 2 indiactes connection is to be setup using WPS Setup
int type = 0;


/*******************************************************************
  Initial Setup
********************************************************************/

void setup() {
/*****
  Setting up Serial Connection with Arduino
*****/
// Initialize Serial Connection between Arduino and ESP at a baud rate of 9600bps
// Note:  The TimerOne Note in Arduino Sketch specifies the reason for choosing this baud rate. 
//        And also lower baud rate ensures less errors.
  Serial.begin(9600);
  delay(100);

/*****
  Disconnect from WiFi AP
*****/
// Note:  We Don't need Esp sending data unless Arduino sends it.
  WiFi.disconnect();

/*****
  Starting Ticker Object
*****/
// Fire readChar function every 1 millisecond to capture character received from Arduino.
// Note:  The reason for choosing this time interval was specified in the TimerOne Note in Arduino Sketch
  reader.attach_ms(1, readChar);
}



/*******************************************************************
  Main Loop
********************************************************************/

void loop() {

  if (flag == 1) {

    // Stop the ticker and trim the recieved string of any white spaces or carriage returns
    reader.detach();
    str.trim();
    
    if (str == "<ConnectToWlan>" || str == "<ConnectByWPS>") {

      // Check how should we connect to WiFi
      if (str == "<ConnectToWlan>"){
        type = 1;
      } else {
        type = 2;
      }

      // Send Acknowledgement string to Arduino
      Serial.print("<Connecting>\r");
      delay(500);
      
    } else if (str == "<ConnectReqAck>") {

      // Once Arduino responds with acknowledgement, start the process to connect to WiFi
      if (isConn != 1) {
        
        if (type == 1) {
          connectWlan();
        } else {
          connectWPS();
        }
      }
      
    } else {                
      // At this point the Esp is Connected to Internet and we can start uploading data
      if (isConn){
        val = str;
        uploadValues();
      }
    }

    // Clear the string buffer, flag and restart the Ticker to recieve further data strings
    str = "";
    flag = 0;
    reader.attach_ms(1, readChar);
    
  }
}



/*******************************************************************
  Establish Connection to Wifi Access Point Using SSID and Password
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
  Establish Connection to Wifi Access Point using WPS Setup
********************************************************************/

void connectWPS() {
  
  bool success = startWpsSetup();
  if(!success){
    Serial.print("Failed to connect with WPS.");
    Serial.print("Trying with provided SSID and Password\r");
    type = 1;
  } else {
    isConn = 1;
    Serial.print("<Connected>\r");
  }
    
}


bool startWpsSetup() {
  Serial.print("Esp8266: Starting WPS Setup....\r");
  bool wpsSuccess = WiFi.beginWPSConfig();
  if(wpsSuccess) {
      // Well this means not always success :-/ in case of a timeout we have an empty ssid
      String newSSID = WiFi.SSID();
      if(newSSID.length() > 0) {
        // WPSConfig has already connected in STA mode successfully to the new station. 
        Serial.print("Esp8266: WPS finished.\n");
        Serial.print("Eso8266: Connected successfull.\n");
        Serial.print("SSID: ");
        Serial.print(newSSID.c_str());
        Serial.print('\r');
        delay(500);
      } else {
        wpsSuccess = false;
      }
  }
  return wpsSuccess; 
}


/*******************************************************************
  Uploading values to ThingSpeak Client
********************************************************************/
void uploadValues() {

  // TODO:  Need to test if all these statements except 
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

// Note:  This is an ISR called by Ticker for reading characters for Arduino

void readChar() {
  if (Serial.available() > 0) {
    c = (char)Serial.read();
    if (c == '\r') {    // '\r' is being used as a deliminator to siginify end of String
      flag = 1;
    } else {
      str.concat(c);
    }
  }
}
