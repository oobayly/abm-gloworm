/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 03/17/2013 : Charles-Henri Hallard (http://hallard.me)
              Modified to use with Arduipi board http://hallard.me/arduipi
						  Changed to use modified bcm2835 and RF24 library
TMRh20 2014 - Updated to work with optimized RF24 Arduino library

 */

/**
 * Example RF Radio Ping Pair
 *
 * This is an example of how to use the RF24 class on RPi, communicating to an Arduino running
 * the GettingStarted sketch.
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <RF24/RF24.h>

using namespace std;

#define INTERVAL 3	/* The send interval (in ms) */
#define TIMEOUT 200	/* The send timout (in ms) */

// Radio configuration
#define RETRY_DELAY 15
#define RETRY_COUNT 15
#define CHANNEL 0x70
//const uint8_t addresses[2][6] {"GLO_M", "GLO_S"}; // Master, Slave
uint8_t addresses[][6] = {"1Node","2Node"};

// Which antennae should be lit
typedef enum {
  ANTENNA_NONE = 0x0,
  ANTENNA_LEFT = 0x1,
  ANTENNA_RIGHT = 0x2,
  ANTENNA_BOTH = ANTENNA_LEFT + ANTENNA_RIGHT
} antenna_e;

// The configuration that is send over the RF24 link
typedef struct {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t antenna;
} led_config;

RF24 * const radio = new RF24(RPI_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);

int main(int argc, char** argv){
	// HACK: Do the startup of the scanner app
	// I think this makes it go into standby
	radio->begin();
	radio->setAutoAck(false);
	radio->startListening();
	radio->stopListening();
	
	radio->begin();
	radio->setAutoAck(true);
	radio->setChannel(CHANNEL);
	
	//radio->enableAckPayload(); // So we can send a response back
	radio->setRetries(RETRY_DELAY, RETRY_COUNT); // How often and how many times to retry
	
	radio->setPayloadSize(4); // TODO: Use sizeof
	
	// Use High power, and slowest rate
	radio->setPALevel(RF24_PA_HIGH);
	//  radio->setDataRate(RF24_1MBPS);
	//  radio->setPALevel(RF24_PA_LOW);
	radio->setDataRate(RF24_250KBPS);
	
	// Pipes
	radio->openReadingPipe(1, addresses[0]); // We write to the slave
	radio->openWritingPipe(addresses[1]);    // And read from the master
	
	radio->printDetails();

	while (1) {
		// Stop listening so we can send
		radio->stopListening();

		// Current time so we can measure the response time
		uint32_t time = millis();

		uint32_t payload = rand() & 0x03c0c0c0;
		printf("Now sending %08x ...", payload);

		bool ok = radio->write(&payload, sizeof(payload));
		printf("%s\n", ok ? "done" : "failed");

		// Start listening agains so we can receive the response
		radio->startListening();

		uint32_t startedWaiting = millis();
		bool timeout = false;
		while (!radio->available() &&  !timeout) {
			if ((millis() - startedWaiting) > TIMEOUT) {
				timeout = true;
			}
		}

		uint32_t pingTime = millis() - time;

		// What happened
		if (timeout) {
			printf("Failed - response timed out after %u.\n", TIMEOUT);
		} else {
			uint32_t response;
			radio->read(&response, sizeof(response));

			printf("Got response: %08x, round-trip delay: %u\n", response, pingTime);
		}

		sleep(INTERVAL);
	}

	return 0;
}
