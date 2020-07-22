#include <EEPROM.h>
#include <SPI.h>
#include "U8glib.h"
                      //     sda       dc
                      //sck, mosi, cs, a0, reset
//U8GLIB_SH1106_128X64 u8g(13, 11,   10, 9,  8);
                       //cs, a0, reset //mosi/sda = 11 sck = 13
U8GLIB_SH1106_128X64 u8g(A4, 6,  7);

enum {BufSize=9};
char buf[BufSize];

const byte midi_mode_pin_1 = A3;
const byte midi_mode_pin_2 = A5;
byte midi_mode;
byte prev_midi_mode;
int rotational_note;
const byte cs_pins[3] = { 8, 8, 9 };
const byte dacchns[3] = { 0, 1, 0 };
const byte gates[3]   = { A0, A1, A2 };
byte notes[3]         = { 255, 255, 255 };
bool gatesstate[3]    = { false, false, false };
int envelopes[3]      = { 0, 0, 0 };
const byte env_cs_pins[3] = { 9,10,10 };
const byte env_dacchns[3] = { 1, 0, 1 };
int peak[3]           = { 4095, 4095, 4095 };
int sustain[3]        = { 4095, 4095, 4095 };
bool peaked[3]        = { false, false, false };
int attack = 7;
int decay = 7;
int min_sustain = 7;
int release = 7;
int attack_increment = 0;
int attack_rate = 0;
int decay_increment = 0;
int decay_rate = 0;
int min_sustain_level = 0;
int release_increment = 0;
int release_rate = 0;
unsigned long last_event[3];

const byte reset_pin = 5;

const byte cs_screen = A4;

const byte encoder_clock = 2;
const byte encoder_data = 3;
const byte encoder_switch = 4;
bool encoder_changed;
unsigned long encoder_held;
bool prev_switch_state;
bool testing_mode = false;
int testing_voltage;

int menu_position = 0;
byte num_outputs;

//MIDI Out
byte midi_channel = 0;
const byte note_off                 = 0 << 4;
const byte note_on                  = 1 << 4;
const byte velocity_on              = 90;
const byte velocity_off             = 0;
byte status_byte;
byte data_byte1;
byte data_byte2;
byte playing_note[16];
byte midi_drums[] = { 35, 38, 45, 46 };
//MIDI IN
bool mi_midi_gate;
byte mi_input_byte;
byte mi_status_byte;
byte mi_data_byte1;
byte mi_data_byte2;
byte mi_channel;
byte mi_key;
byte mi_prev_key;
bool mi_channel_status;
bool mi_prev_channel_status;

byte mi_code;

const byte mi_note_off                 = 0;
const byte mi_note_on                  = 1;
const byte poly_pressure            = 2;
const byte controller_change        = 3;
const byte program_change           = 4;
const byte channel_pressure_change  = 5;
const byte pitch_bend_change        = 6;

// channel mode messages
const byte channel_volume           = 7;
// channel mode messages that cause all notes to switch off
const byte sound_off                = 120;
const byte reset_controllers        = 121;
const byte all_notes_off            = 123;
const byte omni_on                  = 124;
const byte omni_off                 = 125;
const byte mono_on                  = 126;
const byte poly_on                  = 127;

const byte clock_code               = 248;
const byte stop_clock               = 252;
const byte reset_all                = 255;
// \midi variables


unsigned int test_value;
int gate_high;
unsigned long last_changed_gate;
int change_gate_interval = 1000;

