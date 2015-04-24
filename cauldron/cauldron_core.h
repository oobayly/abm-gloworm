#ifndef CAULDRON_CORE_H
#define CAULDRON_CORE_H

typedef enum {
	CAULDRON_MODE_NONE = 0x0,
	CAULDRON_MODE_FAN = 0x1,
	CAULDRON_MODE_FIRE = 0x2,
	CAULDRON_MODE_GLOW = 0x4
  
} cauldron_mode_e;

// The configuration that is send over the RF24 link
typedef struct {
	uint8_t mode;
} cauldron_config;

#endif

