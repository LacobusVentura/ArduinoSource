// *****************************************************************************
// *                       Tiago Ventura - Fev/2021                            *
// *                        tiago.ventura@gmail.com                            *
// *****************************************************************************
#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>
#include <IRremote.h>

#ifndef __MUSIC_BLACK_BOX_H__
#define __MUSIC_BLACK_BOX_H__

// *****************************************************************************
// *                              IDLE MONITOR                                 *
// *****************************************************************************
class IdleMonitor
{
  public:
    typedef void (*idle_handler_t)(void);

  public:
    IdleMonitor(void) {
      this->m_timeout = 0;
      this->m_last_activity = 0;
      this->m_idle_handler = NULL;
    }

    void tick(void) {
      if (this->m_timeout > 0) {
        if (millis() - this->m_last_activity > this->m_timeout) {
          this->on_idle_detected();
          this->m_last_activity = millis();
        }
      }
    }

    void begin(idle_handler_t handler, unsigned long timeout) {
      this->m_idle_handler = handler;
      this->m_timeout = timeout;
    }

    void set_activity(void) {
      this->m_last_activity = millis();
    }

  private:
    void on_idle_detected(void) {
      Serial.println(F("[IDLE]: Triggered!"));
      if (this->m_idle_handler)
        this->m_idle_handler();
    }

  private:
    unsigned long m_timeout;
    unsigned long m_last_activity;
    idle_handler_t m_idle_handler;
};

// *****************************************************************************
// *                                  BUZZER                                   *
// *****************************************************************************
class Buzzer
{
  public:
    Buzzer(byte pin_buzzer) {
      this->m_buzzer_pin = pin_buzzer;
      pinMode(this->m_buzzer_pin, OUTPUT);
    }

    void sos(void) const {
      this->encode(B01110000);
      this->encode(B01110111);
      this->encode(B01110000);
    }

    void invalid_command(void) const {
      this->encode(B00110011);
    }

    void fatal_error(void) const {
      this->encode(B11111111);
    }

    void system_ready(void) const {
      this->encode(B00010000);
    }

    void data_written(void) const {
      this->encode(B01110000);
    }

    void infrared(void) const {
      this->encode(B00110000);
    }

  private:
    void sound(unsigned long t) {
      digitalWrite(this->m_buzzer_pin, HIGH);
      delay(t);
      digitalWrite(this->m_buzzer_pin, LOW);
    }

    void encode(byte code) {
      for (byte i = 0; i < 4; i++) {
        if (code & (0x10 << i)) {
          if (code & (0x01 << i))
            this->sound(300);
          else
            this->sound(100);
          delay(50);
        }
      }
    }

  private:
    byte m_buzzer_pin;
};

// *****************************************************************************
// *                             IR RECEIVER                                   *
// *****************************************************************************
class InfraRedReceiver
{
  public:
    typedef void (*receiver_handler_t)(uint32_t);

  public:
    InfraRedReceiver(void) {
      this->m_pin_infrared_rx = 0;
      this->m_receiver_handler = NULL;
    }

    void begin(byte pin_infrared_rx, receiver_handler_t handler) {
      this->m_pin_infrared_rx = pin_infrared_rx;
      this->m_receiver_handler = handler;
      this->m_irrecv.begin(this->m_pin_infrared_rx, DISABLE_LED_FEEDBACK);
    }

    void tick(void) {
      if (this->m_irrecv.decode()) {
        this->m_irrecv.resume();
        if (this->m_irrecv.decodedIRData.protocol != UNKNOWN)
          this->on_receive();
        delay(100);
      }
    }

  private:
    void on_receive() {
      Serial.print(F("[IR] proto="));
      Serial.print(getProtocolString(this->m_irrecv.decodedIRData.protocol));
      Serial.print(F(" cmd=0x"));
      Serial.print(this->m_irrecv.decodedIRData.command, HEX);
      Serial.print(F(" addr=0x"));
      Serial.print(this->m_irrecv.decodedIRData.address, HEX);
      Serial.print(F(" raw=0x"));
      Serial.println(this->m_irrecv.decodedIRData.decodedRawData, HEX);

      if (this->m_receiver_handler)
        this->m_receiver_handler(this->m_irrecv.decodedIRData.command);
    }

  private:
    IRrecv m_irrecv;
    byte m_pin_infrared_rx;
    receiver_handler_t m_receiver_handler;
};

