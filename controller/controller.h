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
	BOTTLES_BACK,
	BOTTLES_OFF,
	BOTTLES_RED,
	BOTTLES_WHITE
} bottles_e;

typedef enum {
	CAULDRON_BACK,
	CAULDRON_OFF,
	CAULDRON_TOGGLE_ALL,
	CAULDRON_TOGGLE_FAN,
	CAULDRON_TOGGLE_FIRE,
	CAULDRON_TOGGLE_GLOW
} cauldron_e;

typedef enum {
	GLOWWORM_BACK,
	GLOWWORM_OFF,
	GLOWWORM_TOGGLE_BOTH,
	GLOWWORM_TOGGLE_LEFT,
	GLOWWORM_TOGGLE_RIGHT,
	GLOWWORM_BLINK,
	GLOWWORM_RED,
	GLOWWORM_GREEN,
	GLOWWORM_BLUE,
	GLOWWORM_YELLOW,
	GLOWWORM_PINK
} glowworm_e;

typedef enum {
	PROGRAM_EXIT,
	PROGRAM_BOTTLES,
	PROGRAM_GLOWWORM,
	PROGRAM_CAULDRON
} program_e;

const char * BOTTLES_MENU[] = {
	"Back",
	"ALl Off",
	"Toggle Red",
	"Toggle White"
};

const char * CAULDRON_MENU[] = {
	"Back",
	"All Off",
	"Toggle All",
	"Toggle Fan",
	"Toggle Fire",
	"Toggle Cauldron Glow"
};

const char * GLOWWORM_MENU[] = {
	"Back",
	"All Off",
	"Toggle Both",
	"Toggle Left",
	"Toggle Right",
	"Toggle Blink",
	"Red",
	"Green",
	"Blue",
	"Yellow",
	"Pink"
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

bool writeBottles(bottles_mode_e mode);

bool writeCauldron(cauldron_mode_e mode);

bool writeGlowworm(antenna_e antenna);

bool writeGlowworm(uint8_t r, uint8_t g, uint8_t b);

bool writeGlowworm(antenna_e antenna, uint8_t r, uint8_t g, uint8_t b);

bool writeRadio(void * payload, size_t length, void * response);
