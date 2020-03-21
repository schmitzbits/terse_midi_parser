# terse_midi_parser

An introduction into decomposing a MIDI datastream 

to run: compile with gcc and run in shell

```
$ gcc midi.c -o midi
$ ./midi
```
You can then try entering some bytes, line by line

Try 91, 10, 10
(note on channel 1, note=10, velocity=10)

```
$ ./midi.exe
Enter Midi Message in Hexadecimal:
(00)>91
(91)>10
(91)>10
note_on 1, 10,10
(91)>
```








