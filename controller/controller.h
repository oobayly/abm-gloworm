#include "../glowworm_core.h"

#define TIMEOUT 200		 /* The send timout (in ms) */

// Radio configuration
#define RETRY_DELAY 15
#define RETRY_COUNT 15
#define CHANNEL 0x70
uint8_t addresses[][6] = {"Ctrlr","Gworm", "Cldrn"};

typedef enum {
	PROGRAM_GLOWWORM = 0x1,
	PROGRAM_CAULDRON = 0x2
} program_e;

int main(int argc, char** argv);

void setupRadio();

void setupRadio(program_e prog);

bool writeGlowworm(antenna_e antenna);

bool writeGlowworm(uint8_t r, uint8_t g, uint8_t b);

bool writeGlowworm(antenna_e antenna, uint8_t r, uint8_t g, uint8_t b);

bool writeRadio(void * payload, size_t length);
