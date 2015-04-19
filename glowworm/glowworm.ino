#include <TimerOne.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define DEBUG  /* Enable Serial printing */ 

#define FADE_STEPS 250      /* The number of steps taken to fade between 2 colours */
#define FADE_INTERVAL 2000  /* The time taken (in ms) to fade between 2 colours */

#define SERIAL_BAUD 57600   /* Baud rate used for serial output */

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

void setup() {
#ifdef DEBUG
  Serial.begin(SERIAL_BAUD);
  printf_begin();
  
  printf("TIMER1_INTERVAL: %u us\r\n", TIMER1_INTERVAL);
  printf("FADE_STEPS: %u\r\n", FADE_STEPS);
  printf("FADE_INTERVAL: %u ms\r\n", FADE_INTERVAL);
  printf("TIMER1_FADE_STEP: %u\r\n", TIMER1_FADE_STEP);
  
  printf("Size of led_config is %u bytes\r\n", sizeof(led_config));
#endif
  
  // Clear the current lights
  lights->last = 0;
  lights->next = 0;
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
  if (radio->available()) {
    led_config * config = new led_config();

    // Read the data from the radio - we're dealing with constant length data
    // for the time being
    while (radio->available()) {
      radio->read(config, sizeof(*config));
    }
    
    // Swap into write mode so we can respond
    radio->stopListening();
    radio->write(&config, sizeof(*config));
    radio->startListening();
    
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
    
    delete config;
  }

  if(radio->failureDetected){ 
    radio->failureDetected = 0;
    setupRadio();
  }  
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
