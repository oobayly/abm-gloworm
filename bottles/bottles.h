#include "RF24.h"

#include "bottles_core.h"

#ifndef BOTTLES_H
#define BOTTLES_H

#define DEBUG  /* Enable Serial printing */ 

#define SERIAL_BAUD 57600   /* Baud rate used for serial output */
#define SERIAL_BUFFER 64    /* The number of bytes we can read from the buffer */

// The SPI pins
#define SPI_CE 7
#define SPI_CSN 8

#define RETRY_DELAY 0
#define RETRY_COUNT 15
#define CHANNEL 0x70
byte addresses[][6] = {"Ctrlr","Bttls"};

// Timer configuration and steps to be used
#define TIMER1_INTERVAL 1000 /* Timer1 uses microsecond precision */

// PWM pins
typedef enum {
  PWM_WHITE = 3,
  PWM_RED = 5,
} leds_e;

// Container for cycling values
typedef struct {
  volatile bool enabled;
  uint8_t minimum;
  uint8_t maximum;
  int16_t time; // The number of miliseconds over which the value cycles
  volatile int16_t step; // The current step
  volatile int8_t delta;
} cycle;

// The current lighting configuration
volatile bottles_mode_e mode = BOTTLES_MODE_NONE;

// The RF24 radio object
RF24 * const radio = new RF24(SPI_CE, SPI_CSN);

cycle * const white = new cycle();
cycle * const red = new cycle();

bottles_config * readSerial();

uint8_t interpolateValue(cycle * cycle);

void setupCycles();

void setupLEDs();

void setupRadio();

void stepCycle(cycle * cycle);

void timer1_tick();

void updateConfig(bottles_config * config);

#endif