//const unsigned int midi_voltages[] PROGMEM = { 250,333,417,500,583,667,750,833,917,0,83,167,250,333,417,500,583,667,750,833,917,0,83,167,250,333,417,500,583,667,750,833,917,1000,1083,1167,1250,1333,1417,1500,1583,1667,1750,1833,1917,2000,2083,2167,2250,2333,2417,2500,2583,2667,2750,2833,2917,3000,3083,3167,3250,3333,3417,3500,3583,3667,3750,3833,3917,4000,4083,3167,3250,3333,3417,3500,3583,3667,3750,3833,3917,4000,4083,3167,3250,3333,3417,3500,3583,3667,3750,3833,3917,4000,4083,3167,3250,3333,3417,3500,3583,3667,3750,3833,3917,4000,4083,3167,3250,3333,3417,3500,3583,3667,3750,3833,3917,4000,4083,3167,3250,3333,3417,3500,3583,3667,3750,3833 };
const unsigned int midi_voltages[] PROGMEM = { 83,167,250,333,417,500,583,667,750,833,917,0,83,167,250,333,417,500,583,667,750,833,917,0,83,167,250,333,417,500,583,667,750,833,917,0,83,167,250,333,417,500,583,667,750,833,917,1000,1083,1167,1250,1333,1417,1500,1583,1667,1750,1833,1917,2000,2083,2167,2250,2333,2417,2500,2583,2667,2750,2833,2917,3000,3083,3167,3250,3333,3417,3500,3583,3667,3750,3833,3917,4000,4083,3167,3250,3333,3417,3500,3583,3667,3750,3833,3917,4000,4083,3167,3250,3333,3417,3500,3583,3667,3750,3833,3917,4000,4083,3167,3250,3333,3417,3500,3583,3667,3750,3833,3917,4000,4083,3167,3250,3333,3417,3500,3583,3667 };

//                        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15
int increments[16] = {    1,   2,   4,   8,  16,  32,  64, 128, 256, 512, 700, 900,1024,2048,3072,4095 };
int rates[16] =      { 4095,3072,2048,1024, 900, 700, 512, 256, 128,  64,  32,  16,   8,   4,   2,   1 };
int levels[16] =     {    1,   2,   4,   8,  16,  32,  64, 128, 256, 512, 700, 900,1024,2048,3072,4095 };

void setup() {
  
  Serial.begin(31250);

  u8g.setFont(u8g_font_unifont);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  for(int i=0; i<3; i++){
    pinMode(cs_pins[i], OUTPUT);
    digitalWrite(cs_pins[i], HIGH);
    pinMode(env_cs_pins[i], OUTPUT);
    digitalWrite(env_cs_pins[i], HIGH);
  }
  pinMode(cs_screen, OUTPUT);
  digitalWrite(cs_screen, HIGH);
  for(int i=0; i<3; i++){
    pinMode(gates[i], OUTPUT);
    digitalWrite(gates[i], HIGH);
  }

  pinMode(reset_pin, INPUT_PULLUP);

  pinMode(encoder_clock, INPUT_PULLUP);
  pinMode(encoder_data, INPUT_PULLUP);
  pinMode(encoder_switch, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoder_clock), encoder_function, FALLING);

  pinMode(midi_mode_pin_1, INPUT_PULLUP);
  pinMode(midi_mode_pin_2, INPUT_PULLUP);

  num_outputs = EEPROM.read(0);
  midi_channel = EEPROM.read(1);
  num_outputs = min(2, num_outputs);
  midi_channel = min(15, midi_channel);

  attack = EEPROM.read(2);
  decay = EEPROM.read(3);
  min_sustain = EEPROM.read(4);
  release = EEPROM.read(5);
  attack = min(15, attack);
  decay = min(15, decay);
  min_sustain = min(15, min_sustain);
  release = min(15, release);

  calculate_envelopes();

  encoder_changed = true;

  for(int i=0; i<3; i++){
    setOutputChip(0, i);
    setOutputChipEnv(0, i);
  }

  delay(1000); //to stop encoder button reading as triggered
}