// *********************************************************************
// *                        PANEL LEDS                                 *
// *********************************************************************
class PanelLeds
{
  public:
    PanelLeds(void) {
      this->m_actual_state = 0;
      this->m_pin_latch_cs = 0;
      this->m_pin_latch_clock = 0;
      this->m_pin_latch_data = 0;
    }

    void begin(byte pin_latch_cs, byte pin_latch_clock, byte pin_latch_data) {
      this->m_pin_latch_cs = pin_latch_cs;
      this->m_pin_latch_clock = pin_latch_clock;
      this->m_pin_latch_data = pin_latch_data;
      pinMode(this->m_pin_latch_cs, OUTPUT);
      pinMode(this->m_pin_latch_clock, OUTPUT);
      pinMode(this->m_pin_latch_data, OUTPUT);
      this->set_data(0x00);
    }

    void set_data(byte value) {
      digitalWrite(this->m_pin_latch_cs, LOW);
      shiftOut(this->m_pin_latch_data, this->m_pin_latch_clock, MSBFIRST, value);
      digitalWrite(this->m_pin_latch_cs, HIGH);
      this->m_actual_state = value;
    }

    byte set_led(byte id, bool state) {
      if (state)
        this->m_actual_state |= (0x01 << id - 1);
      else
        this->m_actual_state &= ~(0x01 << id - 1);
      this->set_data(this->m_actual_state);
    }

  private:
    byte m_actual_state;
    byte m_pin_latch_cs;
    byte m_pin_latch_clock;
    byte m_pin_latch_data;
};

// *****************************************************************************
// *                               KEYPAD                                      *
// *****************************************************************************
class ButtonKeyPad
{
  public:
    typedef void (*key_press_handler_t)(byte);

  public:
    ButtonKeyPad(void) {
      this->m_pin_latch_cs = 0;
      this->m_pin_latch_clock = 0;
      this->m_pin_latch_data = 0;
      this->m_last_state = 0;
      this->m_press_start = 0;
      this->m_button_pressed = false;
      this->m_long_press_handler = NULL;
      this->m_short_press_handler = NULL;
    }

    void begin(key_press_handler_t short_handler, key_press_handler_t long_handler,
               byte pin_latch_cs, byte pin_latch_clock, byte pin_latch_data, byte pin_line_d0, byte pin_line_d1) {
      this->m_long_press_handler = long_handler;
      this->m_short_press_handler = short_handler;
      this->m_pin_latch_cs = pin_latch_cs;
      this->m_pin_latch_clock = pin_latch_clock;
      this->m_pin_latch_data = pin_latch_data;
      this->m_pin_line_d0 = pin_line_d0;
      this->m_pin_line_d1 = pin_line_d1;
      pinMode(this->m_pin_latch_cs, OUTPUT);
      pinMode(this->m_pin_latch_clock, OUTPUT);
      pinMode(this->m_pin_latch_data, OUTPUT);
      pinMode(this->m_pin_line_d0, INPUT);
      pinMode(this->m_pin_line_d1, INPUT);
    }

    void tick(void) {
      byte current = this->key_id(this->read_state());
      if (this->m_button_pressed && (millis() - this->m_press_start >= 2000)) {
        this->on_long_key_press(this->m_last_state);
        this->m_button_pressed = false;
      }
      else if (!this->m_button_pressed && !this->m_last_state && current) {
        this->m_press_start = millis();
        this->m_button_pressed = true;
      }
      else if (this->m_button_pressed && this->m_last_state && !current) {
        this->on_short_key_press(this->m_last_state);
        this->m_button_pressed = false;
      }
      this->m_last_state = current;
    }

  private:
    byte read_state(void) const {
      byte value = 0;
      byte state = 0;
      for (byte n = 0; n < 4; n++) {
        value = 1 << n;
        digitalWrite(this->m_pin_latch_cs, LOW);
        shiftOut(this->m_pin_latch_data, this->m_pin_latch_clock, MSBFIRST, ~value);
        digitalWrite(this->m_pin_latch_cs, HIGH);
        if (!digitalRead(this->m_pin_line_d0))
          state |= value;
        if (!digitalRead(this->m_pin_line_d1))
          state |= (value << 4);
      }
      return state;
    }

