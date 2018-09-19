// Note:  This Sketch is to be uploaded to Arduino board

/*****
  Including Libraries
*****/
#include <SoftwareSerial.h>
#include <Ticker.h>
#include <TimerOne.h>


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

// TimerOne Library uses Timer interrupts to read characters received from Esp
// It interrupts the sketch every 1millisecond and executes readChar function 
// to read Characters received from esp if there are any

// Note:  The interval of 1 millicscond or 1000 microseconds is choosen because
//        the Arduino and Esp communicate at a baud rate of 9600bps.
//        At this baud rate, a single character (ie. a byte of data. As ASCII characters are represented by 8 bits),
//        requires roughly 833.33 microseconds to be transfered between Esp and Arduino
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
  setupEsp.start();
}


/*******************************************************************
  Main Loop
********************************************************************/
void loop() {

/*****
  Here we check for events and fire appopriate methods to handle them
*****/

  if (ack == 0 && isConn == 0) {
    setupEsp.update();  
  } else if ( ack == 1 && isConn == 0) {
    
    // Stop SetupEsp ticker Object once the Esp responds with <Connecting> string
    // and start sending acknowledgement
    setupEsp.stop();                        
    if (ackEsp.state() == PAUSED) {
      ackEsp.resume();
    } else {
      ackEsp.update(); 
    } 
    
  } else if (ack == 1 && isConn == 1) {

    // Stop ackEsp ticker Object once the Esp responds with <Connected> string
    // and start sending dummy Data
    ackEsp.stop();
    if (sender.state() == PAUSED) {
      sender.resume();
    } else {
      sender.update();    
    }
    
  }

  printer.update();

  // Change interval of sending ack String to Esp
  // to 30 sec as connecting to wifi may take some time.
  if(ackEsp.counter() == 1) {
    ackEsp.interval(30000);
  }
}

/*******************************************************************
  Callback Functions
********************************************************************/


void setupWlan() {
  espSerial.print("<ConnectToWlan>\r");
  Serial.print("Sent req...");
}

void ackConn() {
  espSerial.print("<ConnectReqAck>\r");
}


void sendData() {
  espSerial.print(String(val) + "\r");
  val++;
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

void readChar(){
  if(espSerial.available() > 0) {
    c = (char)espSerial.read();
    
    if (c == '\r') {                  // '\r' is being used as a deliminator to siginify end of String
      flag = 1;
    } else {
      if (c <= 126 && c >= 32) {      // Helps to discard any garbled data.
        str.concat(c);
      }
    }
    
  }
}
