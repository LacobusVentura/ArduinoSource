/*
	FABRICA DE ENIGMAS (c)
	Autor: Tiago Ventura (tiago.ventura@gmail.com)
	Data:  Mar/2018
*/

#include <Arduino.h>
#include <LedControl.h>
#include <EEPROM.h>

/* 8x8 LED Matrix Pins */
const byte PIN_DISPLAY_CS = 3;
const byte PIN_DISPLAY_CLK = 4;
const byte PIN_DISPLAY_DATA_IN = 2;

/* Encoder Pins */
const byte PIN_ENCODER_D0 = A0;
const byte PIN_ENCODER_D1 = A1;
const byte PIN_ENCODER_D2 = A2;
const byte PIN_ENCODER_REF = A3;

/* Speaker Pins */
const byte PIN_SPEAKER = A4;

/* Button Pins */
const byte PIN_BUTTON_DISPLAY_CTRL = 8;
const byte PIN_BUTTON_RECORDING_CTRL = 7;
const byte PIN_BUTTON_SOUND_CTRL = 6;

/* LED Pins */
const byte PIN_LED_RECORDING = 10;
const byte PIN_LED_SOUND_ENABLED = 9;
const byte PIN_LED_LOCKED = 12;
const byte PIN_LED_UNLOCKED = 11;

/* Password Max Length */
const byte PASSWORD_MAX_LENGTH = 128;

/* Rudder Directions */
enum { none, left, right, change_left, change_right };

/* GLOBALS */
byte g_sound_enabled = 1;
byte g_display_enabled = 1;
byte g_locked = 1;
byte g_recording_mode = 0;
byte g_step = 0;
int g_actual = -1;


/* Combination */
byte password[ PASSWORD_MAX_LENGTH ]; // = { 0, 7, 6, 5, 4, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 3, 2, 1, 0, 7, 6 };
byte passwordlen = 0; //21

byte password_buf[ PASSWORD_MAX_LENGTH ] = {0};
byte password_buflen = 0;


LedControl g_display( PIN_DISPLAY_DATA_IN, PIN_DISPLAY_CLK, PIN_DISPLAY_CS, 1 );


uint64_t getCharFont8x8( char c )
{
  switch ( c )
  {
    case '0': return 0x3c66666e76663c00;
    case '1': return 0x7e1818181c181800;
    case '2': return 0x7e060c3060663c00;
    case '3': return 0x3c66603860663c00;
    case '4': return 0x30307e3234383000;
    case '5': return 0x3c6660603e067e00;
    case '6': return 0x3c66663e06663c00;
    case '7': return 0x1818183030667e00;
    case '!': return 0x180018183c3c1800;
    case '?': return 0x1800183860663c00;
    case '*': return 0x105438ee38541000;
    case 'L': return 0x7e06060606060600;
    case 'R': return 0x66361e3e66663e00;
    case 'O': return 0x3c66666666663c00;
    case 'E': return 0x7e06063e06067e00;
    case 'S': return 0x3c66603c06663c00;
    case 'C': return 0x3c66060606663c00;
    case 'B': return 0x3e66663e66663e00;
    default : return 0x0000000000000000;
  }
}


void readConfig( void )
{
  g_sound_enabled = EEPROM.read( 0x0 );
  g_display_enabled = EEPROM.read( 0x1 );
  passwordlen = EEPROM.read( 0x2 );
  EEPROM.get(  0x3, password );
}


void writeConfig( void )
{
  EEPROM.write( 0x0, g_sound_enabled );
  EEPROM.write( 0x1, g_display_enabled );
  EEPROM.write( 0x2, passwordlen );
  EEPROM.put( 0x3, password );
}


byte reverse( byte b )
{
  byte rev = 0;
  int i = 0;

  for (i = 0; i < 8; i++)
    if ((b & (1 << i)))
      rev |= 1 << (7 - i);

  return rev;
}


void displayImage( uint64_t image )
{
  for ( int i = 0; i < 8; i++ )
  {
    byte row = (image >> (i * 8)) & 0xFF;
    g_display.setRow( 0, i, reverse(row) );
  }
}


void displayCombination( void )
{
  int i = 0;
  char * pwd = translateCombination();

  tone( PIN_SPEAKER, 1000, 50 );
  delay(100);
  tone( PIN_SPEAKER, 1000, 50 );

  while ( pwd[i] )
  {
    displayImage( getCharFont8x8( pwd[i++] ) );
    delay(1000);
  }

  displayImage(0);
  delay(1000);

  displayImage( getCharFont8x8('?') );
}