    byte key_id(byte data) const {
      for (byte i = 0; i <= 8; i++)
        if (data == (1 << i))
          return i + 1;
      return 0;
    }

    void on_long_key_press(byte id_key) {
      Serial.print(F("[KEY-LONG]: "));
      Serial.println(id_key);
      if (this->m_long_press_handler)
        this->m_long_press_handler(id_key);
    }

    void on_short_key_press(byte id_key) {
      Serial.print(F("[KEY-SHORT]: "));
      Serial.println(id_key);
      if (this->m_short_press_handler)
        this->m_short_press_handler(id_key);
    }

  private:
    byte m_pin_latch_cs;
    byte m_pin_latch_clock;
    byte m_pin_latch_data;
    byte m_pin_line_d0;
    byte m_pin_line_d1;
    byte m_last_state;
    unsigned long m_press_start;
    bool m_button_pressed;
    key_press_handler_t m_long_press_handler;
    key_press_handler_t m_short_press_handler;
};

// *****************************************************************************
// *                               WAV PLAYER                                  *
// *****************************************************************************
class SDCardAudioFilePlayer
{
  public:
    typedef void (*event_handler_t)(byte, byte);

  public:
    SDCardAudioFilePlayer(void) {
      this->m_is_playing = false;
      this->m_id_album = -1;
      this->m_id_track = -1;
      memset(this->m_filename, 0, sizeof(this->m_filename));
    }

    bool is_playing(void) const {
      return this->m_is_playing;
    }
    byte get_id_album(void) const {
      return this->m_id_album;
    }
    byte get_id_track(void) const {
      return this->m_id_track;
    }
    const char * get_filename(void) const {
      return this->m_filename;
    }

    byte begin(byte pin_sdcard_cs, byte pin_audio_out, byte volume = 5) {
      if (!SD.begin(pin_sdcard_cs))
        return 1;
      this->m_id_album = EEPROM.read(0x00);
      Serial.print(F("[CFG-LOAD]: "));
      Serial.println(this->m_id_album);
      this->m_wavplayer.speakerPin = pin_audio_out;
      this->m_wavplayer.setVolume(volume);
      return 0;
    }

    void tick(void) {
      if (!this->m_is_playing && this->m_wavplayer.isPlaying()) {
        this->on_play();
        this->m_is_playing = true;
      }
      else if (this->m_is_playing && !this->m_wavplayer.isPlaying()) {
        this->on_stop();
        this->m_is_playing = false;
      }
    }

    void set_id_album(byte id_album) {
      if (this->m_id_album != id_album) {
        EEPROM.write(0x00, id_album);
        Serial.print(F("[CFG-SAVE]: "));
        Serial.println(id_album);
      }
      this->m_id_album = id_album;
    }

    void play(byte id_track) {
      sprintf(this->m_filename, "A%d/T%d.WAV", this->m_id_album, id_track);
      this->m_id_track = id_track;
      this->m_wavplayer.play(this->m_filename);
      this->tick();
    }

    void stop(void) {
      this->m_wavplayer.stopPlayback();
      this->tick();
    }

    void set_on_play_handler(event_handler_t handler) {
      this->m_on_play_handler = handler;
    }

    void set_on_stop_handler(event_handler_t handler) {
      this->m_on_stop_handler = handler;
    }

  private:
    void on_play(void) {
      Serial.print(F("[PLAY] album="));
      Serial.print(this->m_id_album);
      Serial.print(F(" track="));
      Serial.print(this->m_id_track);
      Serial.print(F(" wav="));
      Serial.println(this->m_filename);
      if (this->m_on_play_handler)
        this->m_on_play_handler(this->m_id_album, this->m_id_track);
    }

    void on_stop(void) {
      Serial.print(F("[STOP] album="));
      Serial.print(this->m_id_album);
      Serial.print(F(" track="));
      Serial.print(this->m_id_track);
      Serial.print(F(" wav="));
      Serial.println(this->m_filename);
      if (this->m_on_stop_handler)
        this->m_on_stop_handler(this->m_id_album, this->m_id_track);
    }

  private:
    TMRpcm m_wavplayer;
    bool m_is_playing;
    char m_filename[16];
    byte m_id_album;
    byte m_id_track;
    event_handler_t m_on_play_handler;
    event_handler_t m_on_stop_handler;
};

#endif

/* end-of-file */
