#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>
#include <stdint.h>
#include "Strings.h"
#include "Vec.h"
#include "Ops.h"

typedef struct compiler {
    String input;
    uint32_t loc;
} Compiler;

Compiler compilerNew(char *input);
void compilerFree(Compiler *c);
Vec(Op) compile(Compiler *c);

#endif // COMPILER_H
