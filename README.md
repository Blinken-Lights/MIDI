# MIDI

Notes on CC messages: in the file Nano_MIDI2CV.ino the line 'if(mi_code == mi_note_on){' detects 'note on' messages. CC messages could be read at this point too,
by testing the condition 'if(mi_code == controller_change){'. The 'controller number' will be in mi_data_byte1 and the 'controller value' will be in mi_data_byte2.

The file is designed to work with an MCP4922 DAC. It converts note numbers into values to output to the DAC, using the lookup table midi_voltages.
The MCP4922 output voltage is exactly 1/1000 of the 12-bit value you send to it [within some error tolerance, when using a 4.096 voltage reference],
which makes the maths simple.
