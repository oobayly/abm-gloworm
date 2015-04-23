#ifndef GLOWWORM_CORE_H
#define GLOWWORM_CORE_H

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
} glowworm_config;

#endif

