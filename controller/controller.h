#include "../glowworm/glowworm_core.h"

// Useful for determinging the size of an array
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define TIMEOUT 200		 /* The send timout (in ms) */

// Radio configuration
#define RETRY_DELAY 15
#define RETRY_COUNT 15
#define CHANNEL 0x70
uint8_t addresses[][6] = {
	"Ctrlr",
	"Bttls",
	"Gworm",
	"Cldrn"};

typedef enum {
	CAULDRON_BACK
} cauldron_e;

typedef enum {
	BOTTLES_BACK
} bottles_e;

typedef enum {
	GLOWWORM_BACK,
	GLOWWORM_OFF,
	GLOWWORM_TOGGLE_BOTH,
	GLOWWORM_TOGGLE_LEFT,
	GLOWWORM_TOGGLE_RIGHT,
	GLOWWORM_RED,
	GLOWWORM_GREEN,
	GLOWWORM_BLUE
} glowworm_e;

typedef enum {
	PROGRAM_EXIT,
	PROGRAM_BOTTLES,
	PROGRAM_GLOWWORM,
	PROGRAM_CAULDRON
} program_e;

const char * BOTTLES_MENU[] = {
	"Back"
};

const char * CAULDRON_MENU[] = {
	"Back"
};

const char * GLOWWORM_MENU[] = {
	"Back",
	"All Off",
	"Toggle Both",
	"Toggle Left",
	"Toggle Right",
	"Red",
	"Green",
	"Blue"
};

const char * MAIN_MENU[] = {
	"Exit",
	"Bottles",
	"Glowworm",
	"Cauldron"
};

int main(int argc, char** argv);

bool createMenu(program_e prog);

bool doBottles(bottles_e command);

bool doCauldron(cauldron_e command);

bool doGlowworm(glowworm_e command);

void programMenuSelected(program_e prog, ITEM * item);

void setupRadio();

void setupRadio(program_e prog);

bool writeGlowworm(antenna_e antenna);

bool writeGlowworm(uint8_t r, uint8_t g, uint8_t b);

bool writeGlowworm(antenna_e antenna, uint8_t r, uint8_t g, uint8_t b);

bool writeRadio(void * payload, size_t length, void * response);
