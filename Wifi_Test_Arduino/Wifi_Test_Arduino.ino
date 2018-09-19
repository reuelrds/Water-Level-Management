//#include <AltSoftSerial.h>
#include <SoftwareSerial.h>
#include <Ticker.h>
#include <TimerOne.h>

//AltSoftSerial espSerial;
SoftwareSerial espSerial(9,8);

Ticker printer(printString, 500);
Ticker sender(sendData, 20000);
Ticker setupEsp(setupWlan, 5000);
Ticker ackEsp(ackConn, 5000);

String str = "";
char c;
int flag = 0;

int val = 0;

int isConn = 0;
int ack = 0;

unsigned long prevTime = 0;
unsigned long currTime;

void setup() {
  // put your setup code here, to run once:
  Timer1.initialize(1000);
  Timer1.attachInterrupt(readChar);
  delay(50);
  
  Serial.begin(115200);
  espSerial.begin(9600);
  delay(100);

  sender.start();
  sender.pause();
  ackEsp.start();
  ackEsp.pause();
  printer.start();
  setupEsp.start();
  
  
//  espSerial.print("Start Transmit\r");

  
}

void loop() {
  // put your main code here, to run repeatedly:

  if (ack == 0 && isConn == 0) {
    
    setupEsp.update();  
    
  } else if ( ack == 1 && isConn == 0) {
    
    setupEsp.stop();
    if (ackEsp.state() == PAUSED) {
      ackEsp.resume();
    } else {
      ackEsp.update(); 
    } 
    
  } else if (ack == 1 && isConn == 1) {
    ackEsp.stop();
    if (sender.state() == PAUSED) {
      sender.resume();
    } else {
      sender.update();    
    }
  
  }
  printer.update();

  if(ackEsp.counter() == 1) {
    ackEsp.interval(30000);
  }
}

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
    if (str == ".") {
      Serial.print(str);
    } else {
      Serial.println(str);
    }
    
    str = "";
    flag = 0;
    Timer1.restart();
  }
//  Serial.println("ACK: " + String(ack) + " COnn: " + String(isConn));
}

void readChar(){
  if(espSerial.available() > 0) {
    c = (char)espSerial.read();
    
    if (c == '\r') {
      flag = 1;
    } else {
      if (c <= 126 && c >= 32) {
        str.concat(c);
      }
    }
    
  }
}
