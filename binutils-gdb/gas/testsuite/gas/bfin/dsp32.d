#objdump: -dr
#name: dsp32
.*: +file format .*
Disassembly of section .text:

00000000 <.text>:
   0:	29 c2 1c 20 	(R1:0) = ((A1:0) = R3 * R4) (T);
   4:	49 c2 08 21 	(R5:4) = ((A1:0) = R1 * R0) (IS);
   8:	69 c2 08 21 	(R5:4) = ((A1:0) = R1 * R0) (IS,NS);
   c:	a9 c2 ae 21 	(R7:6) = ((A1:0) = R5 * R6) (TFU);
  10:	c1 c2 2e 21 	R4 = ((A1:0) = R5 * R6) (IU);
  14:	e1 c2 2e 21 	R4 = ((A1:0) = R5 * R6) (IU,NS);
  18:	19 c2 a5 20 	(R3:2) = R4 * R5;
  1c:	19 c3 25 21 	(R5:4) = R4 * R5 (M);
  20:	31 c2 9e 20 	R2 = R3 * R6 (T);
  24:	51 c2 9e 20 	R2 = R3 * R6 (IS);
  28:	71 c2 9e 20 	R2 = R3 * R6 (IS,NS);
  2c:	91 c2 9e 20 	R2 = R3 * R6 (FU);
  30:	b1 c2 9e 20 	R2 = R3 * R6 (TFU);
  34:	d1 c2 9e 20 	R2 = R3 * R6 (IU);
  38:	f1 c2 9e 20 	R2 = R3 * R6 (IU,NS);
  3c:	11 c3 9e 20 	R2 = R3 * R6 (M);
  40:	31 c3 9e 20 	R2 = R3 * R6 (M,T);
  44:	51 c3 9e 20 	R2 = R3 * R6 (M,IS);
  48:	71 c3 9e 20 	R2 = R3 * R6 (M,IS,NS);