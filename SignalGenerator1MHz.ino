#include <TimerOne.h>
#include <ClickEncoder.h>
#include <SPI.h>
#include <Wire.h>
#include <MD_AD9833.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define PIN_OLED_MOSI           (9)
#define PIN_OLED_CLK            (10)
#define PIN_OLED_DC             (11)
#define PIN_OLED_CS             (12)
#define PIN_OLED_RESET          (13)

#define PIN_SIG_GEN_DATA        (8)  
#define PIN_SIG_GEN_CLK         (7)  
#define PIN_SIG_GEN_FSYNC       (6)  

#define PIN_ENCODER_SWITCH      (2)
#define PIN_ENCODER_CLK         (3)
#define PIN_ENCODER_DATA        (4)

#define PIN_SPEAKER             (5)

#define SCREEN_WIDTH            (128)
#define SCREEN_HEIGHT           (64)

#define AD_MAX_FREQUENCY        (1000000L)  // 1 MHz
#define AD_MIN_FREQUENCY        (1L)        // 1 Hz

#define AD_MAX_AMPLITUDE        (20.0f)
#define AD_MIN_AMPLITUDE        (0.0f)


Adafruit_SSD1306 display( SCREEN_WIDTH, SCREEN_HEIGHT, PIN_OLED_MOSI, PIN_OLED_CLK, PIN_OLED_DC, PIN_OLED_RESET, PIN_OLED_CS );
MD_AD9833  AD( PIN_SIG_GEN_DATA, PIN_SIG_GEN_CLK, PIN_SIG_GEN_FSYNC );
ClickEncoder encoder( PIN_ENCODER_CLK, PIN_ENCODER_DATA, PIN_ENCODER_SWITCH, 2 );

static const int wave_shape[] = { MD_AD9833::MODE_SINE, MD_AD9833::MODE_TRIANGLE, MD_AD9833::MODE_SQUARE1 };
static const char * wave_shape_name[] = { "SINE", "TRIANGLE", "SQUARE" };
static const long scale_value[] = { 1, 10, 100, 1000, 10000, 100000 };
static const char * scale_name[] = { "x1", "x10", "x100","x1,000","x10,000","x100,000" };
//static const char * scale_name[] = { "x1Hz", "x10Hz", "x100Hz","x1KHz","x10KHz","x100KHz" };

int16_t encoder_last_value = -1;
int16_t encoder_value = 0;
long frequency = 1000;
char mode = 0;
char opt = 0;
char scale = 0;
double amplitude = 5.0;


void callbackTimer1Func( void )
{
   encoder.service();
}

void setup( void )
{
  Serial.begin(9600);

  pinMode( PIN_SPEAKER, OUTPUT );
  digitalWrite( PIN_SPEAKER, LOW ); 
  
  if(!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.println("SSD1306 allocation failed");

    tone(PIN_SPEAKER,4000,200);
    delay(300);
    tone(PIN_SPEAKER,4000,200);

    for(;;);
  }

  encoder.setAccelerationEnabled(true);

  AD.begin();
  AD.setMode(wave_shape[mode]);
  AD.setFrequency( MD_AD9833::CHAN_0, frequency ); 

  Timer1.initialize(1000);
  Timer1.attachInterrupt(callbackTimer1Func);

  tone(PIN_SPEAKER,4000);
  delay(100);
  noTone(PIN_SPEAKER);

  display.display();
}

