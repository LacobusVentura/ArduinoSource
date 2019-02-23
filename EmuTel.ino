// *********************************************************************
// *      ███████╗███╗   ███╗██╗   ██╗████████╗███████╗██╗             *
// *      ██╔════╝████╗ ████║██║   ██║╚══██╔══╝██╔════╝██║             *
// *      █████╗  ██╔████╔██║██║   ██║   ██║   █████╗  ██║             *
// *      ██╔══╝  ██║╚██╔╝██║██║   ██║   ██║   ██╔══╝  ██║             *
// *      ███████╗██║ ╚═╝ ██║╚██████╔╝   ██║   ███████╗███████╗        *
// *      ╚══════╝╚═╝     ╚═╝ ╚═════╝    ╚═╝   ╚══════╝╚══════╝        *
// *********************************************************************
// *                     FABRICA DE ENIGMAS (c)                        *
// *                   Tiago Ventura - Nov/2017                        *
// *                    tiago.ventura@gmail.com                        *
// *********************************************************************

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>


// *********************************************************************
// *            DEVICES AND MODULES PIN CONFIGURATION                  *
// *********************************************************************

#define SD_CARD_CS_PIN                       (4)
#define BUZZER_PIN                           (7)
#define OFFHOOK_PIN                          (8)
#define AUDIO_OUTPUT_PIN                     (9)

#define PULSE_DECODER_TIMEOUT_MS             (200)
#define PULSE_DEBOUNCER_TIME_MS              (10)
#define ONHOOK_TIMEOUT_MS                    (600)

#define TELEPHONE_MAX_DIGITS                 (7)
#define TELEPHONE_MIN_DIGITS                 (3)

#define TELEPHONE_READY_TIMEOUT_MS           (10000)
#define TELEPHONE_BUSY_TIMEOUT_MS            (10000)
#define TELEPHONE_FINISHED_TIMEOUT_MS        (10000)
#define TELEPHONE_DIALING_TIMEOUT_MS         (5000)

const char TELEPHONE_WAIT_AUDIO_FILE[] =     "/SYSTEM/WAIT.WAV";
const char TELEPHONE_READY_AUDIO_FILE[] =    "/SYSTEM/READY.WAV";
const char TELEPHONE_BUSY_AUDIO_FILE[] =     "/SYSTEM/BUSY.WAV";
const char TELEPHONE_FINISHED_AUDIO_FILE[] = "/SYSTEM/FINISHED.WAV";
const char TELEPHONE_AUDIO_DIRECTORY[] =     "/AUDIO/";

const char extension[] =            ".WAV";
#define WAV_FILE_MAX_LEN         (20)

// *********************************************************************
// *                             TELEPHONE                             *
// *********************************************************************

TMRpcm wavplayer;

typedef enum sound_alert_e {
  soundOK,
  soundSDCardNotFound,
  soundSDCardCorrupted,
  soundWavePlayerError
} sound_alert_t;

typedef enum telephone_state_e {
  stateOnHook,
  stateLineReady,
  stateDialing,
  stateCalling,
  stateBusy,
  stateOnCall,
  stateFinished,
  stateDead
} telephone_state_t;

typedef struct telephone_s
{
  telephone_state_t previous_state;
  telephone_state_t state;
  unsigned long start_time;
  char wavfile[ WAV_FILE_MAX_LEN + 1 ];
  char digits[ TELEPHONE_MAX_DIGITS + 1 ];
  byte ndigits;
  boolean isready;
} telephone_t;


telephone_t telephone;

#define initializeTelephone()     do{ telephone.previous_state = stateDead; telephone.state = stateOnHook; telephone.start_time = millis(); } while(0)
#define changeTelephoneState( s ) do{ telephone.previous_state = telephone.state; telephone.state = s; telephone.start_time = millis(); } while(0)


// *********************************************************************
// *                               MISC                                *
// *********************************************************************
#define stop()   do{ delay(100); } while(1)


// *********************************************************************
// *                       DEBUG CONTROL                               *
// *********************************************************************

//#define ENABLE_SERIAL_DEBUG

#define DEBUG_SERIAL_BAUD_RATE    (9600)
#define DEBUG_MSG_MAX_LEN         (60)

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


// *********************************************************************
// *                      IMPLEMENTATION                               *
// *********************************************************************

void soundAlert( int alrt )
{
  switch( alrt )
  {
  case soundOK:
    {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(BUZZER_PIN, LOW);
      break;
    }

  case soundSDCardNotFound:
    {
      for( int i = 0; i < 2; i++ )
      {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
      }

      break;
    }

  case soundSDCardCorrupted:
    {
      for( int i = 0; i < 3; i++ )
      {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
      }

      break;
    }

  case soundWavePlayerError:
    {
      for( int i = 0; i < 4; i++ )
      {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(50);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
      }

      break;
    }

  default:
    {
      break;
    }

  }
}


