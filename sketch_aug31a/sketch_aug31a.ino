/*
 *  Interfacing Ultrasonic Sensor with Arduino
 *  Date: 31/8/2018
 *  
 *  author: reuel
 */

int trigPin = 10;
int echoPin = 2;

float msec;
float distance;

void setup() {
  // init Serial Port
  Serial.begin(9600);

  // Define input and output pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

}

void loop() {
  
  // Trigger Sensor
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read input from echo pin
  msec = pulseIn(echoPin, HIGH);

  // Calculate distance from time
  // TODO: Tune speed of sound to get more accurate measurements
  distance = 0.034837*(msec/2.0);

  // TODO: Adding WIFI Module

  // TODO: Uploading data to cloud through WiFi


  // Log distamce and time
  Serial.print("Distance: " + String(distance) + ", Time: " + String(msec));
  Serial.println("");

  // Interval between two trigger pulses
  delay(100); 
}
