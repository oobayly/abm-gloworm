#include "RF24.h"

#ifndef GLOWWORM_H
#define GLOWWORM_H

#define DEBUG  /* Enable Serial printing */ 

#define FADE_STEPS 250      /* The number of steps taken to fade between 2 colours */
#define FADE_INTERVAL 2000  /* The time taken (in ms) to fade between 2 colours */

#define SERIAL_BAUD 57600   /* Baud rate used for serial output */
#define SERIAL_BUFFER 64    /* The number of bytes we can read from the buffer */

// The SPI pins
#define SPI_CE 7
#define SPI_CSN 8

#define RETRY_DELAY 0
#define RETRY_COUNT 15
#define CHANNEL 0x70
//const uint8_t addresses[2][6] {"GLO_M", "GLO_S"}; // Master, Slave
byte addresses[][6] = {"2Node","1Node"};

// Timer configuration and steps to be used
#define TIMER1_INTERVAL 1000 /* Timer1 uses microsecond precision */
#define TIMER1_FADE_STEP ((FADE_INTERVAL / (TIMER1_INTERVAL / 1000)) / FADE_STEPS)

// LED pins
typedef enum {
  LED_LEFT = 4,
  LED_RIGHT = 2,
  LED_BLUE = 6,
  LED_GREEN = 5,
  LED_RED = 3
} leds_e;

// Which antennae should be lit
typedef enum {
  ANTENNA_NONE = 0x0,
  ANTENNA_LEFT = 0x1,
  ANTENNA_RIGHT = 0x2,
  ANTENNA_BOTH = ANTENNA_LEFT + ANTENNA_RIGHT
} antenna_e;

// The LED colours
// Colours are stored as a 32b ARGB value, with Alpha being ignored
typedef struct {
  volatile uint32_t current; // The currently set colour
  volatile uint32_t last;    // The last set colour - used for interpolation
  volatile uint32_t next;    // The colour that is to be set
  volatile uint16_t step;    // The counter for interpolating
  volatile antenna_e antenna;// Which antennae should be lit 
} led_status;

// The configuration that is send over the RF24 link
typedef struct {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t antenna;
} led_config;

// The current lighting configuration
led_status * const lights = new led_status();

// The RF24 radio object
RF24 * const radio = new RF24(SPI_CE, SPI_CSN);

// The counter used for Timer1
uint16_t timer1_counter = 0;

// Changes the LEDs to the specified colours
void changeColour(led_config *);

led_config * readSerial();

void setupLEDs();

void setupRadio();

void timer1_tick();

#endif