boolean checkSDFileSystem( void )
{
  if(!SD.exists((char*)TELEPHONE_WAIT_AUDIO_FILE)) return false;
  if(!SD.exists((char*)TELEPHONE_READY_AUDIO_FILE)) return false;
  if(!SD.exists((char*)TELEPHONE_BUSY_AUDIO_FILE)) return false;
  if(!SD.exists((char*)TELEPHONE_FINISHED_AUDIO_FILE)) return false;
  if(!SD.exists((char*)TELEPHONE_AUDIO_DIRECTORY)) return false;

  return true;
}


void telephoneDialFirstDigit( byte digit )
{
  if( digit <= 0 )
    return;

  if( digit >= 10 )
    digit = 0;

  telephone.digits[0] = digit + '0';
  telephone.digits[1] = '\0';
  telephone.ndigits = 1;
}


void telephoneDialNextDigit( byte digit )
{
  byte i = 0;

  if( digit <= 0 )
    return;

  if( digit >= 10 )
    digit = 0;

  DEBUG("Digit Added: %d\n", digit );

  telephone.digits[ telephone.ndigits ] = digit + '0';
  telephone.digits[ telephone.ndigits + 1 ] = '\0';
  telephone.ndigits++;

  if( (telephone.ndigits == TELEPHONE_MAX_DIGITS) || (telephone.ndigits == TELEPHONE_MIN_DIGITS) )
  {
    for( i = 0; i < sizeof(TELEPHONE_AUDIO_DIRECTORY); i++ )
      telephone.wavfile[i] = TELEPHONE_AUDIO_DIRECTORY[i];

    for(i = 0; i < telephone.ndigits; i++ )
      telephone.wavfile[ i + sizeof(TELEPHONE_AUDIO_DIRECTORY) - 1] = telephone.digits[i];

    for(i = 0; i < sizeof(extension); i++ )
      telephone.wavfile[ i + sizeof(TELEPHONE_AUDIO_DIRECTORY) + telephone.ndigits - 1] = extension[i];

    telephone.isready = true;
  }
  else
  {
    telephone.isready = false;
  }
}


boolean isTelephoneOnHook( void )
{
  unsigned long t0 = millis();

  while( digitalRead( OFFHOOK_PIN ) == LOW )
  {
    if( millis() > t0 + ONHOOK_TIMEOUT_MS )
      return true;
  }

  return false;
}


int pulseDecoder( void )
{
  byte last_level = HIGH;
  byte pulse_counter = 0;
  int idle_time = 0;
  byte actual_level = 0;

  if( isTelephoneOnHook() )
    return -1;

  while( idle_time < PULSE_DECODER_TIMEOUT_MS )
  {
    actual_level = digitalRead( OFFHOOK_PIN );

    if( (actual_level != last_level) && (actual_level == LOW) )
    {
      pulse_counter++;
      idle_time = 0;
    }
    else
    {
      idle_time += PULSE_DEBOUNCER_TIME_MS;
    }

    last_level = actual_level;

    delay(PULSE_DEBOUNCER_TIME_MS);
  }

  return pulse_counter;
}


// *********************************************************************
// *                        setup()                                    *
// *********************************************************************

void setup(void)
{
  DEBUG_INIT();

  DEBUG( "*** EmuTel v1.00 (Emulador de Telefone)\n");
  DEBUG( "*** Compilado em %s %s\n\n", __DATE__, __TIME__ );

  pinMode( BUZZER_PIN, OUTPUT );
  digitalWrite( BUZZER_PIN, LOW );

  pinMode( OFFHOOK_PIN, INPUT );
  pinMode( AUDIO_OUTPUT_PIN, OUTPUT );

  wavplayer.speakerPin = AUDIO_OUTPUT_PIN;
  wavplayer.volume(5);

  initializeTelephone();

  DEBUG( "Inicializando Cartao SD..." );

  if(!SD.begin(SD_CARD_CS_PIN))
  {
    DEBUG("FALHOU!\n");
    soundAlert( soundSDCardNotFound );
    stop();
  }

  DEBUG("OK!\n");

  DEBUG( "Verificando Cartao SD..." );

  if(!checkSDFileSystem())
  {
    DEBUG("FALHOU!\n");
    soundAlert( soundSDCardCorrupted );
    stop();
  }

  DEBUG("OK!\n");

  DEBUG("EMUTEL pronto.\n\n");

  soundAlert( soundOK );
}


