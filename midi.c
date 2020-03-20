/* 

Terse midi parser 

schmitzbits.de

How to parse MIDI 

Example code to be run in a console environment, e.g. gcc + bash


MIT License

Copyright (c) 2002, 2020 René Schmitz 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <stdio.h>
#include <inttypes.h>

#include "midi.h"

uint8_t runstat;
uint8_t our_channel = 0x01;

uint8_t midi_get_byte(){
	int val = 0;
	char str [80];
	printf(">");
	if (fgets(str, 80, stdin)!=NULL){
		sscanf(str, "%02x", &val);
		//printf("%d", val);
	}
	return val;
}

void midi_clock(void){
  printf("clock\n");
}
void midi_start(void){
  printf("start\n");
}
void midi_stop(void){
  printf("stop\n");
}
void midi_cont(){
  printf("cont\n");
}

void midi_note_off(uint8_t ch,uint8_t db1, uint8_t db2){
  printf("note_off %01x,%02x,%02x\n", ch, db1, db2);
}

void midi_note_on(uint8_t ch, uint8_t db1, uint8_t db2){
  if (db2 == 0x00) {
	midi_note_off(ch, db1, db2);
	return;
  }
   printf("note_on %01x, %02x,%02x\n", ch, db1, db2);
}
void midi_control(uint8_t ch,uint8_t db1, uint8_t db2){
  printf("midi_control %01x,%02x, %02x\n", ch,  db1, db2);
}

void midi_aftertouch(uint8_t ch,uint8_t db1, uint8_t db2){
  printf("midi_aftch %01x,%02x, %02x\n", ch, db1, db2);
}
void midi_pitchwheel(uint8_t ch,uint8_t db1, uint8_t db2){
  printf("midi_pitch %01x,%02x, %02x\n", ch, db1, db2);
}
void midi_patch(uint8_t ch, uint8_t db1){
  printf("midi_patch %01x,%02x\n", ch, db1);
}
void midi_pressure(uint8_t ch, uint8_t db1){
  printf("midi_pressure %01x,%02x\n", ch, db1);
}

void midi_reset(void){
  printf("midi_reset %02x\n", runstat);
}

void sys_realtime(uint8_t ch){
	printf("sys_realtime(%02x)\n",ch );
	switch (ch){
		case MIDI_CLOCK:
			midi_clock();
			return;
		case MIDI_START:
			midi_start();
			return;
		case MIDI_STOP:
			midi_stop();
			return;
		case MIDI_CONT:
			midi_cont();
			return;
		case MIDI_RESET:
			runstat = 0;
			midi_reset();
			break;
		default:
			return;
	}
}

// returns how many message bytes the system common messages expect

uint8_t sys_common(uint8_t ch){
	printf("sys_common(%02x)\n",ch );
	switch (ch){
		case MIDI_SONG_SEL:
		case MIDI_MTC:
			return 1;
		case MIDI_SPP:
			return 2;
		case MIDI_SYSEX:
			return 3;
		default:
			return 0;
	}
}

// returns how many message bytes the voice messages expect

uint8_t voice_message(uint8_t ch){
	uint8_t type = ch & 0xf0;
	switch (type) {
		case MIDI_NOTE_ON:
		case MIDI_NOTE_OFF:
		case MIDI_CONTROL:
		case MIDI_AFTCH:
		case MIDI_PITCH:
			return 2;
		case MIDI_PATCH:
		case MIDI_PRESSURE:
			return 1;
		default:
			return 0;
	}
}

// processes one byte and updates the internal state: running status, db1 and db2

void midi_parse(uint8_t x){
    static uint8_t db_expect = 0;	//holds how many databytes the current message has
    static uint8_t db1;
    static uint8_t db2;

	if (x >= 0xf8)	{
		sys_realtime(x);
		return;
	}
		
	if (x >= 0xf0) {
		runstat = 0;				// system common cancels running status
		db_expect = sys_common(x);  // how many databytes to expect next?
		return;
	}
	
	if (x >= 0x80) {
		runstat = x;   // update running status
		db_expect = voice_message(runstat);  // how many databytes do follow?
		return;
	}

	if (runstat != 0 && db_expect == 2) {
		db1 = x;		// store into databyte1
		db_expect = 1;  // one down, one to go
		return;
	}
	if (runstat != 0 && db_expect == 1) {
		db2 = x;		// store into databyte2
		db_expect = 0;  // thats all, we finished a message, we do not return but move on
	} else { 
		// if db_expect is 3
		// this gracefully handles SYSEX. 
		// discard the byte, (or do something with it here)
		// two things get us out of this state: New Running Status, OR Sysex End
		return; 
	}
	
	// split runstat into message type and channel for convenience
	uint8_t type = runstat & 0xf0;
	uint8_t rec_channel = runstat & 0x0f;
	
	//adjust if you want to receive on more than one channel
	if (rec_channel != our_channel) return;
	
	// at this point our message is complete, and db1 and db2 hold the respective data bytes
	// if there is only one byte, since its the last, it is contained in db2
	
	switch (type) { // dispatch our messages
		case MIDI_NOTE_ON:
			midi_note_on(rec_channel,db1, db2);
			break;
		case MIDI_NOTE_OFF:
			midi_note_off(rec_channel, db1, db2);
			break;
		case MIDI_CONTROL:
			midi_control(rec_channel, db1, db2);
			break;
		case MIDI_AFTCH:
			midi_aftertouch(rec_channel, db1, db2);
			break;
		case MIDI_PITCH:
			midi_pitchwheel(rec_channel, db1, db2);
			break;
		case MIDI_PATCH:
			midi_patch(rec_channel, db2);  
			break;
		case MIDI_PRESSURE:
			midi_pressure(rec_channel, db2);
			break;
		
	}
	db_expect = voice_message(runstat);  // prepare db_expect for the next round
}

int main(int argc, char* argv){
	uint8_t x;
	while(1){
		printf("(%02x)", runstat);
		x = midi_get_byte();
		midi_parse(x);
	}
}
