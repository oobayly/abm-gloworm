#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <RF24/RF24.h>

#include "../glowworm/glowworm_core.h"
#include "controller.h"

using namespace std;

#define INTERVAL 3	/* The send interval (in ms) */

RF24 * const radio = new RF24(RPI_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);

glowworm_config * const glowworm = new glowworm_config();

int main(int argc, char** argv){
	setupRadio();
	setupRadio(PROGRAM_GLOWWORM);

	while (1) {
		// For the time being, just generate some random colours
		// Clip the antenna to 3, and truncate the colours to increments of 64
		uint32_t data = rand() & 0x03c0c0c0;
		writeGlowworm(
			(antenna_e)((data >> 24) & 0xff),
			data >> 16,
			data >> 8,
			data);

		sleep(INTERVAL);
	}

	return 0;
}

void setupRadio() {
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
	
	// Use High power, and slowest rate
	radio->setPALevel(RF24_PA_HIGH);
	radio->setDataRate(RF24_250KBPS);
}

void setupRadio(program_e prog) {
	radio->stopListening();
	size_t payload;
	const uint8_t read = 0;
	uint8_t write;

	switch (prog) {
		case PROGRAM_GLOWWORM:
			payload = sizeof(glowworm_config);
			write = PROGRAM_GLOWWORM;
			break;

		default:
			return;

	}

	radio->setPayloadSize(payload);
	radio->openReadingPipe(1, addresses[read]);
	radio->openWritingPipe(addresses[write]);

	radio->startListening();
	
	radio->printDetails();
}

bool writeGlowworm(antenna_e antenna) {
	return writeGlowworm(antenna, glowworm->r, glowworm->g, glowworm->b);
}

bool writeGlowworm(uint8_t r, uint8_t g, uint8_t b) {
	return writeGlowworm((antenna_e)glowworm->antenna, r, g, b);
}

bool writeGlowworm(antenna_e antenna, uint8_t r, uint8_t g, uint8_t b) {
	glowworm->antenna = antenna;
	glowworm->r = r;
	glowworm->g = g;
	glowworm->b = b;

	printf("Sending: Antenna: %i, R: 0x%02x, G: 0x%02x, B: 0x%02x ...\n",
		antenna, r, g, b);

	return writeRadio(glowworm, sizeof(glowworm));
}

bool writeRadio(void * payload, size_t length) {
	// Stop listening so we can send
	radio->stopListening();

	// Current time so we can measure the response time
	uint32_t time = millis();

	printf("Sending %i bytes...", length);
	bool ok = radio->write(payload, length);
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
		uint8_t * response = new uint8_t[length];
		radio->read(response, length);
		printf("Received %i bytes - round-trip delay: %u\n", length, pingTime);
		delete response;
	}

	return true;
}
