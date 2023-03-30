#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdint.h>
#include "Vec.h"
#include "Ops.h"

typedef struct tape {
    uint32_t size;
    char *data;
    char *ptr;
} Tape;

Tape tapeNew(uint32_t size);
void tapeMovePtr(Tape *t, int32_t i);
void tapeFree(Tape *t);

void interpreterExecute(Vec(Op) program, Tape *tape);

#endif // INTERPRETER_H
