#include <LedControl.h>
#include <RGBLed.h>
#include <IRremote.h>

/* RNG Seed */
const byte PIN_RNG_SEED = A7;

/* RGB Power LED */
const byte PIN_RGB_LED_RED = 3;
const byte PIN_RGB_LED_GREEN = 5;
const byte PIN_RGB_LED_BLUE = 6;

/* Panel Keys */
const byte PIN_KEY_RED = A0;
const byte PIN_KEY_GREEN = A1;
const byte PIN_KEY_BLUE = A2;
const byte PIN_KEY_YELLOW = A3;

/* Panel LEDs */
const byte PIN_LED_RED = 2;
const byte PIN_LED_GREEN = 4;
const byte PIN_LED_BLUE = 8;
const byte PIN_LED_YELLOW = 9;

/* 8x8 Matrix Displays */
const byte PIN_DISPLAY_DIN = 12;
const byte PIN_DISPLAY_CLK = 11;
const byte PIN_DISPLAY_CS = 10;

/* Speaker */
const byte PIN_FTE = A5;

/* InfraRed Comm */
const byte PIN_IR_RX = 7;
const byte PIN_IR_TX = A4;

/* RGB LED Colors */
const byte COLOR_OFF     = 0;
const byte COLOR_RED     = 1;
const byte COLOR_GREEN   = 2;
const byte COLOR_BLUE    = 3;
const byte COLOR_YELLOW  = 4;

RGBLed powerled = RGBLed(PIN_RGB_LED_RED, PIN_RGB_LED_GREEN, PIN_RGB_LED_BLUE, RGBLed::COMMON_CATHODE);
LedControl matrix = LedControl(PIN_DISPLAY_DIN, PIN_DISPLAY_CLK, PIN_DISPLAY_CS, 2);
IRrecv infrared_recv = IRrecv(PIN_IR_RX);

const uint64_t ASCII_TABLE[] PROGMEM = {
  0x0000000000000000, 0x0000000000000000, 0x7c92aa82aa827c00, 0x7ceed6fed6fe7c00, 
  0x10387cfefeee4400, 0x10387cfe7c381000, 0x381054fe54381000, 0x38107cfe7c381000,
  0x00387c7c7c380000, 0xffc7838383c7ffff, 0x0000000000000000, 0x0c12129ca0c0f000,
  0x107c103844443800, 0x0000000000000000, 0x066eecc88898f000, 0x105438ee38541000,
  0x061e7efe7e1e0600, 0xc0f0fcfefcf0c000, 0x1038541054381000, 0x6666006666666600,
  0x0000000000000000, 0x0000000000000000, 0x000000ffff000000, 0xc0c0c0c0c0c0ffff,
  0x383838fe7c381000, 0x10387cfe38383800, 0x10307efe7e301000, 0x1018fcfefc181000,
  0xffff030303030303, 0x0000000000000000, 0xfefe7c7c38381000, 0x1038387c7cfefe00,
  0x0000000000000000, 0x180018183c3c1800, 0x000000006c6c6c00, 0x6c6cfe6cfe6c6c00,
  0x103c403804781000, 0x60660c1830660600, 0xfc66a6143c663c00, 0x0000000018181800,
  0x6030181818306000, 0x060c1818180c0600, 0x006c38fe386c0000, 0x0010107c10100000,
  0x060c0c0c00000000, 0x0000003c00000000, 0x0606000000000000, 0x00060c1830600000,
  0x3c66666e76663c00, 0x7e1818181c181800, 0x7e060c3060663c00, 0x3c66603860663c00,
  0x30307e3234383000, 0x3c6660603e067e00, 0x3c66663e06663c00, 0x1818183030667e00,
  0x3c66663c66663c00, 0x3c66607c66663c00, 0x0018180018180000, 0x0c18180018180000,
  0x6030180c18306000, 0x00003c003c000000, 0x060c1830180c0600, 0x1800183860663c00,
  0x003c421a3a221c00, 0x6666667e66663c00, 0x3e66663e66663e00, 0x3c66060606663c00,
  0x3e66666666663e00, 0x7e06063e06067e00, 0x0606063e06067e00, 0x3c66760606663c00,
  0x6666667e66666600, 0x3c18181818183c00, 0x1c36363030307800, 0x66361e0e1e366600,
  0x7e06060606060600, 0xc6c6c6d6feeec600, 0xc6c6e6f6decec600, 0x3c66666666663c00,
  0x06063e6666663e00, 0x603c766666663c00, 0x66361e3e66663e00, 0x3c66603c06663c00,
  0x18181818185a7e00, 0x7c66666666666600, 0x183c666666666600, 0xc6eefed6c6c6c600,
  0xc6c66c386cc6c600, 0x1818183c66666600, 0x7e060c1830607e00, 0x7818181818187800,
  0x006030180c060000, 0x1e18181818181e00, 0x0000008244281000, 0xff00000000000000,
  0x0000000060303000, 0x7c667c603c000000, 0x3e66663e06060600, 0x3c6606663c000000,
  0x7c66667c60606000, 0x3c067e663c000000, 0x0c0c3e0c0c6c3800, 0x3c607c66667c0000,
  0x6666663e06060600, 0x3c18181800180000, 0x1c36363030003000, 0x66361e3666060600,
  0x1818181818181800, 0xd6d6feeec6000000, 0x6666667e3e000000, 0x3c6666663c000000,
  0x06063e66663e0000, 0xf0b03c36363c0000, 0x060666663e000000, 0x3e403c027c000000,
  0x1818187e18180000, 0x7c66666666000000, 0x183c666600000000, 0x7cd6d6d6c6000000,
  0x663c183c66000000, 0x3c607c6666000000, 0x3c0c18303c000000, 0x7018180c18187000,
  0x1818181818181818, 0x0e18183018180e00, 0x000000365c000000, 0x0000000000000000
};

