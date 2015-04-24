#include <TimerOne.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "cauldron_core.h"
#include "cauldron.h"

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
  cauldron_config * config = NULL;
  
  if (radio->available()) {
    config = new cauldron_config();

    // Read the data from the radio - we're dealing with constant length data
    // for the time being
    while (radio->available()) {
      radio->read(config, sizeof(*config));
    }
    
    // Swap into write mode so we can respond
    radio->stopListening();
    radio->write(&config, sizeof(*config));
    radio->startListening();
  
  } else if (Serial.available() >= (2 * sizeof(cauldron_config))) {
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

uint32_t interpolateValue(cycle * cycle) {
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

cauldron_config * readSerial() {
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

  cauldron_config *config = NULL;
  
  if (read >= (2 * sizeof(cauldron_config))) {
    config = new cauldron_config();
    
    // We're expecting a hex representation of the glowworm object
    // so null terminate the string
    buff[2 * sizeof(cauldron_config)] = 0;
    
    printf("Read %u bytes from serial: %s\n", read, buff);
    
    // Fill the buffer into the config
    uint32_t * temp = (uint32_t *)config;
    *temp = strtol(buff, NULL, 16);
  }

  delete buff;
  
  return config;
}

void setupCycles() {
  // The fan cycles between 50% and 100% every 5 seconds
  fan->enabled = false;
  fan->minimum = 0x80;
  fan->maximum = 0xff;
  fan->time = 5000;
  fan->step = 0;
  fan->delta = 1;

  // The fire cycles between 50% and 100% every 250ms
  fire->enabled = false;
  fire->minimum = 0x80;
  fire->maximum = 0xff;
  fire->time = 250;
  fire->step = 0;
  fire->delta = 1;

  // The fire cycles between blue and green every 3 seconds
  glow->enabled = false;
  glow->minimum = 0x0000ff;
  glow->maximum = 0x00ff00;
  glow->time = 3000;
  fire->step = 0;
  fire->delta = 1;
}

void setupLEDs() {
  pinMode(PWM_FAN, OUTPUT);
  pinMode(PWM_RED, OUTPUT);
  pinMode(PWM_GREEN, OUTPUT);
  pinMode(PWM_BLUE, OUTPUT);
  
  analogWrite(PWM_FAN, LOW);
  analogWrite(PWM_RED, LOW);
  Timer1.pwm(PWM_GREEN, 0); // Uses Timer1
  analogWrite(PWM_BLUE, LOW);
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
  
  radio->setPayloadSize(sizeof(cauldron_config)); // TODO: Use sizeof
  
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
  if (fan->enabled) {
    uint32_t value = interpolateValue(fan);
    analogWrite(PWM_FAN, (uint8_t)value);
    
    stepCycle(fan);
  }
  
  if (fire->enabled) {
    uint32_t value = interpolateValue(fire);
    analogWrite(PWM_RED, (uint8_t)value);

    stepCycle(fire);
  }
  
  if (glow->enabled) {
    uint32_t value = interpolateValue(glow);
    Timer1.setPwmDuty(PWM_GREEN, 1023 * (value >> 8) / 0xff);
    analogWrite(PWM_BLUE, (uint8_t)value);

    stepCycle(glow);
  }
}

void updateConfig(cauldron_config * config) {
  if (fan->enabled = (config->mode & CAULDRON_MODE_FAN)) {
    fan->step = 0;
    fan->delta = 1;
  } else {
    analogWrite(PWM_FAN, 0);
  }

  if (fire->enabled = (config->mode & CAULDRON_MODE_FIRE)) {
    fire->step = 0;
    fire->delta = 1;
  } else {
    analogWrite(PWM_RED, 0);
  }

  if (glow->enabled = (config->mode & CAULDRON_MODE_GLOW)) {
    glow->step = 0;
    glow->delta = 1;
  } else {
    analogWrite(PWM_BLUE, 0);
    Timer1.setPwmDuty(PWM_GREEN, 0);
  }
  
  printf("Setting: Fan = %s, Fire = %s, Glow = %s\n",
    fan->enabled ? "On" : "Off",
    fire->enabled ? "On" : "Off",
    glow->enabled ? "On" : "Off");
}