void setup( void )
{
  readConfig();

  g_display.shutdown( 0, false );
  g_display.setIntensity( 0, 1 );
  g_display.clearDisplay( 0 );

  if ( digitalRead( PIN_BUTTON_RECORDING_CTRL ) )
    displayCombination();

  pinMode( PIN_ENCODER_D0, INPUT );
  pinMode( PIN_ENCODER_D1, INPUT );
  pinMode( PIN_ENCODER_D2, INPUT );
  pinMode( PIN_ENCODER_REF, INPUT );

  pinMode( PIN_SPEAKER, OUTPUT );

  pinMode( PIN_BUTTON_RECORDING_CTRL, INPUT );
  pinMode( PIN_BUTTON_SOUND_CTRL, INPUT );
  pinMode( PIN_BUTTON_DISPLAY_CTRL, INPUT );

  pinMode( PIN_LED_SOUND_ENABLED, OUTPUT );
  pinMode( PIN_LED_LOCKED, OUTPUT );
  pinMode( PIN_LED_UNLOCKED, OUTPUT );
  pinMode( PIN_LED_RECORDING, OUTPUT );

  digitalWrite( PIN_LED_SOUND_ENABLED, g_sound_enabled );
  digitalWrite( PIN_LED_LOCKED, g_locked );
  digitalWrite( PIN_LED_UNLOCKED, g_locked ? 0 : 1 );
  digitalWrite( PIN_LED_RECORDING, g_recording_mode );

  tone( PIN_SPEAKER, 1000, 50 );

  g_display.shutdown( 0, g_display_enabled ? false : true );
  displayImage( getCharFont8x8('?') );
}


byte readEncoder( void )
{
  byte data = 0;

  data |= ( digitalRead( PIN_ENCODER_D0 ) << 0 );
  data |= ( digitalRead( PIN_ENCODER_D1 ) << 1 );
  data |= ( digitalRead( PIN_ENCODER_D2 ) << 2 );

  return data;
}


byte isReadyEncoder( void )
{
  if ( digitalRead( PIN_ENCODER_REF ) == HIGH )
    return true;

  return false;
}

void soundClick( void )
{
  if ( g_sound_enabled == 0 )
    return;

  for ( int i = 0; i < 2; i++ )
  {
    digitalWrite( PIN_SPEAKER, HIGH ); delay(10);
    digitalWrite( PIN_SPEAKER, LOW ); delay(40);
  }
}


void soundUnlock( void )
{
  if ( g_sound_enabled == 0 )
    return;

  for ( int i = 0; i < 4; i++ )
  {
    digitalWrite( PIN_SPEAKER, HIGH ); delay(10);
    digitalWrite( PIN_SPEAKER, LOW ); delay(100);
  }
}


bool checkCombination( byte * buf, int len )
{
  byte turnleft[9] = {0, 1, 2, 3, 4, 5, 6, 7, 0};
  byte turnright[9] = {0, 7, 6, 5, 4, 3, 2, 1, 0};
  byte nzeros = 0;

  /* Combinacao muito grande */
  if ( len >= PASSWORD_MAX_LENGTH )
    return false;

  /* Primeiro numero da combinacao eh sempre 0 */
  if ( buf[ 0 ] != 0 )
    return false;

  /* Ultimo numero da combinacao mao pode ser 0 */
  if ( buf[ len - 1 ] == 0 )
    return false;

  /* Calcula a quantidade minima de zeros na combinacao */
  for ( byte i = 0; i < len; i++ )
    if ( buf[i] == 0 )
      nzeros++;

  /* Combinacoes com menos de 3 zeros sao invalidas */
  if ( nzeros < 3 )
    return false;

  /* verifica integridade da sequencia da combinacao */
  for ( byte i = 0; i < len - 1; i++ )
  {
    byte actual = buf[ i ];
    byte next = buf[ i + 1 ];

    /* A combinacao soh possui numeros entre 0 e 7 */
    if ( (actual > 7) || (next > 7) )
      return false;

    /* Combinacao eh uma sequencia */
    if ( (actual != next - 1) && (actual != next + 1) )
      if ( (actual != 7 && next != 0) && (actual != 0 && next != 7) )
        return false;
  }

  /* Volta completa para esquerda nao eh permitida */
  if ( memmem( buf, len, turnleft, 9 ) )
    return false;

  /* Volta completa para a direta nao eh permitida */
  if ( memmem( buf, len, turnright, 9 ) )
    return false;

  /* combinacao ok */
  return true;
}


