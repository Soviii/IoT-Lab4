
#include <Arduino.h>
#include <BLEDevice.h>
#include "SparkFunLSM6DSO.h"
#include "Wire.h"
#include <string.h>

#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

int LED_pin = GPIO_NUM_25;

/* code from lab4 documentation */
class MyCallbacks: public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pCharacteristic) {
		std::string value = pCharacteristic->getValue();
		
		/* Part A */
		if (value.length() > 0) {
			if(value == "on"){
				digitalWrite(LED_pin, HIGH);
			} else if (value == "off"){
				digitalWrite(LED_pin, LOW);
			}
			Serial.println("received value");
		}
	}
};

LSM6DSO myIMU;
int SDA_pin = GPIO_NUM_22;
int SCL_pin = GPIO_NUM_21;

/* variables needed for callibration and counting steps*/
bool prev_peak = false;
float prev_accel = 0;
int steps = 0;
float z_accel_offset = 0;



/* variables for bluetooth instance setup */
BLECharacteristic *pCharacteristic;

void setup(){
	Serial.begin(9600);
	delay(3000); 

	Wire.begin(SDA_pin, SCL_pin); // connection via I2C
	delay(1000);

	/* connecting to IMU via I2C connection */
	if (myIMU.begin())
		Serial.println("Ready.");
	else{
		Serial.println("Could not connect to IMU.");
		Serial.println("Freezing");
		while(true){}; // do not continue (bugs out program)
	}

	/* initializing IMU */
	if (myIMU.initialize(BASIC_SETTINGS))
		Serial.println("Loaded Settings.");

	/* calibration for acceleration in z-axis */
	Serial.println("Calibrating...");
	float z_accel_total = 0;
	for (int i = 0; i < 20; i++){
		z_accel_total += myIMU.readFloatAccelZ();
		delay(10);
	}
	z_accel_offset = z_accel_total / 200.0; // divided by 200 to reduce offset effectiveness
	Serial.print("Calibration complete. Z-axis offset: ");
	Serial.println(z_accel_offset);




	/* setup for bluetooth (from lab4 documentation)*/
	pinMode(LED_pin, OUTPUT);
	Serial.println("Starting bluetooth instance...");
	BLEDevice::init("SDSUCS Sophia & Sovi");
	BLEServer *pServer = BLEDevice::createServer();

	BLEService *pService = pServer->createService(SERVICE_UUID);

	pCharacteristic = pService->createCharacteristic(
										CHARACTERISTIC_UUID,
										BLECharacteristic::PROPERTY_READ |
										BLECharacteristic::PROPERTY_NOTIFY | //added notify to consistently download new data
										BLECharacteristic::PROPERTY_WRITE
										);

	pCharacteristic->setCallbacks(new MyCallbacks());
	pService->start();

	BLEAdvertising *pAdvertising = pServer->getAdvertising();
	pAdvertising->start();

	Serial.println("Bluetooth instance setup complete!");
}

void loop(){
	/* Part B */
	float curr_accel = myIMU.readFloatAccelZ() - z_accel_offset; // read acceleration from sensor
	bool curr_peak = (curr_accel > 1.5 && prev_accel < 1.5); // if we have taken a step, meaning acceleration has increased
	prev_peak = (curr_accel < prev_accel && prev_peak == true); // needed for if we have found a peak already and do not need to increment for the same step
	prev_accel = curr_accel; // assign to previous for next step observation


	if (curr_peak){
		Serial.println("Step detected!");
		Serial.print("Number of steps: ");
		Serial.println(++steps);
		String msg = "Steps: " + String(steps);
		pCharacteristic->setValue(msg.c_str());
		delay(10);
		pCharacteristic->notify();
	}

	delay(50);
}
