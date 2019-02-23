/*
	FABRICA DE ENIGMAS (c)
	Autor: Tiago Ventura (tiago.ventura@gmail.com)
	Data:  Abril/2018
*/

#include <Arduino.h>

int g_config_timeout = 0;
int g_config_counter = 0;


/* Call Bell Push Button */
const byte PIN_CALL_BELL = 10;

/* LEDs */
const byte PIN_LED_WAITING = 11;
const byte PIN_LED_ENABLED = 12;

/* Relay */
const byte PIN_RELAY = A0;

/* Counter DIP Switch */
const byte PIN_DIP_SWITCH_COUNTER_Q0 = 2;
const byte PIN_DIP_SWITCH_COUNTER_Q1 = 3;
const byte PIN_DIP_SWITCH_COUNTER_Q2 = 4;
const byte PIN_DIP_SWITCH_COUNTER_Q3 = 5;

/* Timeout DIP Switch */
const byte PIN_DIP_SWITCH_TIMEOUT_Q0 = 6;
const byte PIN_DIP_SWITCH_TIMEOUT_Q1 = 7;
const byte PIN_DIP_SWITCH_TIMEOUT_Q2 = 8;
const byte PIN_DIP_SWITCH_TIMEOUT_Q3 = 9;


int readTimeout( void )
{
  byte t = 0;

  t |= ( digitalRead( PIN_DIP_SWITCH_TIMEOUT_Q3 ) ? 0 : 1 ) << 0;
  t |= ( digitalRead( PIN_DIP_SWITCH_TIMEOUT_Q2 ) ? 0 : 1 ) << 1;
  t |= ( digitalRead( PIN_DIP_SWITCH_TIMEOUT_Q1 ) ? 0 : 1 ) << 2;
  t |= ( digitalRead( PIN_DIP_SWITCH_TIMEOUT_Q0 ) ? 0 : 1 ) << 3;

  return (t + 1) * 1000;
}


byte readCounter( void )
{
  byte n = 0;

  n |= ( digitalRead( PIN_DIP_SWITCH_COUNTER_Q3 ) ? 0 : 1 ) << 0;
  n |= ( digitalRead( PIN_DIP_SWITCH_COUNTER_Q2 ) ? 0 : 1 ) << 1;
  n |= ( digitalRead( PIN_DIP_SWITCH_COUNTER_Q1 ) ? 0 : 1 ) << 2;
  n |= ( digitalRead( PIN_DIP_SWITCH_COUNTER_Q0 ) ? 0 : 1 ) << 3;

  return n + 1;
}


void setup( void )
{
  pinMode( PIN_CALL_BELL, INPUT_PULLUP );
  pinMode( PIN_LED_WAITING, OUTPUT );
  pinMode( PIN_LED_ENABLED, OUTPUT );
  pinMode( PIN_RELAY, OUTPUT );

  digitalWrite( PIN_RELAY, HIGH );
  digitalWrite( PIN_LED_WAITING, LOW );
  digitalWrite( PIN_LED_ENABLED, LOW );

  pinMode( PIN_DIP_SWITCH_COUNTER_Q0, INPUT_PULLUP );
  pinMode( PIN_DIP_SWITCH_COUNTER_Q1, INPUT_PULLUP );
  pinMode( PIN_DIP_SWITCH_COUNTER_Q2, INPUT_PULLUP );
  pinMode( PIN_DIP_SWITCH_COUNTER_Q3, INPUT_PULLUP );

  pinMode( PIN_DIP_SWITCH_TIMEOUT_Q0, INPUT_PULLUP );
  pinMode( PIN_DIP_SWITCH_TIMEOUT_Q1, INPUT_PULLUP );
  pinMode( PIN_DIP_SWITCH_TIMEOUT_Q2, INPUT_PULLUP );
  pinMode( PIN_DIP_SWITCH_TIMEOUT_Q3, INPUT_PULLUP );

  g_config_timeout = readTimeout();
  g_config_counter = readCounter();

  Serial.begin(9600);
}

bool machineState = false;
bool buttonState = false;
int lastMachineState = false;
long t0 = 0;
const int debounceDelay = 50;
int count = 0;
bool enabled = false;

void loop( void )
{
  int reading;
  long t = millis();

  if( (t - t0) > g_config_timeout )
  {
     if( enabled )
     {
        count = 0;
        enabled = false;
     }
     else if( (enabled == false) && (g_config_counter == count) )
     {
        enabled = true;
     }
     else
     {
        count = 0;
     }

     t0 = t;
  }

  if( (t - t0) > debounceDelay )
  {
    reading = !digitalRead( PIN_CALL_BELL );

    if( reading != buttonState )
    {
      buttonState = reading;

      if( buttonState == LOW )
      {
        t0 = t;
        machineState = !machineState;
      }
    }
  }

  if( lastMachineState != machineState )
  {
    enabled = false;
    count++;
  }

  lastMachineState = machineState;

  digitalWrite( PIN_LED_WAITING, count > 0 );
  digitalWrite( PIN_RELAY, !enabled );
  digitalWrite( PIN_LED_ENABLED, enabled );

  Serial.print( " count=" );
  Serial.print( count );

  Serial.print( " timeout=" );
  Serial.print( (count > 0) ? g_config_timeout - (t - t0) : 0 );

  Serial.print( " enabled=" );
  Serial.println( enabled );
}
