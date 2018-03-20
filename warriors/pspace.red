This text before the ;redcode line is ignored.
;redcode-94
;name P-space demo
;author Stefan
for 0
    This warrior demonstrates the new P-space features
    of pMARS. It is by no means a competitive warrior.
    By the way, this is a for/rof block comment.
rof
;assert VERSION >= 80 && ROUNDS > 1    ;must play at least two rounds

_RESULT     equ #0 ;P-space cell 0 is initialized with the last round result
_COUNTER    equ #1 ;user-defined
_STRATEGY   equ #2 ;user-defined

for ROUNDS > 10         ;a lot of history to work with, otherwise don't bother
    ldp _COUNTER,#0     ;increment a round counter
    add #1,-1
    stp.b -2,_COUNTER

str ldp _STRATEGY,#0    ;load strategy

res ldp _RESULT,#0      ;load last result into B-field
    sne #-1,res         ;is this the first round, i.e. no result?
    jmp naive           ;don't know what's going on yet
    jmz loss,res        ;a zero indicates a loss in the last round
  for WARRIORS <= 2     ;standard one-on-one
    djn tie,res
    jmp win
  rof WARRIORS <= 2
  for WARRIORS > 2      ;multi-warrior
    slt #10,res         ;won with ten other warriors or less?
    jmp win
    jmp tie
  rof WARRIORS > 2
rof rounds > 10

naive   jmp 0           ;here goes some default code
win     jmp 0           ;use _STRATEGY in str to find out what we
                        ;did last and keep doing it
loss    jmp 0           ;look at _STRATEGY and do something else, update
                        ;_STRATEGY
tie     jmp 0           ;who knows?

The     End
