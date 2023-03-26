#include <stdio.h>
#include <stddef.h> // size_t
#include <string.h>
#include "Vec.h"

typedef enum op {
    OP_INCREMENT,
    OP_DECREMENT,
    OP_FORWARD,
    OP_BACKWARD,
    OP_READ,
    OP_WRITE,
    OP_START_LOOP,
    OP_END_LOOP
} Op;

static const char *op_to_str(Op op) {
    static const char *op_names[] = {
        "OP_INCREMENT",
        "OP_DECREMENT",
        "OP_FORWARD",
        "OP_BACKWARD",
        "OP_READ",
        "OP_WRITE",
        "OP_START_LOOP",
        "OP_END_LOOP"
    };
    return op_names[op];
}

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "USAGE: %s [code]\n", argv[0]);
        return 1;
    }
    Op *program = VEC_NEW(Op);
    size_t input_length = strlen(argv[1]);
    for(size_t i = 0; i < input_length; ++i) {
        char c = argv[1][i];
        switch(c) {
            case '+': VEC_PUSH(program, OP_INCREMENT); break;
            case '-': VEC_PUSH(program, OP_DECREMENT); break;
            case '>': VEC_PUSH(program, OP_FORWARD); break;
            case '<': VEC_PUSH(program, OP_BACKWARD); break;
            case ',': VEC_PUSH(program, OP_READ); break;
            case '.': VEC_PUSH(program, OP_WRITE); break;
            case '[': VEC_PUSH(program, OP_START_LOOP); break;
            case ']': VEC_PUSH(program, OP_END_LOOP); break;
            default:
                break;
        }
    }

    VEC_ITERATE(op, program) {
        printf("%s\n", op_to_str(*op));
    }
    VEC_FREE(program);
    return 0;
}
