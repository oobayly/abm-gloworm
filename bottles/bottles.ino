#include <TimerOne.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "bottles_core.h"
#include "bottles.h"

void setup() {
#ifdef DEBUG
  Serial.begin(SERIAL_BAUD);
  printf_begin();
  
  printf("TIMER1_INTERVAL: %u us\r\n", TIMER1_INTERVAL);
#endif

  // Timer needs to be initialised before the LEDs so that the PWM function can be used for Pin 9
  Timer1.initialize(TIMER1_INTERVAL);
  
  setupLEDs();
  
  setupCycles();
  
  // Only now can we start the timer
  Timer1.attachInterrupt(timer1Tick);

  // Configure the radio using the specified SPI pins
  setupRadio();
}

void loop() {
  bottles_config * config = NULL;
  
  if (radio->available()) {
    config = new bottles_config();

    // Read the data from the radio - we're dealing with constant length data
    // for the time being
    while (radio->available()) {
      radio->read(config, sizeof(*config));
    }
    
    // Swap into write mode so we can respond
    radio->stopListening();
    radio->write(&config, sizeof(*config));
    radio->startListening();
  
  } else if (Serial.available() >= (2 * sizeof(bottles_config))) {
    config = readSerial();
  }

  if (config) {
    updateConfig(config);
    delete config;
  }

  if(radio->failureDetected){ 
    radio->failureDetected = 0;
    setupRadio();
  }  
}

uint8_t interpolateValue(cycle * cycle) {
  uint32_t value = 0;
  
  // Act on each byte of the value separately
  uint8_t * minimum = (uint8_t *)&cycle->minimum;
  uint8_t * maximum = (uint8_t *)&cycle->maximum;
  uint8_t * temp = (uint8_t *)&value;

  for (uint8_t i = 0; i < sizeof(cycle->maximum); i++) {
    // Use signed as the difference can be negative
    int16_t l = minimum[i];
    int16_t n = maximum[i];
   
    // Cast delta as float so we don't overflow
    temp[i] = (uint8_t)(l + ((float)(n - l) * cycle->step / cycle->time));     
  }
  
  return value;
}

bottles_config * readSerial() {
  char * buff = new char[SERIAL_BUFFER];
  
  uint8_t read = 0;
  while (Serial.available()) {
    if (read < SERIAL_BUFFER) {
      buff[read++] = Serial.read();
    } else {
      // Just dump the data
      Serial.read();
    }
  }

  bottles_config *config = NULL;
  
  if (read >= (2 * sizeof(bottles_config))) {
    config = new bottles_config();
    
    // We're expecting a hex representation of the glowworm object
    // so null terminate the string
    buff[2 * sizeof(bottles_config)] = 0;
    
    printf("Read %u bytes from serial: %s\n", read, buff);
    
    // Fill the buffer into the config
    uint32_t * temp = (uint32_t *)config;
    *temp = strtol(buff, NULL, 16);
  }

  delete buff;
  
  return config;
}

void setupCycles() {
  white->enabled = false;
  white->minimum = 0x40;
  white->maximum = 0xff;
  white->time = 5000;
  white->step = 0;
  white->delta = 1;

  red->enabled = false;
  red->minimum = 0x10;
  red->maximum = 0xff;
  red->time = 1500;
  red->step = 0;
  red->delta = 1;
}

void setupLEDs() {
  pinMode(PWM_WHITE, OUTPUT);
  pinMode(PWM_RED, OUTPUT);
  
  analogWrite(PWM_WHITE, LOW);
  analogWrite(PWM_RED, LOW);
}

void setupRadio() {
  // HACK: Do the startup of the scanner app
  // I think this makes it go into standby
  radio->begin();
  radio->setAutoAck(false);
  radio->startListening();
  radio->stopListening();

  radio->begin();
  radio->setAutoAck(true);
  radio->setChannel(CHANNEL);
  
  //radio->enableAckPayload(); // So we can send a response back
  radio->setRetries(RETRY_DELAY, RETRY_COUNT); // How often and how many times to retry
  
  radio->setPayloadSize(sizeof(bottles_config)); // TODO: Use sizeof
  
  // Use High power, and slowest rate
  radio->setPALevel(RF24_PA_HIGH);
  radio->setDataRate(RF24_250KBPS);
  
  // Pipes - Write to the controller
  radio->openWritingPipe(addresses[0]);
  radio->openReadingPipe(1, addresses[1]);
  
  radio->printDetails();
  
  radio->startListening();
}

void stepCycle(cycle * cycle) {
  cycle->step += cycle->delta;
  if (cycle->step < 0) {
    cycle->step = 0;
    cycle->delta = -cycle->delta;
  }
  
  if (cycle->step >= cycle->time) {
    cycle->step = cycle->time;
    cycle->delta = -cycle->delta;
  }
}

void timer1Tick() {
  if (mode == BOTTLES_MODE_RIGGING) {
    analogWrite(PWM_RED, 0x10);
    
  } else {
    if (white->enabled) {
      uint8_t value = interpolateValue(white);
      analogWrite(PWM_WHITE, value);
      
      stepCycle(white);
    }
    
    if (red->enabled) {
      uint8_t value = interpolateValue(red);
      analogWrite(PWM_RED, value);
      
      stepCycle(red);
    }
  }
}

void updateConfig(bottles_config * config) {
  mode = (bottles_mode_e)config->mode;
  
  if (white->enabled = (config->mode & BOTTLES_MODE_WHITE)) {
    white->step = 0;
    white->delta = 1;
  } else {
    analogWrite(PWM_WHITE, 0);
  }

  if (red->enabled = (config->mode & BOTTLES_MODE_RED)) {
    red->step = 0;
    red->delta = 1;
  } else {
    analogWrite(PWM_RED, 0);
  }
  
  printf("Setting: Red = %s, White = %s\n",
    white->enabled ? "On" : "Off",
    red->enabled ? "On" : "Off");
}

