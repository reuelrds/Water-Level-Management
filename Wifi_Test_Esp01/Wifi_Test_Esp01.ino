#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ThingSpeak.h>
#include <Ticker.h>

Ticker reader;


const char* ssid = "ReuelRds";
const char* password = "reuelrds1234";

const char* host = "api.thingspeak.com";
const char* privateKey = "09E2POZ8LM87VIBT";
const int channelNumber = 570672;

int flag = 0;
String str = "";
char c;

int isConn = 0;

int val = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(100);

  WiFi.disconnect();

  reader.attach_ms(1, readChar);
}

void loop() {
  // put your main code here, to run repeatedly:

  if (flag == 1) {
    reader.detach();
    str.trim();
    if (str == "<ConnectToWlan>") {
      Serial.print("<Connecting>\r");
      delay(500);
    } else if (str == "<ConnectReqAck>") {
      if (isConn == 1) {
//        Serial.print("<Connected>");
//        Serial.print('\r');
//        delay(500);
      } else {
        connectWlan();  
      }
      
      
    } else {
      val = str.toInt();
      uploadValues();
    }
    str = "";
    flag = 0;
    reader.attach_ms(1, readChar);
  }
}

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


void connectWlan() {

//  Serial.println();
//  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print("\r");

  delay(1000);

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

void uploadValues() {
  // Use WiFiClient class to create TCP connections
  
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.print("connection failed\r");
  }

  ThingSpeak.begin(client);
  ThingSpeak.setField(1, val);
  ThingSpeak.writeFields(channelNumber, privateKey);
  Serial.print("Sent val: " + String(val) + "\r");
  delay(1000);
}