void encoder_function(){
  bool switch_state = bitRead(PIND, 4);
  
  if(bitRead(PIND, 3)){
    //left
    if(testing_mode){
      testing_voltage++;
      if(testing_voltage > 4){
        testing_voltage = 0;
      }
    }else if(!switch_state){
      menu_position++;
    }else if(menu_position==0){
      midi_channel++;
      if(midi_channel>15){
        midi_channel = 0;
      }
      reset_notes();
    }else if(menu_position==1){
      num_outputs++;
      if(num_outputs>2){
        num_outputs = 0;
      }
      reset_notes();
    }else if(menu_position==2){
      attack++;
      if(attack>15){
        attack = 0;
      }
    }else if(menu_position==3){
      decay++;
      if(decay>15){
        decay = 0;
      }
    }else if(menu_position==4){
      min_sustain++;
      if(min_sustain>15){
        min_sustain = 0;
      }
    }else if(menu_position==5){
      release++;
      if(release>15){
        release = 0;
      }
    }
  }else{
    //right
    if(testing_mode){
      testing_voltage--;
      if(testing_voltage < 0){
        testing_voltage = 4;
      }
    }else if(!switch_state){
      menu_position--;
    }else if(menu_position==0){
      if(midi_channel > 0){
        midi_channel--;
      }else{
        midi_channel = 15;
      }
      reset_notes();
    }else if(menu_position==1){
      if(num_outputs>0){
        num_outputs--;
        //turn off unused outputs with note > 255
        reset_notes();
      }else{
        num_outputs = 2;
      }
    }else if(menu_position==2){
      attack--;
      if(attack<0){
        attack = 15;
      }
    }else if(menu_position==3){
      decay--;
      if(decay<0){
        decay = 15;
      }
    }else if(menu_position==4){
      min_sustain--;
      if(min_sustain<0){
        min_sustain = 15;
      }
    }else if(menu_position==5){
      release--;
      if(release<0){
        release = 15;
      }
    }
  }

  if(testing_mode){
    for(int i=0; i<3; i++){
      setOutputChip(1000 * testing_voltage, i);
    }
    delay(100);
  }

  calculate_envelopes();

  if(menu_position > 5){
    menu_position = 0;
  }else if(menu_position < 0){
    menu_position = 5;
  }

  EEPROM.write(0, num_outputs);
  EEPROM.write(1, midi_channel);
  EEPROM.write(2, attack);
  EEPROM.write(3, decay);
  EEPROM.write(4, min_sustain);
  EEPROM.write(5, release);

  encoder_held = 0;
  
  encoder_changed = true;
}

void calculate_envelopes(){

  attack_increment = increments[attack];
  attack_rate = rates[attack];
  decay_increment = increments[decay];
  decay_rate = rates[decay];
  min_sustain_level = levels[min_sustain];
  release_increment = increments[release];
  release_rate = rates[release];
  
}

