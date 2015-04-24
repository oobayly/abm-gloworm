#include "RF24.h"

#include "cauldron_core.h"

#ifndef CAULDRON_H
#define CAULDRON_H

#define DEBUG  /* Enable Serial printing */ 

#define SERIAL_BAUD 57600   /* Baud rate used for serial output */
#define SERIAL_BUFFER 64    /* The number of bytes we can read from the buffer */

// The SPI pins
#define SPI_CE 7
#define SPI_CSN 8

#define RETRY_DELAY 0
#define RETRY_COUNT 15
#define CHANNEL 0x70
byte addresses[][6] = {"Ctrlr","Cldrn"};

// Timer configuration and steps to be used
#define TIMER1_INTERVAL 1000 /* Timer1 uses microsecond precision */

// PWM pins
typedef enum {
  PWM_FAN = 3,
  PWM_BLUE = 5,
  PWM_RED = 6,
  PWM_GREEN = 9 // Need to scale PWM values by 4 for this as Timer1 uses 10bit resolution
} leds_e;

// Container for cycling values
typedef struct {
  volatile bool enabled;
  uint32_t minimum;
  uint32_t maximum;
  int16_t time; // The number of miliseconds over which the value cycles
  volatile int16_t step; // The current step
  volatile int8_t delta;
} cycle;

// The current lighting configuration
volatile cauldron_mode_e mode = CAULDRON_MODE_NONE;

// The RF24 radio object
RF24 * const radio = new RF24(SPI_CE, SPI_CSN);

cycle * const fan = new cycle();
cycle * const fire = new cycle();
cycle * const glow = new cycle();

cauldron_config * readSerial();

uint32_t interpolateValue(cycle * cycle);

void setupCycles();

void setupLEDs();

void setupRadio();

void stepCycle(cycle * cycle);

void timer1_tick();

void updateConfig(cauldron_config * config);

#endif
