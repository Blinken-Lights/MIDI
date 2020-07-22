
/*
 * For Midi to Gate module
 */

const byte rotary_pin8 = 2;
const byte rotary_pin4 = 3;
const byte rotary_pin2 = 4;
const byte rotary_pin1 = 5;

const byte single_channel_pin = 6;

byte chosen_channel = 0;
byte single_channel = 0;
byte prev_single_channel = 0;
byte prev_chosen_channel = 0;

byte first_note = 35;

const byte strobe_pin = 8;
const byte data_pin = 9;
const byte clock_pin = 10;

const byte reset_pin = 7;

const byte midi_clk = 11;
//unsigned long midi_clk_high;
//bool clock_is_high;
int clock_pulses;

byte code;

const byte note_off                 = 0;
const byte note_on                  = 1;
const byte poly_pressure            = 2;
const byte controller_change        = 3;
const byte program_change           = 4;
const byte channel_pressure_change  = 5;
const byte pitch_bend_change        = 6;

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

byte input_byte;
byte status_byte;
byte data_byte1;
byte data_byte2;
byte channel;
byte key[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
bool channel_status[16];
byte velocity[16];

byte i;

int gate_states = 0;

void setup() {
    Serial.begin(31250);
    
    pinMode(strobe_pin, OUTPUT);
    pinMode(data_pin, OUTPUT);
    pinMode(clock_pin, OUTPUT);
    
    pinMode(midi_clk, OUTPUT);

    pinMode(rotary_pin8, INPUT_PULLUP);
    pinMode(rotary_pin4, INPUT_PULLUP);
    pinMode(rotary_pin2, INPUT_PULLUP);
    pinMode(rotary_pin1, INPUT_PULLUP);

    pinMode(single_channel_pin, INPUT_PULLUP);

    pinMode(reset_pin, INPUT_PULLUP);

    gate_states = 0;
    update_gates();
}

void loop() {

    chosen_channel = (!digitalRead(rotary_pin8) << 3) | (!digitalRead(rotary_pin4) << 2) | (!digitalRead(rotary_pin2) << 1) | !digitalRead(rotary_pin1);
    single_channel = !digitalRead(single_channel_pin);

    if((prev_single_channel != single_channel)
        || chosen_channel != prev_chosen_channel){
        gate_states = 0;
        update_gates();
    }
    /*if(clock_is_high){
        if(millis() - midi_clk_high > 10){
            digitalWrite(midi_clk, LOW);
            clock_is_high = false;
        }
    }*/
    
    if (Serial.available() > 0) {
      
        input_byte = Serial.read();

        if(bitRead(input_byte, 7)){
            status_byte = input_byte;
            code = (status_byte & B01110000) >> 4;
            channel = status_byte & B00001111;
            data_byte1 = 0;
            data_byte2 = 0;

            //reset all
            if(status_byte == reset_all){
                gate_states = 0;
                update_gates();
            }else if(status_byte == clock_code){
                //digitalWrite(midi_clk, HIGH);
                //midi_clk_high = millis();
                //clock_is_high = true;
                clock_pulses++;
                if(clock_pulses>=24){
                    clock_pulses = 0;
                }else if(clock_pulses == 1
                    || clock_pulses == 7
                    || clock_pulses == 13
                    || clock_pulses == 19){
                    digitalWrite(midi_clk, HIGH);
                }else if(clock_pulses == 4
                    || clock_pulses == 10
                    || clock_pulses == 16
                    || clock_pulses == 22){
                    digitalWrite(midi_clk, LOW);
                }
            }else if(status_byte == stop_clock){
                digitalWrite(midi_clk, LOW);
            }
            
        }else if(data_byte1==0){
            data_byte1 = input_byte;

            //if status byte matches single data byte messages, reset here
            if(code == program_change
                || code == channel_pressure_change){
                //reset data bytes;
                data_byte1 = 0;
                data_byte2 = 0;
            }
            
        }else{
            data_byte2 = input_byte;

            if(code == note_on){
                key[channel] = data_byte1;
                velocity[channel] = data_byte2;
                if(velocity[channel] > 0){
                    channel_status[channel] = true;
                }else{
                    channel_status[channel] = false;
                }
                if(single_channel){
                    if(channel == chosen_channel
                        && key[channel] >= first_note
                        && key[channel] < (first_note+16)){
                        bitWrite(gate_states, (data_byte1 - first_note), channel_status[channel]);
                    }
                }else{
                    bitWrite(gate_states, channel, channel_status[channel]);
                }
            }else if(code == note_off){
                key[channel] = data_byte1;
                velocity[channel] = data_byte2;
                channel_status[channel] = false;
                if(single_channel){
                    if(channel == chosen_channel
                        && key[channel] >= first_note
                        && key[channel] < (first_note+16)){
                        bitClear(gate_states, (data_byte1 - first_note));
                    }
                }else{
                    bitWrite(gate_states, channel, channel_status[channel]);
                }
            }else if(code == controller_change
                && (data_byte1 == sound_off
                    || data_byte1 == reset_controllers
                    || data_byte1 == all_notes_off
                    || data_byte1 == omni_on
                    || data_byte1 == omni_off
                    || data_byte1 == mono_on
                    || data_byte1 == poly_on)
                ){
                // turn off all notes on the channel
                if(single_channel){
                    if(channel == chosen_channel){
                        gate_states = 0;
                    }
                }else{
                    bitWrite(gate_states, channel, 0);
                }
            }

            if(code == note_on
                || code == note_off
                || (code == controller_change
                    && (data_byte1 == sound_off
                        || data_byte1 == reset_controllers
                        || data_byte1 == all_notes_off
                        || data_byte1 == omni_on
                        || data_byte1 == omni_off
                        || data_byte1 == mono_on
                        || data_byte1 == poly_on))
                 ){
                update_gates();
            }
            
            //reset data bytes;
            data_byte1 = 0;
            data_byte2 = 0;
        }
    }else{

        if(!digitalRead(reset_pin)){
            gate_states = 0;
            update_gates();
        }
        
    }

    prev_single_channel = single_channel;
    prev_chosen_channel = chosen_channel;
}

void update_gates(){

    //write to shift registers
    for(i=0; i<=15; i++){
        // clock_pin LOW
        PORTB &= B11111011;

        // write bit of data to data_pin
        bitWrite(PORTB, 1, bitRead(gate_states, i));
        // clock_pin HIGH
        PORTB |= B00000100;
    }
    // strobe_pin HIGH
    PORTB |= B00000001;
    // strobe_pin LOW
    PORTB &= B11111110;
}

