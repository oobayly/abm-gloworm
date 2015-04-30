#include "RF24.h"

#include "glowworm_core.h"

#ifndef GLOWWORM_H
#define GLOWWORM_H

#define DEBUG  /* Enable Serial printing */ 

#define FADE_STEPS 250      /* The number of steps taken to fade between 2 colours */
#define FADE_INTERVAL 1000  /* The time taken (in ms) to fade between 2 colours */

#define BLINK_INTERVAL 500  /* The time (in ms) between antenna blinks */

#define SERIAL_BAUD 57600   /* Baud rate used for serial output */
#define SERIAL_BUFFER 64    /* The number of bytes we can read from the buffer */

// The SPI pins
#define SPI_CE 7
#define SPI_CSN 8

#define RETRY_DELAY 0
#define RETRY_COUNT 15
#define CHANNEL 0x70
byte addresses[][6] = {"Ctrlr","Gworm"};

// Timer configuration and steps to be used
#define TIMER1_INTERVAL 1000 /* Timer1 uses microsecond precision */
#define TIMER1_FADE_STEP ((FADE_INTERVAL / (TIMER1_INTERVAL / 1000)) / FADE_STEPS)

// LED pins
typedef enum {
  LED_LEFT = 4,
  LED_RIGHT = 2,
  LED_BLUE = 3,
  LED_RED = 5,
  LED_GREEN = 6
} leds_e;

// The LED colours
// Colours are stored as a 32b ARGB value, with Alpha being ignored
typedef struct {
  volatile uint32_t current; // The currently set colour
  volatile uint32_t last;    // The last set colour - used for interpolation
  volatile uint32_t next;    // The colour that is to be set
  volatile uint16_t step;    // The counter for interpolating
  volatile antenna_e antenna;// Which antennae should be lit 
} led_status;

// The current lighting configuration
led_status * const lights = new led_status();

// The RF24 radio object
RF24 * const radio = new RF24(SPI_CE, SPI_CSN);

// The counter used for Timer1
uint16_t timer1_counter = 0;

// Changes the LEDs to the specified colours
void changeColour(glowworm_config *);

glowworm_config * readSerial();

void setupLEDs();

void setupRadio();

void timer1_tick();

#endif
