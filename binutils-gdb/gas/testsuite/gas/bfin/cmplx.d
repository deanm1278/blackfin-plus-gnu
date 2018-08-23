
a.out:     file format elf32-bfin


Disassembly of section .text:

00000000 <.text>:
   0:	a0 c1 b7 38 	R2 = CMUL(R6,R7);
   4:	a1 c1 b7 38 	R2 = CMUL(R6,R7*);
   8:	a2 c1 b7 38 	R2 = CMUL(R6*,R7*);
   c:	a8 c1 37 39 	(R5:4) = CMUL(R6,R7);
  10:	a9 c1 37 39 	(R5:4) = CMUL(R6,R7*);
  14:	aa c1 37 39 	(R5:4) = CMUL(R6*,R7*);
  18:	a0 c1 37 00 	(A1:0) = CMUL(R6,R7);
  1c:	a1 c1 37 00 	(A1:0) = CMUL(R6,R7*);
  20:	e2 c1 37 00 	(A1:0) = CMUL(R6*,R7*) (IS);
  24:	a0 c1 37 08 	(A1:0) += CMUL(R6,R7);
  28:	a1 c1 37 08 	(A1:0) += CMUL(R6,R7*);
  2c:	a2 c1 37 08 	(A1:0) += CMUL(R6*,R7*);
  30:	a0 c1 37 10 	(A1:0) -= CMUL(R6,R7);
  34:	a1 c1 37 10 	(A1:0) -= CMUL(R6,R7*);
  38:	a2 c1 37 10 	(A1:0) -= CMUL(R6*,R7*);
  3c:	a0 c1 b7 20 	R2 = ((A1:0) = CMUL(R6,R7));
  40:	c1 c1 b7 20 	R2 = ((A1:0) = CMUL(R6,R7*)) (T);
  44:	a2 c1 b7 20 	R2 = ((A1:0) = CMUL(R6*,R7*));
  48:	a0 c1 b7 28 	R2 = ((A1:0) += CMUL(R6,R7));
  4c:	e1 c1 b7 28 	R2 = ((A1:0) += CMUL(R6,R7*)) (IS);
  50:	a2 c1 b7 28 	R2 = ((A1:0) += CMUL(R6*,R7*));
  54:	a0 c1 b7 30 	R2 = ((A1:0) -= CMUL(R6,R7));
  58:	a1 c1 b7 30 	R2 = ((A1:0) -= CMUL(R6,R7*));
  5c:	a2 c1 b7 30 	R2 = ((A1:0) -= CMUL(R6*,R7*));
  60:	a8 c1 37 21 	(R5:4) = ((A1:0) = CMUL(R6,R7));
  64:	a9 c1 37 21 	(R5:4) = ((A1:0) = CMUL(R6,R7*));
  68:	aa c1 37 21 	(R5:4) = ((A1:0) = CMUL(R6*,R7*));
  6c:	a8 c1 37 29 	(R5:4) = ((A1:0) += CMUL(R6,R7));
  70:	a9 c1 37 29 	(R5:4) = ((A1:0) += CMUL(R6,R7*));
  74:	ea c1 37 29 	(R5:4) = ((A1:0) += CMUL(R6*,R7*)) (IS);
  78:	a8 c1 37 31 	(R5:4) = ((A1:0) -= CMUL(R6,R7));
  7c:	a9 c1 37 31 	(R5:4) = ((A1:0) -= CMUL(R6,R7*));
  80:	aa c1 37 31 	(R5:4) = ((A1:0) -= CMUL(R6*,R7*));
