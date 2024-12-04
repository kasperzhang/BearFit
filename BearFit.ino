#include <HCSR04.h>
#include <ArduinoBLE.h>
#include "ble_functions.h"

#define TRIGGER_PIN 2
#define ECHO_PIN 3

// Create the distance sensor object
UltraSonicDistanceSensor distanceSensor(TRIGGER_PIN, ECHO_PIN);

// Global variables
int sensorValue0;
int sensorValue1;

const int BUZZER_PIN = 11;       // Pin for haptic feedback buzzer
const int LED_PIN = LED_BUILTIN; // Status LED pin

float distance = 0.0f;          // Current distance in cm
float smoothedDistance = 0.0f;  // Filtered distance
unsigned long lastDistanceReadTime = 0;
unsigned int distanceReadInterval = 30;  // Time between reads in milliseconds 

//Name your controller!
const char* deviceName = "BearFit";
// Movement state tracking
int currentMovement = 0;         // Current movement value (0=none, 1=up, 2=down, 3=handshake)

// Rolling average variables for distance
const int DISTANCE_AVERAGE_WINDOW = 5;
float distanceReadings[DISTANCE_AVERAGE_WINDOW];
int distanceReadIndex = 0;
float distanceTotalValue = 0;

// Function to initialize the rolling average array
void initializeDistanceAverage() {
  for (int i = 0; i < DISTANCE_AVERAGE_WINDOW; i++) {
    distanceReadings[i] = 0;
  }
  distanceTotalValue = 0;
  distanceReadIndex = 0;
}

// Function to update rolling average with new value
void updateDistanceAverage(float newValue) {
  // Only update if we have a valid reading
  if (newValue > 0) {
    // Subtract the oldest reading from the total
    distanceTotalValue = distanceTotalValue - distanceReadings[distanceReadIndex];
    // Add the new reading to the array
    distanceReadings[distanceReadIndex] = newValue;
    // Add the new reading to the total
    distanceTotalValue = distanceTotalValue + newValue;
    // Advance to the next position in the array
    distanceReadIndex = (distanceReadIndex + 1) % DISTANCE_AVERAGE_WINDOW;
    // Calculate the average
    smoothedDistance = distanceTotalValue / DISTANCE_AVERAGE_WINDOW;
  }
}

// Function to read distance sensor and update the global value
void readDistance() {
  unsigned long currentTime = millis();
  if (currentTime - lastDistanceReadTime >= distanceReadInterval) {
    // Read the distance
    float newDistance = distanceSensor.measureDistanceCm();
    
    // Update values if reading is valid
    if (newDistance > 0) {  // Basic error checking
      distance = newDistance;
      updateDistanceAverage(distance);
    }
    
    // Print the values
    printDistanceValues();
    
    // Update the last read time
    lastDistanceReadTime = currentTime;
  }
}

// Function to print distance values
void printDistanceValues() {
  printPresureValue();
  Serial.print("Distance Smoothed: ");
  Serial.print(smoothedDistance);
  Serial.println(" cm");
  Serial.println();    
}

void printPresureValue(){
  sensorValue0 = analogRead(A0);  // read the input on analog pin
  Serial.print(sensorValue0);     // print the value
  Serial.print(", ");             // print a comma and space

  sensorValue1 = analogRead(A1);  // read the input on analog pin
  Serial.print(sensorValue1);     // print the value
  Serial.print(", ");             // print a comma and space
}


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  Serial.println("Distance Sensor test!");
  // Initialize Bluetooth Low Energy with device name and status LED
  setupBLE(deviceName, LED_PIN);
  // Initialize the rolling average
  initializeDistanceAverage();
}

// the loop routine runs over and over again forever:
void loop() {
  readDistance();
  // Update BLE connection status and handle incoming data
  updateBLE();
  
  //read the inputs te determine the current state
  //results in changing the value of currentMovement
  handleInput();

  //send the movement state to P5  
  sendMovement(currentMovement);

}

void handleInput(){
  if(smoothedDistance > 45){
    currentMovement = 2;        // UP movement
    Serial.println("DOWN");
  }
  else if(25 < smoothedDistance && smoothedDistance < 45){
    currentMovement = 0;         // No movement
    Serial.println("NO");
  }
  else{
    currentMovement = 1;         // DOWN movement
    Serial.println("UP");
  }
}