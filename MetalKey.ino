// *****************************************************************************
// *   ███╗   ███╗███████╗████████╗ █████╗ ██╗     ██╗  ██╗███████╗██╗   ██╗   *
// *   ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██║     ██║ ██╔╝██╔════╝╚██╗ ██╔╝   *
// *   ██╔████╔██║█████╗     ██║   ███████║██║     █████╔╝ █████╗   ╚████╔╝    *
// *   ██║╚██╔╝██║██╔══╝     ██║   ██╔══██║██║     ██╔═██╗ ██╔══╝    ╚██╔╝     *
// *   ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║███████╗██║  ██╗███████╗   ██║      *
// *   ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚══════╝   ╚═╝      *
// *****************************************************************************
// *                        FABRICA DE ENIGMAS (c)                             *
// *                      Tiago Ventura - Abr/2018                             *
// *                       tiago.ventura@gmail.com                             *
// *****************************************************************************

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <Arduino.h>


// *****************************************************************************
// *                   DEVICES AND MODULES PIN CONFIGURATION                   *
// *****************************************************************************

/* Control */
#define AUTO_CALIBRATE_SAMPLES_COUNT        (32)
#define THRESHOLD_VARIATION_AMMOUNT         (0.10)
#define FREQUENCY_COUNTER_PULSE_TIMEOUT_US  (10000)
#define FREQUENCY_COUNTER_MAX_SAMPLES       (128)


/* Device Pins */
#define FIRST_OSCILLATOR_PIN     (2)
#define SECOND_OSCILLATOR_PIN    (3)
#define FIRST_RELAY_PIN          (A0)
#define SECOND_RELAY_PIN         (A1)
#define FIRST_LED_PIN            (A2)
#define SECOND_LED_PIN           (A3)


int g_fosc1_threshold = 0;
int g_fosc2_threshold = 0;

bool g_blink_led1 = false;
bool g_blink_led2 = false;


// *****************************************************************************
// *                              DEBUG CONTROL                                *
// *****************************************************************************

//#define ENABLE_SERIAL_DEBUG

#define DEBUG_SERIAL_BAUD_RATE    (9600)
#define DEBUG_MSG_MAX_LEN         (80)

#ifdef ENABLE_SERIAL_DEBUG
#define DEBUG_INIT()         do{ Serial.begin(DEBUG_SERIAL_BAUD_RATE); while(!Serial); } while(0)
#define DEBUG( _msg, ... )   debug( _msg, ## __VA_ARGS__ )
#else
#define DEBUG_INIT()
#define DEBUG( _msg, ... )
#endif

#ifdef ENABLE_SERIAL_DEBUG
void debug( const char * dbgmsg, ... )
{
  static char buf[ DEBUG_MSG_MAX_LEN + 1 ];
  va_list args;

  va_start(args, dbgmsg);
  vsnprintf(buf, DEBUG_MSG_MAX_LEN, dbgmsg, args);
  buf[DEBUG_MSG_MAX_LEN] = '\0';
  va_end(args);

  Serial.print(buf);
}
#endif

// *****************************************************************************
// *                              IMPLEMENTATION                               *
// *****************************************************************************

void initLedBlinkTimer( int f )
{
  noInterrupts();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = (int) (34286.0 / f);

  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << OCIE1A);

  interrupts();
}


ISR( TIMER1_COMPA_vect )
{
  static bool st = false;

  st = !st;

  if ( g_blink_led1 )
    digitalWrite( FIRST_LED_PIN, st );

  if ( g_blink_led2 )
    digitalWrite( SECOND_LED_PIN, st );
}


int readFrequencyKHz( int pin )
{
  float t = 0.0;
  int Ht = 0;
  int Lt = 0;
  int i = 0;

  for ( i = 0; i < FREQUENCY_COUNTER_MAX_SAMPLES; i++ )
  {
    Ht = pulseIn( pin, HIGH, FREQUENCY_COUNTER_PULSE_TIMEOUT_US );
    Lt = pulseIn( pin, LOW, FREQUENCY_COUNTER_PULSE_TIMEOUT_US );
    t += Ht + Lt;
  }

  return ((int) (1000.0 / (t / FREQUENCY_COUNTER_MAX_SAMPLES)));
}


