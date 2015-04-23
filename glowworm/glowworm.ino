#include <TimerOne.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "glowworm_core.h"
#include "glowworm.h"

void setup() {
#ifdef DEBUG
  Serial.begin(SERIAL_BAUD);
  printf_begin();
  
  printf("TIMER1_INTERVAL: %u us\r\n", TIMER1_INTERVAL);
  printf("FADE_STEPS: %u\r\n", FADE_STEPS);
  printf("FADE_INTERVAL: %u ms\r\n", FADE_INTERVAL);
  printf("TIMER1_FADE_STEP: %u\r\n", TIMER1_FADE_STEP);
  
  printf("Size of glowworm is %u bytes\r\n", sizeof(glowworm_config));
#endif
  
  // Clear the current lights
  lights->last = 0;
  lights->next = 0x808080;
  lights->step = 0;
  lights->antenna = ANTENNA_NONE;
  
  setupLEDs();

  // Initialise the timer
  Timer1.initialize(TIMER1_INTERVAL);
  Timer1.attachInterrupt(timer1Tick);
  
  // Configure the radio using the specified SPI pins
  setupRadio();
}

void loop() {
  glowworm_config * config = NULL;
  
  if (radio->available()) {
    config = new glowworm_config();

    // Read the data from the radio - we're dealing with constant length data
    // for the time being
    while (radio->available()) {
      radio->read(config, sizeof(*config));
    }
    
    // Swap into write mode so we can respond
    radio->stopListening();
    radio->write(&config, sizeof(*config));
    radio->startListening();
  
  } else if (Serial.available() >= (2 * sizeof(glowworm_config))) {
    config = readSerial();
  }

  if (config) {
    changeColour(config);
    delete config;
  }

  if(radio->failureDetected){ 
    radio->failureDetected = 0;
    setupRadio();
  }  
}

void changeColour(glowworm_config * config) {
  lights->last = lights->current; // Use the current colour to interpolate from
  lights->next =
    ((uint32_t)config->r << 16) |
    ((uint32_t)config->g << 8) |
    ((uint32_t)config->b << 0);
  lights->step = 0;
  
  lights->antenna = (antenna_e)(config->antenna & ANTENNA_BOTH);
    digitalWrite(LED_LEFT, lights->antenna & ANTENNA_LEFT);
    digitalWrite(LED_RIGHT, lights->antenna & ANTENNA_RIGHT);

  printf("Setting colour from %06lx to %06lx - Antenna: %u\r\n", lights->last, lights->next, lights->antenna);
}

glowworm_config * readSerial() {
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

  glowworm_config *config = NULL;
  
  if (read >= (2 * sizeof(glowworm_config))) {
    config = new glowworm_config();
    
    // We're expecting a hex representation of the glowworm object
    // so null terminate the string
    buff[2 * sizeof(glowworm_config)] = 0;
    
    printf("Read %u bytes from serial: %s\n", read, buff);
    
    // Fill the buffer into the config
    uint32_t * temp = (uint32_t *)config;
    *temp = strtol(buff, NULL, 16);
  }

  delete buff;
  
  return config;
}

void setupLEDs() {
  pinMode(LED_LEFT, OUTPUT);
  pinMode(LED_RIGHT, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  
  digitalWrite(LED_LEFT, LOW);
  digitalWrite(LED_RIGHT, LOW);
  analogWrite(LED_RED, LOW);
  analogWrite(LED_GREEN, LOW);
  analogWrite(LED_BLUE, LOW);
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
  
  radio->setPayloadSize(4); // TODO: Use sizeof
  
  // Use High power, and slowest rate
  radio->setPALevel(RF24_PA_HIGH);
//  radio->setDataRate(RF24_1MBPS);
//  radio->setPALevel(RF24_PA_LOW);
  radio->setDataRate(RF24_250KBPS);
  
  // Pipes
  radio->openReadingPipe(1, addresses[0]); // We write to the slave
  radio->openWritingPipe(addresses[1]);    // And read from the master
  
  radio->printDetails();
  
  radio->startListening();
}

void timer1Tick() {
  // Reset the timer every fade interval
  if (timer1_counter >= FADE_INTERVAL) {
    timer1_counter = 0;
  }
  
//  // Generate a new colour & antenna configuration every interval
//  if (timer1_counter == 0) {
//    lights->last = lights->next;
//    lights->next = random(0x1000000) & 0xe0e0e0; // Clip to every 0x20
//    lights->step = 0;
//    
//    // Generate a random antenna value
//    lights->antenna = (antenna_e)random(1 + ANTENNA_BOTH);
//    digitalWrite(LED_LEFT, lights->antenna & ANTENNA_LEFT);
//    digitalWrite(LED_RIGHT, lights->antenna & ANTENNA_RIGHT);
//
//    printf("%06lX\t%06lX\t%u\r\n", lights->last, lights->next, lights->antenna);
//  }

  // Every fade increment update the colours
  if (!(timer1_counter % TIMER1_FADE_STEP)) {
    // But only if the colours have changed
    if (lights->current != lights->next) {
      // Act on the each byte of the ARGB values
      uint8_t * last = (uint8_t *)&lights->last;
      uint8_t * next = (uint8_t *)&lights->next;
      uint8_t * current = (uint8_t *)&lights->current;
  
      // Interpolate the rgb values, ignoring MSB (Alpha)
      for (uint8_t i = 0; i < 3; i++) {
        // Use signed as the difference can be negative
        int16_t l = last[i];
        int16_t n = next[i];
        
        // Cast delta as float so we don't overflow
        current[i] = (uint8_t)(l + ((float)(n - l) * lights->step / FADE_STEPS));
      }
      
      analogWrite(LED_RED, current[2]);
      analogWrite(LED_GREEN, current[1]);
      analogWrite(LED_BLUE, current[0]);
  
      lights->step++;
    }
  }
  
  timer1_counter++;
}