uint64_t matrix_get_charset_img(char ch)
{
  uint64_t img = 0;
  memcpy_P(&img, &ASCII_TABLE[ch], 8);
  return img;
}

const uint64_t ANIMATION[8] = {
  0x0000001818000000,
  0x00003c24243c0000,
  0x007e424242427e00,
  0xff818181818181ff,
  0x007e424242427e00,
  0x00003c24243c0000,
  0x0000001818000000,
  0x0000000000000000
};

const uint64_t ICONS[10] = {
  0x0000000000000000, 0x181818ffff181818, 0xc3e77e3c3c7ee7c3, 0xffffc3c3c3c3ffff, 0x3c7ee7c3c3e77e3c,
  0x0000000000000000, 0x6624a5ff3c7e5a7e, 0x083e7efe1e0e0b0e, 0x7dfebe848e151f11, 0x66447c7c7c870704
};

void matrix_print_image(byte id, uint64_t image) {
  for (int i = 0; i < 8; i++) {
    byte row = (image >> i * 8) & 0xFF;
    for (int j = 0; j < 8; j++)
      matrix.setLed(id, j, 7 - i, bitRead(row, j));
  }
}

void matrix_print(byte id, char ch) {
  matrix_print_image(id, matrix_get_charset_img(ch));
}

void display_setup() {
  for (int i = 0; i < 2; i++) {
    matrix.shutdown(i, false);
    matrix.setIntensity(i, 5);
    matrix.clearDisplay(i);
  }
}

void powerled_set_color(byte color) {
  switch (color) {
    case COLOR_OFF:    powerled.setColor(0, 0, 0); break;
    case COLOR_RED:    powerled.setColor(255, 0, 0); break;
    case COLOR_GREEN:  powerled.setColor(0, 255, 0); break;
    case COLOR_BLUE:   powerled.setColor(0, 0, 255); break;
    case COLOR_YELLOW: powerled.setColor(255, 128, 0); break;
    default: break;
  }
}


void powerled_fadein_color(byte color, int steps, int duration) {
  switch (color) {
    case COLOR_OFF:    powerled.fadeIn(0, 0, 0, steps, duration);break;
    case COLOR_RED:    powerled.fadeIn(255, 0, 0, steps, duration);break;
    case COLOR_GREEN:  powerled.fadeIn(0, 255, 0, steps, duration);break;
    case COLOR_BLUE:   powerled.fadeIn(0, 0, 255, steps, duration);break;
    case COLOR_YELLOW: powerled.fadeIn(255, 128, 0, steps, duration);break;
    default: break;
  }
}

void powerled_fadeout_color(byte color, int steps, int duration) {
  switch (color) {
    case COLOR_OFF:    powerled.fadeOut(0, 0, 0, steps, duration); break;
    case COLOR_RED:    powerled.fadeOut(255, 0, 0, steps, duration);break;
    case COLOR_GREEN:  powerled.fadeOut(0, 255, 0, steps, duration);break;
    case COLOR_BLUE:   powerled.fadeOut(0, 0, 255, steps, duration);break;
    case COLOR_YELLOW: powerled.fadeOut(255, 128, 0, steps, duration);break;
    default: break;
  }
}

