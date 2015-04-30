#ifndef BOTTLES_CORE_H
#define BOTTLES_CORE_H

typedef enum {
	BOTTLES_MODE_NONE = 0,
	BOTTLES_MODE_WHITE = 0x1,
	BOTTLES_MODE_RED = 0x2,
} bottles_mode_e;

// The configuration that is send over the RF24 link
typedef struct {
	uint8_t mode;
} bottles_config;

#endif

