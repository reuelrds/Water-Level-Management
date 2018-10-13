// Note:  This Sketch is to be uploaded to Arduino board

/*****
  Including Libraries
*****/
#include <SoftwareSerial.h>
#include <Ticker.h>
#include <TimerOne.h>
#include <Button.h>


/*******************************************************************
  Declaring and initializing variables & Objects
********************************************************************/

/*****
  Setting up SoftwareSerial object 
*****/

// RX port as Digital pin 9 and 
// TX port as digital pin 8
SoftwareSerial espSerial(9,8);


/*****
 Initializing Ticker Objects
*****/

// Printer checks every 500milliseconds to see
// if a string was received from ESP Module
Ticker printer(printString, 500);

// sendData sends data to ESP every 20seconds 
// when the connection to Internet is Established
Ticker sender(sendData, 20000);

// Setting up ESP to Connect to a WiFi Access Point
// It fires a connection request string <ConnectToWlan> every 5seconds
// untill the Esp responds with acknowlwdgement string <Connecting>
Ticker setupEsp(setupWlan, 5000);

// Sends a acknowledgement string <ConnectReqAck> to Esp once it receives
// <Connecting> string untill the Esp responds with <Connected>
// whicch signifies that the Esp has connected to Internet successfully
// note: we change the frequency from 5sec to 30sec after sending ack string for first time.
Ticker ackEsp(ackConn, 5000);

// Setting up ESP to Connect to a WiFi Access Point via WPS
// It fires a connection request string <ConnectByWPS> every 5seconds
// untill the Esp responds with acknowlwdgement string <Connecting>
Ticker setupEspWPS(setupWPS, 5000);

// Checks button presses when connection to Wifi Access Point via WPS
Ticker button(buttonCheck, 250);

// Checks the water level every 1 sec 
// And starts the water pump if water level is less than 40%
// And stops the water pump if water level is greater than 60%
Ticker pump(checkPump, 1000);


/*****
 Initializing Button Object
*****/
// Checks button state and
// It takes care of debouncing the button when is pressed
// Note:  The Push Button is connected to Digital Pin 2
Button buttonWPS(2);

/*****
 Declaring Pins for HC-SR04
*****/
int trigPin = 12;
int echoPin = 11;


/*****
 Declaring Signal Pin for relay
*****/
// This controls the relay
int relaySig = 13;

/*****
  Variable to store button count
*****/
// Note:  it is used to turn the button off when pressed twice.
int btnCount = 0;

/*****
  Variables for receiving data from Esp
*****/
// str:   used to store the received string
// c:     used to store a single incomming character which is then appended to str
// flag:  used to signify if we have received a complete string or not
String str = "";
char c;
int flag = 0;

/*****
  Variable to store data being sent to Esp
*****/
// val: is used to as a dummy data to be uploaded to ThingSpeak,
//      once connection to Internet has been established
int val = 0;
float msec;
float distance;


/*****
  Variable is used while calculating initial tank depth
*****/
boolean isBaseDistSet=false;
String distCmd = "";
float initialDist;


/*****
  Flags for Setting up connection
*****/
// isConn:  This flag tells if the Esp has connected to Internet
//          It is set after receiving <Connected> string
// ack:     This flag tells if the Esp has acknowledged the <ConnectToWlan> Flag
//          It is set after receiving <Connecting> string
int isConn = 0;
int ack = 0;


