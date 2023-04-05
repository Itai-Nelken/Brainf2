#include <stdio.h>
#include <stdbool.h>
#include <stddef.h> // NULL
#include "Strings.h"
#include "Vec.h"
#include "Ops.h"
#include "Compiler.h"

Compiler compilerNew(char *input) {
    return (Compiler){
        .input = stringCopy(input),
        .loc = 0
    };
}

void compilerFree(Compiler *c) {
    stringFree(c->input);
    c->input = NULL;
    c->loc = 0;
}

static bool is_end(Compiler *c) {
    return c->loc >= stringLength(c->input);
}

static char next(Compiler *c) {
    assert(!is_end(c));
    return c->input[c->loc++];
}


static Vec(Op) compile_internal(Compiler *c, bool in_loop) {
    Vec(Op) out = VEC_NEW(Op);
    while(!is_end(c)) {
        switch(next(c)) {
            case '+':
                VEC_PUSH(out, opNew(OP_INCREMENT));
                break;
            case '-':
                VEC_PUSH(out, opNew(OP_DECREMENT));
                break;
            case '>':
                VEC_PUSH(out, opNew(OP_FORWARD));
                break;
            case '<':
                VEC_PUSH(out, opNew(OP_BACKWARD));
                break;
            case ',':
                VEC_PUSH(out, opNew(OP_READ));
                break;
            case '.':
                VEC_PUSH(out, opNew(OP_WRITE));
                break;
            case '[': {
                Op loop = opNew(OP_LOOP);
                loop.as.loop_body = compile_internal(c, true);
                if(!loop.as.loop_body) {
                    return NULL;
                }
                VEC_PUSH(out, loop);
                break;
            }
            case ']':
                if(in_loop) {
                    return out;
                } else {
                    fputs("Error: unexpected ']'!\n", stderr);
                    return NULL;
                }
            default:
                break;
        }
    }
    return out;
}

Vec(Op) compile(Compiler *c) {
    return compile_internal(c, false);
}
