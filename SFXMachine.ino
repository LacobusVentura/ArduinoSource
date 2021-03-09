// *********************************************************************
// *                   Tiago Ventura - Mar/2021                        *
// *                    tiago.ventura@gmail.com                        *
// *********************************************************************
#include "MusicBlackBox.h"

// *********************************************************************
// *            DEVICES AND MODULES PIN CONFIGURATION                  *
// *********************************************************************
#define PIN_SD_CARD_CS                         (10)
#define PIN_BUZZER                             (2)
#define PIN_AUDIO_OUTPUT                       (9)
#define PIN_INFRARED_RX                        (8)

#define PIN_KEYPAD_LINE_D0                     (3)
#define PIN_KEYPAD_LINE_D1                     (4)

#define PIN_LATCH_KEYPAD_CLK                   (A5)
#define PIN_LATCH_KEYPAD_CS                    (A4)
#define PIN_LATCH_KEYPAD_DATA                  (A3)

#define PIN_LATCH_LEDS_CLK                     (A2)
#define PIN_LATCH_LEDS_CS                      (A1)
#define PIN_LATCH_LEDS_DATA                    (A0)

#define AUDIO_VOLUME                           (5)
#define KEYPAD_LONG_PRESS                      (2000)
#define IDLE_TIME_LIMIT                        (300000)

// *********************************************************************
// *                         DEVICES                                   *
// *********************************************************************
IdleMonitor idlemon;
SDCardAudioFilePlayer wavplayer;
ButtonKeyPad keypad;
InfraRedReceiver irrecv;
PanelLeds leds;
Buzzer buzzer(PIN_BUZZER);

// *********************************************************************
// *                        HANDLERS                                   *
// *********************************************************************
void idle_handler(void) {
  for (byte n = 0; n < 6; n++){
    for (byte i = 0; i < 4; i++){
      leds.set_data((n % 2) ? 1 << i : 128 >> i); delay(20);
      leds.set_data(0); delay(40);
    }
  }
  buzzer.sos();
}

void player_on_play_handler(byte id_album, byte id_track) {
  leds.set_led(id_track, true);
  idlemon.set_activity();
}

void player_on_stop_handler(byte id_album, byte id_track) {
  leds.set_led(id_track, false);
  idlemon.set_activity();
}

void infrared_receiver_handler(uint32_t cmd) {
  byte id_track = (cmd % 8) + 1;
  if (wavplayer.is_playing() && (id_track == wavplayer.get_id_track())) {
    wavplayer.stop();
  }
  else if (wavplayer.is_playing() && (id_track != wavplayer.get_id_track())) {
    wavplayer.stop();
    wavplayer.play(id_track);
  }
  else if (!wavplayer.is_playing()) {
    wavplayer.play(id_track);
  }

  idlemon.set_activity();
}

void keypad_on_long_key_press(byte key) {
  if (!wavplayer.is_playing()) {
    wavplayer.set_id_album(key);
    leds.set_led(key, true);
    buzzer.data_written();
    leds.set_led(key, false);
  } else {
    buzzer.invalid_command();
  }
  idlemon.set_activity();
}

void keypad_on_short_key_press(byte key) {
  if (wavplayer.is_playing() && (key == wavplayer.get_id_track())) {
    wavplayer.stop();
  }
  else if (wavplayer.is_playing() && (key != wavplayer.get_id_track())) {
    wavplayer.stop();
    wavplayer.play(key);
  }
  else if (!wavplayer.is_playing()) {
    wavplayer.play(key);
  }
  idlemon.set_activity();
}

// *********************************************************************
// *                        setup()                                    *
// *********************************************************************
void setup(void)
{
  /* SERIAL MONITOR */
  Serial.begin(115200);

  /* IDLE MONITOR */
  idlemon.begin(idle_handler, IDLE_TIME_LIMIT);

  /* INFRARED RECEIVER  */
  irrecv.begin(infrared_receiver_handler, PIN_INFRARED_RX);

  /* PANEL LED */
  leds.begin(PIN_LATCH_LEDS_CS, PIN_LATCH_LEDS_CLK, PIN_LATCH_LEDS_DATA);

  /* PANEL KEYPAD */
  keypad.begin(keypad_on_short_key_press, keypad_on_long_key_press,
    PIN_LATCH_KEYPAD_CS, PIN_LATCH_KEYPAD_CLK, PIN_LATCH_KEYPAD_DATA,
     PIN_KEYPAD_LINE_D0, PIN_KEYPAD_LINE_D1);

  /* SD CARD AUDIO FILE PLAYER */
  if (wavplayer.begin(PIN_SD_CARD_CS, PIN_AUDIO_OUTPUT, AUDIO_VOLUME)) {
    buzzer.fatal_error();
    while (true) {
      leds.set_data(0x99); delay(50);
      leds.set_data(0x00); delay(500);
    }
  }

  wavplayer.set_on_play_handler(player_on_play_handler);
  wavplayer.set_on_stop_handler(player_on_stop_handler);

  buzzer.system_ready();
  
  leds.set_led(wavplayer.get_id_album(), true);
  delay(250);
  leds.set_led(wavplayer.get_id_album(), false);

  Serial.println(F("-----<[ Muzik 3lack B0x v1.00 ]>-----"));
}

// *********************************************************************
// *                          loop()                                   *
// *********************************************************************
void loop(void) {
  irrecv.tick();
  wavplayer.tick();
  keypad.tick();
  idlemon.tick();
}
// *********************************************************************
// end-of-file