/*******************************************************************
  Initial Setup
********************************************************************/
void setup() {

/*****
  Setting Up ISR for Reading Characters from ESP
*****/

// the relaySig pin controlls the relay. 
// If the Output Signal is LOW, the pump starts
// And if the Output signal is HIGH, the pump stops
  pinMode(relaySig, OUTPUT);

// Calling stopPump to ensure that the Water Pump remains shut while the initial setup takes place
// and to start only if water level is less than 40%, which is handled by pump ticker object (calls checkPump method every 1sec)
  stopPump();

// TimerOne Library uses Timer interrupts to read characters received from Esp
// It interrupts the sketch every 1 millisecond and executes readChar function 
// to read Characters received from esp if there are any

// Note:  The interval of 1 millicscond or 1000 microseconds is choosen because
//        the Arduino and Esp are set to communicate at a baud rate of 9600bps.
//        At this baud rate, a single character (ie. a byte of data. As ASCII characters are represented by 8 bits),
//        requires roughly 833.33 microseconds to be transfered between Esp and Arduino 
//        (assuming that the distance between two is not large)
  Timer1.initialize(1000);
  Timer1.attachInterrupt(readChar);
  delay(50);

/*****
  Setting up Serial Connections
*****/
// Initialize the Serial Monitor at a baud rate of 115200bps 
// to print data to serial monitor as soon as possible.
// Note: Higher baud can also be used
  Serial.begin(115200);

// Initialize Serial Connection between Arduino and ESP at a baud rate of 9600bps
// Note:  The reason for choosing this particular baud rate was specified in the above TimerOne Note.
//        And also lower baud rate ensures less errors.
  espSerial.begin(9600);
  delay(100);

/*****
  Setting up Ultrasonic Sensor
*****/
  // Define input and output pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);


/*****
  Starting Ticker Objects
*****/

// Note:  Since Ticker Library doesn't specify a status Flag when we initialize the object,
//        we cannot call TickerObj.status() when it hasn't been started.
//        Here, Some ticker objects are paused right after they are started because we need
//        them to start/fire only after some event happens.

  sender.start();
  sender.pause();
  ackEsp.start();
  ackEsp.pause();
  printer.start();
  pump.start();
  pump.pause();

// Setting up how Esp connects to WiFi Access Point
  int choice = setupConnType();
  if (choice == 1) {
    Serial.println("Connecting to WiFi AP via SSID and Password....");
    setupEsp.start();
  } else {

/*****
    Initializing Button object
*****/
    // Note:  The Button Object(buttonWPS) and the Ticker Object(button) work together
    //        for setting up the Esp to connect to Wifi AP.
    //        The Wifi AP needs to be in WPS config mode before the ESP can connect.
    //        So, After the initial button press on the Arduino, we can press the WPS button 
    //        on the WIFI AP and then press the one connected to Arduino board to begin setup.
    buttonWPS.begin();
    button.start();
    Serial.println("Press the Button to Connect to WLAN");
  }
}


/*******************************************************************
  Main Loop
********************************************************************/
void loop() {

/*****
  Here we check for events and fire appopriate methods to handle them
*****/

  // Note: The update() method needs to be called every iteration for ticker bojects 
  //        so that they can check their time since last tick(called their callback function).

  if (ack == 0 && isConn == 0) {
    // Run this untill Esp responds with <Connecting>
    
    setupEsp.update();
    setupEspWPS.update(); 
  } else if ( ack == 1 && isConn == 0) {
    
    // Stop setupEsp & setupEspWPS ticker Object once the Esp responds with <Connecting> string
    // and start sending acknowledgement
    setupEsp.stop();
    setupEspWPS.stop();      
                      
    if (ackEsp.state() == PAUSED) {
      ackEsp.resume();
    } else {
      ackEsp.update(); 
    } 
    
  } else if (ack == 1 && isConn == 1) {

    // Stop ackEsp ticker Object once the Esp responds with <Connected> string
    // and start sending dummy Data
    ackEsp.stop();

    // Checks if initial depth is set. If not it requests it from user to send <CaliberateSR04>
    
    if (!isBaseDistSet){
      Serial.println("\n Initial Base Distance or Tank Depth is not Set.");
      Serial.println("\nSend <CaliberateSR04> command to the Arduino to set the initial distance.");
      distCmd="";
      int sf=0;
      while(sf == 0) {
        if (Serial.available() > 0){
          distCmd += (char)Serial.read();
        }
        distCmd.trim();
        if (distCmd == "<CaliberateSR04>") {
          sf = 1;
        }
      }

      // Received <CaliberateSR04> command. Call calcDistance to calculate intital distance
      isBaseDistSet = true;
      Serial.println("Calculating initial depth");
      initialDist = calcDistance();

      // Initial distance is set.
      Serial.println("Initial depth: " + String(initialDist));
      Serial.println("\n\nUploading Data");
    } else {
      // As Initial distance is set, now start mointoring the water level and sending data to Thingspeak
        if (sender.state() == PAUSED) {
        sender.resume();
        pump.resume();
      } else {
        sender.update();
        pump.update();
      }
    }
  }

  printer.update();
  button.update();
  
  // Change interval of sending ack String to Esp
  // to 30 sec as connecting to wifi may take some time.
  if(ackEsp.counter() == 1) {
    ackEsp.interval(30000);
  }
}