void refresh_status( void )
{
  display.clearDisplay();
  
  display.setTextColor(WHITE); 
  display.cp437(true);       

  display.drawRoundRect(0, 0, 128, 16, 5, WHITE );
  display.setTextColor(WHITE); 
  
  display.setCursor(4, 5);   
  display.print("Freq: ");
  display.print(frequency);
  display.print(" Hz");

  if( opt == 1 )
  {
    display.fillRoundRect(0, 16, 128, 15, 5, WHITE );
    display.setTextColor(BLACK); 
  }
  else
  {
    display.drawRoundRect(0, 16, 128, 15, 5, WHITE );
    display.setTextColor(WHITE); 
  }

  display.setCursor(4, 20);   
  display.print("Wave: ");
  display.print(wave_shape_name[mode]);


  if( opt == 2 )
  {
    display.fillRoundRect(0, 32, 128, 15, 5, WHITE );
    display.setTextColor(BLACK); 
  }
  else
  {
    display.drawRoundRect(0, 32, 128, 15, 5, WHITE );
    display.setTextColor(WHITE); 
  }

  display.setCursor(4 , 36 );   
  display.print("Scale: ");
  display.print(scale_name[scale]);

  if( opt == 3 )
  {
    display.fillRoundRect(0, 48, 128, 15, 5, WHITE );
    display.setTextColor(BLACK); 
  }
  else
  {
    display.drawRoundRect(0, 48, 128, 15, 5, WHITE );
    display.setTextColor(WHITE); 
  }
  
  display.setCursor(4, 52);   
  display.print("Amplitude: ");
  display.print(amplitude);
  display.print("Vpp");

  display.display();
}

void loop( void )
{   
    encoder_value = encoder.getValue();

    if( encoder_value != encoder_last_value )
    {
      if( opt == 0 )
      {
        if( (frequency == 1) && (scale > 0) && (encoder_value > 0) )
          frequency--;
        
        frequency += encoder_value * scale_value[scale];
      
        if (frequency > AD_MAX_FREQUENCY) frequency = AD_MAX_FREQUENCY;
        if (frequency < AD_MIN_FREQUENCY) frequency = AD_MIN_FREQUENCY; 
      
        Serial.print("Frequency: ");
        Serial.println(frequency);
    
        AD.setFrequency( MD_AD9833::CHAN_0, frequency );
        
      }
      else if( opt == 1 )
      {
        if( encoder_value > 0 ) mode++;
        if( encoder_value < 0 ) mode--;
     
        if( mode > 2 ) mode = 0;
        if( mode < 0 ) mode = 2;
      
        Serial.print("Shape=");
        Serial.println(wave_shape_name[mode]);

        AD.setMode( wave_shape[mode] );
      }
      else if( opt == 2 )
      {
        if( encoder_value > 0 ) scale++;
        if( encoder_value < 0 ) scale--;
     
        if( scale > 5 ) scale = 0;
        if( scale < 0 ) scale = 5;
      
        Serial.print("Scale=");
        Serial.println( scale_name[scale] );
      }
      else if( opt == 3 )
      {
        amplitude += encoder_value * (AD_MAX_AMPLITUDE / 100);
      
        if (amplitude > AD_MAX_AMPLITUDE) amplitude = AD_MAX_AMPLITUDE;
        if (amplitude < AD_MIN_AMPLITUDE) amplitude = AD_MIN_AMPLITUDE; 
      
        Serial.print("Amplitude: ");
        Serial.println(amplitude);
      }
  
      tone(PIN_SPEAKER,5000,1);
      
      encoder_last_value = encoder_value;
      refresh_status();
  }
  
  ClickEncoder::Button b = encoder.getButton();
  
  if( b != ClickEncoder::Open )
  {
    tone( PIN_SPEAKER, 5000, 1 );
    
    Serial.print("Button: ");
    
    switch (b)
    {
      case ClickEncoder::Clicked:
          Serial.println("ClickEncoder::Clicked");
  
          opt++;

          if( opt > 3 )
            opt = 0;

          Serial.print("Option=");
          Serial.println(opt);

          refresh_status();
      break;
/*
      case ClickEncoder::Released: Serial.println("ClickEncoder::Released"); break;
      case ClickEncoder::Pressed: Serial.println("ClickEncoder::Pressed"); break;
      case ClickEncoder::Held: Serial.println("ClickEncoder::Held"); break;
      case ClickEncoder::DoubleClicked: Serial.println("ClickEncoder::DoubleClicked"); break;
*/          
      default:
      break;
    }
  } 
}