void loop() {

  unsigned long current_millis = micros();

  bool switch_state = !bitRead(PIND, 5); //reset button
  if(switch_state != prev_switch_state){
      encoder_held = millis();
  }
  prev_switch_state = switch_state;
  if(switch_state && millis() - encoder_held > 2000){
      encoder_held = millis();
      testing_mode = !testing_mode;
      encoder_changed = true;

      testing_voltage = 0;

      for(int i=0; i<3; i++){
        if(testing_mode){
          setOutputChip(1000 * testing_voltage, i);
          setOutputChipEnv(4095, i);
          gatesstate[i] = true;
        }else{
          setOutputChip(0, i);
          setOutputChipEnv(0, i);
          gatesstate[i] = false;
        }
      }
  }

  /*for(byte i=0; i<4; i++){
      setOutputChip(test_value, i);
  }
  test_value++;
  if(test_value > 4095){
     test_value = 0;
  }
  if(current_millis - last_changed_gate > change_gate_interval){
      last_changed_gate = current_millis;
      for(byte i=0; i<4; i++){
          digitalWrite(gates[i], HIGH);
      }
      digitalWrite(gates[gate_high], LOW);
      gate_high++;
      if(gate_high>3){
         gate_high = 0;
      }
  }*/

  if(!bitRead(PIND, 5)){

      reset_notes();
  
  }else if (Serial.available() > 0 && !testing_mode) {
    
      mi_input_byte = Serial.read();
      
      if(bitRead(mi_input_byte, 7)){
          mi_status_byte = mi_input_byte;
          mi_code = (mi_status_byte & B01110000) >> 4;
          mi_channel = mi_status_byte & B00001111;
          mi_data_byte1 = 0;
          mi_data_byte2 = 0;

          //reset all
          if(mi_status_byte == reset_all){
              //turn off all notes on all channels
              reset_notes();
          }
          
      }else if(mi_data_byte1==0){
          mi_data_byte1 = mi_input_byte;

          //if status byte matches single data byte messages, do something and reset here
          if(mi_code > 1){
              //reset data bytes;
              mi_data_byte1 = 0;
              mi_data_byte2 = 0;
          }
          
      }else if(mi_channel == midi_channel){
          mi_data_byte2 = mi_input_byte;

          if(mi_code == mi_note_on){
              mi_key = mi_data_byte1;
              
              if(mi_data_byte2 > 0){
                  // turn on specific note
                  midi_note_on(mi_key, mi_data_byte2);
              }else{
                  //turn off specific note
                  midi_note_off(mi_key);
              }
          }else if(mi_code == mi_note_off){
              mi_midi_gate = false;
              mi_key = mi_data_byte1;
              //turn off specific note
              midi_note_off(mi_key);
          }else if(mi_code == controller_change
              && (mi_data_byte1 >=120 && mi_data_byte1 <= 127)
              ){
              //turn off channel notes
              reset_notes();
          }

          //reset data bytes;
          mi_data_byte1 = 0;
          mi_data_byte2 = 0;
      }
  }else if(encoder_changed){
    u8g.firstPage();  
    do {
      draw();
    } while( u8g.nextPage() );

    encoder_changed = false;
  }else{

    midi_mode = (!bitRead(PINC, 3) << 1) | !bitRead(PINC, 5);
    if(midi_mode != prev_midi_mode){
        reset_notes();
        encoder_changed = true;
    }
    prev_midi_mode = midi_mode;
    
    for(int i=0; i<3; i++){
      if(gatesstate[i]){
        if(!peaked[i] && envelopes[i]<peak[i]){
          if(current_millis - last_event[i] >= attack_rate){
            last_event[i] = current_millis;
            envelopes[i]+=attack_increment;
            if(envelopes[i]>=peak[i]){
              envelopes[i] = peak[i];
              peaked[i] = true;
            }
            setOutputChipEnv(envelopes[i], i);
          }
        }else if(peaked[i] && envelopes[i]>sustain[i]){
          if(current_millis - last_event[i] >= decay_rate){
            last_event[i] = current_millis;
            envelopes[i]-=decay_increment;
            if(envelopes[i]<sustain[i]){
              envelopes[i] = sustain[i];
            }
            setOutputChipEnv(envelopes[i], i);
          }
        }
      }else if(envelopes[i]>0){
        if(current_millis - last_event[i] >= release_rate){
          last_event[i] = current_millis;
          envelopes[i]-=release_increment;
          if(envelopes[i] < 0){
            envelopes[i] = 0;
          }
        }

        setOutputChipEnv(envelopes[i], i);
      }
    }
  }
   
}

void draw(){

  if(testing_mode){
    itoa(testing_voltage, buf, 10);
    u8g.drawStr(64,27,buf);
    return;
  }
  
  if(menu_position < 2){
    u8g.drawStr(40,(menu_position*20)+10,"<");
  }else{
    u8g.drawStr(50,((menu_position-2)*15)+10,">");
  }

  if(midi_mode==1){
    u8g.drawStr(0,50,"Stack");
  }else if(midi_mode==0){
    u8g.drawStr(0,50,"Rotate");
  }else if(midi_mode==2){
    u8g.drawStr(0,50,"Unison");
  }
  
  u8g.drawStr(0,10,"Ch:");
  itoa(midi_channel+1, buf, 10);
  u8g.drawStr(30,10,buf);
  u8g.drawStr(0,30,"# :");
  itoa(num_outputs+1, buf, 10);
  u8g.drawStr(30,30,buf);

  u8g.drawStr(60,10,"A:");
  itoa(attack, buf, 10);
  u8g.drawStr(80,10,buf);
  u8g.drawStr(60,25,"D:");
  itoa(decay, buf, 10);
  u8g.drawStr(80,25,buf);
  u8g.drawStr(60,40,"S:");
  itoa(min_sustain, buf, 10);
  u8g.drawStr(80,40,buf);
  u8g.drawStr(60,55,"R:");
  itoa(release, buf, 10);
  u8g.drawStr(80,55,buf);
  
}