void panel_set_led(byte color) {
  digitalWrite(PIN_LED_RED, color == COLOR_RED);
  digitalWrite(PIN_LED_GREEN, color == COLOR_GREEN);
  digitalWrite(PIN_LED_BLUE, color == COLOR_BLUE);
  digitalWrite(PIN_LED_YELLOW, color == COLOR_YELLOW);
}

bool panel_check_key(byte color) {
  switch (color) {
    case COLOR_RED:    return digitalRead(PIN_KEY_RED);
    case COLOR_GREEN:  return digitalRead(PIN_KEY_GREEN);
    case COLOR_BLUE:   return digitalRead(PIN_KEY_BLUE);
    case COLOR_YELLOW: return digitalRead(PIN_KEY_YELLOW);
    default:           return LOW;
  }
}

byte panel_get_keyboard_status() {
  byte ret = 0;
  ret |= (!digitalRead(PIN_KEY_RED)) ? COLOR_RED : COLOR_OFF;
  ret |= (!digitalRead(PIN_KEY_GREEN)) ? COLOR_GREEN : COLOR_OFF;
  ret |= (!digitalRead(PIN_KEY_BLUE)) ? COLOR_BLUE : COLOR_OFF;
  ret |= (!digitalRead(PIN_KEY_YELLOW)) ? COLOR_YELLOW : COLOR_OFF;
  return ret;
}

void setup_rng() {
  int a, b;
  do {
    a = analogRead(PIN_RNG_SEED); delay(25);
    b = analogRead(PIN_RNG_SEED); delay(25);
  } while (a == b);
  randomSeed(a ^ b);
}

void setup_pins() {
  pinMode(PIN_RNG_SEED, INPUT);
  pinMode(PIN_FTE, OUTPUT);
  pinMode(PIN_IR_TX, OUTPUT);
  pinMode(PIN_KEY_RED, INPUT_PULLUP);
  pinMode(PIN_KEY_GREEN, INPUT_PULLUP);
  pinMode(PIN_KEY_BLUE, INPUT_PULLUP);
  pinMode(PIN_KEY_YELLOW, INPUT_PULLUP);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
}

void setup()
{
  setup_pins();
  setup_rng();
  display_setup();
  tone(PIN_FTE, 1500, 10);
}

void play_coin() {
  tone(PIN_FTE, 988, 100);
  delay(100);
  tone(PIN_FTE, 1319, 250);
}

void play_oneup() {
  tone(PIN_FTE, 1319, 125); delay(130);
  tone(PIN_FTE, 1568, 125); delay(130);
  tone(PIN_FTE, 2637, 125); delay(130);
  tone(PIN_FTE, 2093, 125); delay(130);
  tone(PIN_FTE, 2349, 125); delay(130);
  tone(PIN_FTE, 3136, 125);
}

void play_error() {
  tone(PIN_FTE, 250, 100);
  delay(150);
  tone(PIN_FTE, 250, 200);
}

int get_random_color(byte skip_color = COLOR_OFF) {
  static byte last_color = COLOR_OFF;
  byte color = COLOR_OFF;
  do {
    color = (random(100) % 4) + 1;
  } while (color == last_color || color == skip_color);
  last_color = color;
  return color;
}

void screen_saver(byte color) {
/*
  matrix_print(0, 'Z');
  matrix_print(1, 'z');
  powerled_fadeout_color(color, 10, 500);
  powerled_fadein_color(color, 10, 500);

  matrix_print(0, 'z');
  matrix_print(1, 'Z');
  powerled_fadeout_color(color, 10, 500);
  powerled_fadein_color(color, 10, 500);
*/
  for(int i = 0; i < 8; i++)
  {
    matrix_print_image(0, ANIMATION[i]);
    delay(100);
  }
}

byte skip_color = COLOR_OFF;
byte score = 0;
byte buttonState = 0;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long last_action = millis();
byte lastButtonState = 0;

void loop() {
  byte color = get_random_color(skip_color);

  panel_set_led(color);
  powerled_set_color(color);
  matrix_print_image(0, ICONS[color]);
  matrix_print_image(1, ICONS[color + 5]);

  while (true)
  {
    byte reading = panel_get_keyboard_status();

    if (reading != lastButtonState)
      lastDebounceTime = millis();

    if (millis() > last_action + 10000) {
      screen_saver(color);
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {

      if (reading != buttonState) {
        buttonState = reading;

        if (buttonState == color) {
          play_coin();
          score += 1;
          if(score==10){
            score = 0;
            play_oneup();
          }
          last_action = millis();
          break;
        } else if (buttonState != COLOR_OFF) {
          play_error();
          skip_color = buttonState;
          score = 0;
          last_action = millis();
          break;
        }
      }
    }
    lastButtonState = reading;
  }
}