const char * translateCombination( void )
{
  static char txt[ 128 ];
  int dir = none;
  byte j = 0;

  if ( !checkCombination( password, passwordlen ) )
    return "?!?!?!";

  for ( int i = 1; i < passwordlen; i++ )
  {
    int actual = password[ i ];
    int prev = password[ i - 1 ];

    if ( (prev == 0) && (actual == 7) )
    {
      txt[ j++ ] = 'R';
      dir = change_right;
    }
    else if ( (prev == 0) && (actual == 1) )
    {
      txt[ j++ ] = 'L';
      dir = change_left;
    }
    else if ( (prev == 7) && (actual == 0) )
    {
      if (dir == change_right)
        txt[ j++ ] = '7';

      dir = none;
    }
    else if ( (prev == 1) && (actual == 0) )
    {
      if (dir == change_left)
        txt[ j++ ] = '1';

      dir = none;
    }
    else if ( (prev < actual) && (dir == right) )
    {
      txt[ j++ ] = prev + '0';
      dir = change_left;
    }
    else if ( (prev > actual) && (dir == left) )
    {
      txt[ j++ ] = prev + '0';
      dir = change_right;
    }
    else if ( (prev < actual) && (dir == change_left) )
    {
      dir = left;
    }
    else if ( (prev > actual) && (dir == change_right) )
    {
      dir = right;
    }
  }

  txt[ j++ ] = password[ passwordlen - 1 ] + '0';
  txt[ j ] = 0;

  return txt;
}


void operate( void )
{
  byte data = readEncoder();

  if ( isReadyEncoder() && (data != g_actual) )
  {
    displayImage( getCharFont8x8( data + '0') );

    soundClick();

    g_actual = data;
    g_locked = 1;

    if ( password[ g_step++ ] == data )
    {
      if ( g_step == passwordlen )
      {
        displayImage( getCharFont8x8('*') );
        soundUnlock();
        g_step = 0;
        g_locked = 0;
      }
    }
    else
    {
      g_step = 0;
    }
  }
}


int record( void )
{
  byte data = readEncoder();

  if ( isReadyEncoder() && (data != g_actual) )
  {
    displayImage( getCharFont8x8( data + '0') );

    soundClick();

    g_actual = data;

    password_buf[ g_step++ ] = data;
    password_buflen = g_step;

    if ( password_buflen == PASSWORD_MAX_LENGTH )
      return -1;
  }

  return 0;
}


void loop( void )
{
  if ( digitalRead(PIN_BUTTON_SOUND_CTRL) )
  {
    g_sound_enabled = g_sound_enabled ? 0 : 1;
    digitalWrite( PIN_LED_SOUND_ENABLED, g_sound_enabled );
    EEPROM.write( 0x0, g_sound_enabled );
    delay(300);
  }

  if ( digitalRead( PIN_BUTTON_DISPLAY_CTRL ) )
  {
    g_display_enabled = g_display_enabled ? 0 : 1;
    g_display.shutdown( 0, g_display_enabled ? false : true );
    EEPROM.write( 0x1, g_display_enabled );
    delay(300);
  }

  if ( g_recording_mode == 0 )
  {
    if ( (digitalRead( PIN_BUTTON_RECORDING_CTRL ) == HIGH)  && (g_recording_mode == 0) )
    {
      if ( g_actual != 0 )
      {
        tone( PIN_SPEAKER, 100, 50 );
      }
      else
      {
        g_recording_mode = 1;
        g_step = 1;
        password_buf[0] = 0;
        password_buflen = 1;

        digitalWrite( PIN_LED_RECORDING, HIGH );
        tone( PIN_SPEAKER, 1000, 50 );
      }

      delay(300);
    }

    digitalWrite( PIN_LED_LOCKED, g_locked );
    digitalWrite( PIN_LED_UNLOCKED, g_locked ? 0 : 1 );

    operate();
  }
  else
  {
    if ( (digitalRead( PIN_BUTTON_RECORDING_CTRL ) == HIGH)  && (g_recording_mode == 1) )
    {
      if ( checkCombination( password_buf, password_buflen ) )
      {
        memcpy( password, password_buf, password_buflen );
        passwordlen = password_buflen;

        EEPROM.write( 0x2, passwordlen );
        EEPROM.put( 0x3, password );

        tone( PIN_SPEAKER, 1000, 50 );
        delay(100);
        tone( PIN_SPEAKER, 1000, 50 );

        displayCombination();
      }
      else
      {
        tone( PIN_SPEAKER, 100, 50 );
      }

      g_recording_mode = 0;
      g_step = 0;

      digitalWrite( PIN_LED_RECORDING, LOW );
      delay(300);
    }

    int ret = record();

    if ( ret != 0 )
    {
      tone( PIN_SPEAKER, 100, 50 );

      g_recording_mode = 0;
      g_step = 0;

      digitalWrite( PIN_LED_RECORDING, LOW );
    }
  }
}

/* $Id$ */