void setOutputChip(unsigned int val, byte d){
  byte cs_pin = cs_pins[d];
  byte dac_channel = dacchns[d];
  byte lowByte = val & 0xff;
  byte highByte = ((val >> 8) & 0xff) | (dac_channel << 7) | B00110000; // Gain = 1x because using Aref. Shutdown = off

  //digitalWrite(cs_pin, LOW);
  bitClear(PORTB, cs_pin - 8);
  SPI.transfer(highByte);
  SPI.transfer(lowByte);
  bitSet(PORTB, cs_pin - 8);
}

void setOutputChipEnv(unsigned int val, byte d){
  byte cs_pin = env_cs_pins[d];
  byte dac_channel = env_dacchns[d];
  byte lowByte = val & 0xff;
  byte highByte = ((val >> 8) & 0xff) | (dac_channel << 7) | B00110000; // Gain = 1x because using Aref. Shutdown = off

  //digitalWrite(cs_pin, LOW);
  bitClear(PORTB, cs_pin - 8);
  SPI.transfer(highByte);
  SPI.transfer(lowByte);
  bitSet(PORTB, cs_pin - 8);
}

void midi_note_on(byte note, byte velocity){

    if(midi_mode == 1){ // Stack
    
        for(int i=0; i<=num_outputs; i++){
            if(notes[i] == 255){
                notes[i] = note;
                gatesstate[i] = true;
                peaked[i] = false;
                peak[i] = velocity * 33;
                if(peak[i] > 4095){
                  peak[i] = 4095;
                }
                sustain[i] = min_sustain_level;
    
                setOutputChip(pgm_read_word_near(midi_voltages + note), i);
    
                bitClear(PORTC, i);
                
                break;
            }
        }

    }else if(midi_mode == 0){ //Rotate

        notes[rotational_note] = note;
        gatesstate[rotational_note] = true;
        peaked[rotational_note] = false;
        peak[rotational_note] = velocity * 33;
        if(peak[rotational_note] > 4095){
          peak[rotational_note] = 4095;
        }
        sustain[rotational_note] = min_sustain_level;

        setOutputChip(pgm_read_word_near(midi_voltages + note), rotational_note);

        bitClear(PORTC, rotational_note);

        rotational_note++;
        if(rotational_note > num_outputs){
            rotational_note = 0; 
        }
      
    }else if(midi_mode == 2){ //Unison
      
       for(int i=0; i<=num_outputs; i++){
            
          notes[i] = note;
          gatesstate[i] = true;
          peaked[i] = false;
          peak[i] = velocity * 33;
          if(peak[i] > 4095){
            peak[i] = 4095;
          }
          sustain[i] = min_sustain_level;

          setOutputChip(pgm_read_word_near(midi_voltages + note), i);

          bitClear(PORTC, i);
          
       }
        
    }
}

void midi_note_off(byte note){

    if(midi_mode == 1 || midi_mode == 0){ // Stack or Rotate
  
        for(int i=0; i<=num_outputs; i++){
            if(notes[i] == note){
                notes[i] = 255;
                gatesstate[i] = false;
                peaked[i] = false;
    
                bitSet(PORTC, i);
                
                break;
            }
        }
      
    }else if(midi_mode == 2){ // Unison

        for(int i=0; i<=num_outputs; i++){
          
            notes[i] = 255;
            gatesstate[i] = false;
            peaked[i] = false;

            bitSet(PORTC, i);
                
        }
        
    }
}

void reset_notes(){
    for(int i=0; i<3; i++){
        notes[i] = 255;
        gatesstate[i] = false;
        peaked[i] = false;

        bitSet(PORTC, i);
    }
}