/*******************************************************************
  Setup Wifi Config
********************************************************************/
int setupConnType(){
  int ch = 0;
  Serial.println("Setting up WLAN Commection\n");
  Serial.println("1. Connect via SSID and Password provided\n2.Connect via WPS");
  Serial.println("Choose a Connection Option: ");
  
  while(ch == 0){
    if(Serial.available()){
      ch = (int)Serial.read()-48;
    }
  }
  Serial.println("Choice: ");
  Serial.print(ch);
  Serial.println("\n");
  return ch;
}



/*******************************************************************
  Calculate Depth of Water
********************************************************************/
float calcDistance() {

 // We send a LOW signal to ensure that the sensor is deactivated before it activates
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Ultrasonic sensor needs s HIGH Signal for 10 microseconds to activate
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read input from echo pin
  // the msec holds the time required for the sound waves to go and come back
  msec = pulseIn(echoPin, HIGH);

  // Calculate distance from time
  distance = 0.034837*(msec/2.0);
  Serial.println("\nDIST: " + String(distance));

  return distance;
}

/*******************************************************************
  Callback Functions
********************************************************************/

// Note: the explaination to what these functions do is given right at the top where the Ticker Objects are declared

void setupWPS(){
  espSerial.print("<ConnectByWPS>\r");
  Serial.print("Sent wps req...");
}

void setupWlan() {
  espSerial.print("<ConnectToWlan>\r");
  Serial.print("Sent req...");
}

void ackConn() {
  espSerial.print("<ConnectReqAck>\r");
}


void sendData() {
  float percent = ((initialDist - calcDistance()) / initialDist)*100;
  espSerial.print(String(percent) + "\r");
}

void checkPump() {
  float dist = calcDistance();
  float percent = ((initialDist - dist) / initialDist)*100;
  Serial.println("Percent: " + String(percent));
  if (percent > 60.0) {
    stopPump();
  } else if (percent < 40.0) {
    startPump();
  }
}

void startPump() {
  Serial.println("Pump Started");
  digitalWrite(relaySig, LOW);
}

void stopPump() {
  digitalWrite(relaySig, HIGH);
  Serial.println("Pump Stoped");
}

void printString() {
  if(flag == 1){
    Timer1.stop();
    str.trim();
    if(str == "<Connecting>") {
      ack = 1;
    } 
    
    if (str == "<Connected>") {
      ackEsp.stop();
      isConn = 1;
    }
    if (str == ".") {       // Special condition for while waiting to get IP adderss from router
      Serial.print(str);
    } else {
      Serial.println(str);
    }
    
    str = "";
    flag = 0;
    Timer1.restart();
  }
}

void buttonCheck() {
    
  if (buttonWPS.toggled() && buttonWPS.read() == Button::RELEASED) {
    btnCount++;
    if (btnCount == 1) {
      Serial.println("\nPress the WPS Button on the WiFi Access Point. And then,");
      Serial.println("Press the Button connected to Arduino once again to start the setup");
    } else if (btnCount == 2) {
      Serial.println("\nStarting Setup.....");
      setupEspWPS.start();
      button.stop();
    }
  }
}


void readChar(){

  // Note: This is an ISR (Interrupt Service Routine) which gets Called every time TimerOne interrupts the Microprocessor
  
  if(espSerial.available() > 0) {
    c = (char)espSerial.read();
    
    if (c == '\r') {                  // '\r' is being used as a deliminator to siginify end of String
      flag = 1;
    } else {
      if (c <= 126 && c >= 32 || c == '\n') {      // Helps to discard any garbled data.
        str.concat(c);
      }
    }
    
  }
}