int autoCalibrateThreshold( void )
{
  int f1 = 0;
  int f2 = 0;

  g_blink_led1 = true;
  g_blink_led2 = true;

  for ( int i = 0; i < AUTO_CALIBRATE_SAMPLES_COUNT; i++ )
  {
    f1 += readFrequencyKHz( FIRST_OSCILLATOR_PIN );
    delay(100);

    f2 += readFrequencyKHz( SECOND_OSCILLATOR_PIN );
    delay(100);
  }

  g_fosc1_threshold = f1 / AUTO_CALIBRATE_SAMPLES_COUNT;
  g_fosc2_threshold = f2 / AUTO_CALIBRATE_SAMPLES_COUNT;

  g_fosc1_threshold += g_fosc1_threshold * THRESHOLD_VARIATION_AMMOUNT;
  g_fosc2_threshold += g_fosc2_threshold * THRESHOLD_VARIATION_AMMOUNT;

  g_blink_led1 = false;
  g_blink_led2 = false;
}


// *****************************************************************************
// *                               setup()                                     *
// *****************************************************************************

void setup( void )
{
  DEBUG_INIT();

  pinMode( FIRST_OSCILLATOR_PIN, INPUT );
  pinMode( SECOND_OSCILLATOR_PIN, INPUT );

  pinMode( FIRST_LED_PIN, OUTPUT );
  pinMode( SECOND_LED_PIN, OUTPUT );

  pinMode( FIRST_RELAY_PIN, OUTPUT );
  pinMode( SECOND_RELAY_PIN, OUTPUT );

  digitalWrite( FIRST_LED_PIN, LOW );
  digitalWrite( SECOND_LED_PIN, LOW );

  digitalWrite( FIRST_RELAY_PIN, HIGH );
  digitalWrite( SECOND_RELAY_PIN, HIGH );

  initLedBlinkTimer(5);
  autoCalibrateThreshold();
}


// *****************************************************************************
// *                                loop()                                     *
// *****************************************************************************

void loop( void )
{
  int f1 = readFrequencyKHz( FIRST_OSCILLATOR_PIN );
  int f2 = readFrequencyKHz( SECOND_OSCILLATOR_PIN );

  if ( f1 == 0 )
  {
    g_blink_led1 = true;
    digitalWrite( FIRST_RELAY_PIN, HIGH );
  }
  else if ( f1 >= g_fosc1_threshold )
  {
    g_blink_led1 = false;
    digitalWrite( FIRST_LED_PIN, HIGH );
    digitalWrite( FIRST_RELAY_PIN, LOW );
  }
  else if ( f1 < g_fosc1_threshold )
  {
    g_blink_led1 = false;
    digitalWrite( FIRST_LED_PIN, LOW );
    digitalWrite( FIRST_RELAY_PIN, HIGH );
  }

  if ( f2 == 0 )
  {
    g_blink_led2 = true;
    digitalWrite( SECOND_RELAY_PIN, HIGH );
  }
  else if ( f2 >= g_fosc2_threshold )
  {
    g_blink_led2 = false;
    digitalWrite( SECOND_LED_PIN, HIGH );
    digitalWrite( SECOND_RELAY_PIN, LOW );
  }
  else if ( f2 < g_fosc2_threshold )
  {
    g_blink_led2 = false;
    digitalWrite( SECOND_LED_PIN, LOW );
    digitalWrite( SECOND_RELAY_PIN, HIGH );
  }

  DEBUG( "%d,%d,%d,%d\n", f1, g_fosc1_threshold, f2, g_fosc2_threshold );

  delay(200);
}

// *****************************************************************************
// $Id: MetalKey.ino 598 2017-12-08 17:08:02Z tiago.ventura $
