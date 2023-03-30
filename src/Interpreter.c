#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include "common.h"
#include "Vec.h"
#include "Ops.h"
#include "Interpreter.h"

Tape tapeNew(uint32_t size) {
    Tape t;
    t.size = size;
    t.data = calloc(size, sizeof(*t.data));
    assert(t.data);
    t.ptr = t.data;
    return t;
}

void tapeMovePtr(Tape *t, int32_t i) {
    char *new_ptr = t->ptr + i;
    assert(new_ptr < t->data + t->size && new_ptr >= t->data);
    t->ptr = new_ptr;
}

void tapeFree(Tape *t) {
    free(t->data);
    t->size = 0;
    t->data = t->ptr = NULL;
}

void interpreterExecute(Vec(Op) program, Tape *tape) {
    VEC_ITERATE(op, program) {
        //for(int i = 0; i < 5; ++i) {
        //    printf("[%u]", tape->data[i]);
        //}
        //putchar('\n');
        switch(op->type) {
            case OP_INCREMENT:
                ++*tape->ptr;
                break;
            case OP_INCREMENT_X:
                *tape->ptr += op->as.x;
                break;
            case OP_DECREMENT:
                --*tape->ptr;
                break;
            case OP_DECREMENT_X:
                *tape->ptr -= op->as.x;
                break;
            case OP_FORWARD:
                tapeMovePtr(tape, 1);
                break;
            case OP_FORWARD_X:
                tapeMovePtr(tape, op->as.x);
                break;
            case OP_BACKWARD:
                tapeMovePtr(tape, -1);
                break;
            case OP_BACKWARD_X:
                tapeMovePtr(tape, -op->as.x);
                break;
            case OP_READ:
                *tape->ptr = getchar();
                break;
            case OP_WRITE:
                putchar(*tape->ptr);
                break;
            case OP_LOOP:
                while(*tape->ptr) {
                    interpreterExecute(op->as.loop_body, tape);
                }
                break;
            default:
                fprintf(stderr, "Error: unkown op:\n");
                opPrint(stderr, *op);
                UNREACHABLE();
        }
    }
}
