/* 

Terse midi parser 

schmitzbits.de

How to parse MIDI 

Example code to be run in a console environment, e.g. gcc + bash


Copyright (c) 2002, 2020, schmitzbits.de
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include "midi.h"

uint8_t runstat;
uint8_t our_channel = 0x01;

uint8_t midi_get_byte(){
	int val = 0;
	char str[80];
	printf(">");
	fflush(stdout);
	if (fgets(str, 80, stdin)!=NULL){
		sscanf(str, "%02x", &val);
//		printf("%s", str);
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
  printf("note_off ch:%01x, note: %02x, vel: %02x\n", ch, db1, db2);
}

void midi_note_on(uint8_t ch, uint8_t db1, uint8_t db2){
  if (db2 == 0x00) {
	midi_note_off(ch, db1, db2);
	return;
  }
   printf("note_on ch:%01x, note: %02x, vel: %02x\n", ch, db1, db2);
}
void midi_control(uint8_t ch,uint8_t db1, uint8_t db2){
  printf("midi_control ch:%01x, cc: %02x, value: %02x\n", ch,  db1, db2);
}

void midi_aftertouch(uint8_t ch,uint8_t db1, uint8_t db2){
  printf("midi_aftch ch: %01x,%02x, %02x\n", ch, db1, db2);
}
void midi_pitchwheel(uint8_t ch,uint8_t db1, uint8_t db2){
  printf("midi_pitch ch: %01x,%02x, %02x\n", ch, db1, db2);
}
void midi_patch(uint8_t ch, uint8_t db1){
  printf("midi_patch ch: %01x,%02x\n", ch, db1);
}
void midi_pressure(uint8_t ch, uint8_t db1){
  printf("midi_pressure ch:%01x,%02x\n", ch, db1);
}

void midi_reset(void){
  printf("midi_reset new running status: %02x\n", runstat);
}

void sys_realtime(uint8_t ch){
	//printf("sys_realtime(%02x)\n",ch );
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
	//printf("sys_common(%02x)\n",ch );
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
		// if runstat = 0 then we are possibly waiting for the databyte of a system common message
		// we silently discard them
		
		// if db_expect is 3
		// this gracefully handles SYSEX. 
		// one way to get us out of this state: new running status
		return; 
	}
	
	// split runstat into message type and channel for convenience
	uint8_t type = runstat & 0xf0;
	uint8_t rec_channel = runstat & 0x0f;
	
    //adjust if you want to receive only on specific channel
	//    if (rec_channel != our_channel) {
	//        printf("Message is not for channel 1\n");
	//        return;
	//    }

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
		default:
			printf("Running Status: %02x, db1: %02x, db2: %02x", runstat, db1, db2);
		
	}
	db_expect = voice_message(runstat);  // prepare db_expect for the next round
}


int main(int argc, char** argv){
	uint8_t x;
	printf("Enter Midi Message in Hexadecimal:\r\n");
	while(1){
		printf("(%02x)", runstat);
		x = midi_get_byte();
		midi_parse(x);
	}
	exit(0);
}