// *********************************************************************
// *                          loop()                                   *
// *********************************************************************
void loop(void)
{
  switch( telephone.state )
  {
  case stateOnHook:
    {
      if( telephone.previous_state != telephone.state )
      {
        DEBUG("stateOnHook\n");
        wavplayer.stopPlayback();
      }

      if(!isTelephoneOnHook())
      {
        changeTelephoneState( stateLineReady );
        break;
      }

      telephone.previous_state = stateOnHook;
      break;
    }

  case stateLineReady:
    {
      if( telephone.previous_state != telephone.state )
      {
        DEBUG("stateLineReady\n");
        wavplayer.play(TELEPHONE_READY_AUDIO_FILE);
      }

      if( millis() > telephone.start_time + TELEPHONE_READY_TIMEOUT_MS )
      {
        DEBUG("Timeout!\n");
        changeTelephoneState( stateFinished );
        break;
      }

      int digit = pulseDecoder();

      if( digit > 0 )
      {
        changeTelephoneState( stateDialing );
        telephoneDialFirstDigit( digit );
        break;
      }
      else if( digit < 0 )
      {
        changeTelephoneState( stateOnHook );
        break;
      }

      telephone.previous_state = stateLineReady;
      break;
    }

  case stateDialing:
    {
      if( telephone.previous_state != telephone.state )
      {
        DEBUG("stateDialing\n");
        wavplayer.stopPlayback();
      }

      if( millis() > telephone.start_time + TELEPHONE_DIALING_TIMEOUT_MS )
      {
        DEBUG("Timeout!\n");
        changeTelephoneState( stateFinished );
        break;
      }

      int digit = pulseDecoder();

      if( digit > 0 )
      {
        telephone.start_time = millis();

        telephoneDialNextDigit( digit );

        if( telephone.isready )
        {
          DEBUG("Checking %d digits phone number: %s\n", telephone.ndigits, telephone.digits );

          if( SD.exists(telephone.wavfile) )
          {
            DEBUG("File found: %s\n", telephone.wavfile );
            changeTelephoneState( stateCalling );
            break;
          }

          DEBUG("Number not available!\n" );

          if( telephone.ndigits == TELEPHONE_MAX_DIGITS )
          {
            changeTelephoneState( stateBusy );
            break;
          }
        }
      }
      else if( digit < 0 )
      {
        changeTelephoneState( stateOnHook );
        break;
      }

      telephone.previous_state = stateDialing;
      break;
    }

  case stateCalling:
    {
      if( telephone.previous_state != telephone.state )
      {
        DEBUG("stateCalling\n");
        wavplayer.play(TELEPHONE_WAIT_AUDIO_FILE);
      }

      if( millis() > telephone.start_time + TELEPHONE_DIALING_TIMEOUT_MS )
      {
        DEBUG("Call Answered!\n");
        changeTelephoneState( stateOnCall );
        break;
      }

      if(isTelephoneOnHook())
      {
        changeTelephoneState( stateOnHook );
        break;
      }

      delay(500);

      telephone.previous_state = stateCalling;
      break;
    }

  case stateBusy:
    {
      if( telephone.previous_state != telephone.state )
      {
        DEBUG("stateBusy\n");
        wavplayer.play(TELEPHONE_BUSY_AUDIO_FILE);
      }

      if( millis() > telephone.start_time + TELEPHONE_BUSY_TIMEOUT_MS )
      {
        DEBUG("Timeout!\n");
        changeTelephoneState( stateDead );
        break;
      }

      if(isTelephoneOnHook())
      {
        changeTelephoneState( stateOnHook );
        break;
      }

      delay(500);

      telephone.previous_state = stateBusy;
      break;
    }

  case stateOnCall:
    {
      if( telephone.previous_state != telephone.state )
      {
        DEBUG("stateOnCall\n");

        DEBUG("Playing file: %s\n", telephone.wavfile );
        wavplayer.play( telephone.wavfile );

        if( !wavplayer.isPlaying() )
        {
          DEBUG("Invalid or Corrupted Wave file!\n" );
          soundAlert( soundWavePlayerError );
          changeTelephoneState( stateFinished );
          break;
        }

      }

      if(isTelephoneOnHook())
      {
        changeTelephoneState( stateOnHook );
        break;
      }

      if(!wavplayer.isPlaying())
      {
        changeTelephoneState( stateFinished );
        break;
      }

      delay(500);

      telephone.previous_state = stateOnCall;
      break;
    }

  case stateFinished:
    {
      if( telephone.previous_state != telephone.state )
      {
        DEBUG("stateFinished\n");
        wavplayer.play(TELEPHONE_FINISHED_AUDIO_FILE);
      }

      if( millis() > telephone.start_time + TELEPHONE_FINISHED_TIMEOUT_MS )
      {
        DEBUG("Timeout!\n");
        changeTelephoneState( stateDead );
        break;
      }

      if(isTelephoneOnHook())
      {
        changeTelephoneState( stateOnHook );
        break;
      }

      delay(500);

      telephone.previous_state = stateFinished;
      break;
    }

  case stateDead:
    {
      if( telephone.previous_state != telephone.state )
      {
        DEBUG("stateDead\n");
        wavplayer.stopPlayback();
      }

      if(isTelephoneOnHook())
      {
        changeTelephoneState( stateOnHook );
        break;
      }

      delay(500);

      telephone.previous_state = stateDead;
      break;
    }

  default:
    {
      break;
    }
  }
}

// *********************************************************************
// $Id: EmuTel.ino 588 2017-11-22 16:02:12Z tiago.ventura $ //


