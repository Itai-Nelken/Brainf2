#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "Vec.h"
#include "Strings.h"

typedef enum op_type {
    OP_INCREMENT,
    OP_DECREMENT,
    OP_FORWARD,
    OP_BACKWARD,
    OP_READ,
    OP_WRITE,
    OP_LOOP
} OpType;

typedef struct op {
    OpType type;
    union {
        Vec(struct op) loop_body;
    } as;
} Op;

Op opNew(OpType type) {
    return (Op){
        .type = type
    };
}

void opFree(Op op) {
    if(op.type == OP_LOOP) {
        VEC_ITERATE(op2, op.as.loop_body) {
            opFree(*op2);
        }
        VEC_FREE(op.as.loop_body);
    }
}

static const char *op_type_str(OpType op) {
    static const char *op_names[] = {
        "OP_INCREMENT",
        "OP_DECREMENT",
        "OP_FORWARD", 
        "OP_BACKWARD",
        "OP_READ",
        "OP_WRITE",
        "OP_LOOP"
    };
    return op_names[op];
}

static void op_print_internal(FILE *to, Op op, uint8_t depth) {
    // depth * 2 so for each depth level, 2 spaces are printed.
    for(uint8_t i = 0; i < depth * 2; ++i) fputc(' ', to);
    fprintf(to, "%s", op_type_str(op.type));
    if(op.type == OP_LOOP) {
        VEC_ITERATE(op2, op.as.loop_body) {
            fputc('\n', to);
            op_print_internal(to, *op2, depth + 1);
        }
    }
}

void opPrint(FILE *to, Op op) {
    op_print_internal(to, op, 0);
}

typedef struct compiler {
    String input;
    uint32_t loc;
} Compiler;

Compiler compilerNew(char *input) {
    return (Compiler){
        .input = stringCopy(input),
        .loc = 0
    };
}

bool compilerIsEnd(Compiler *c) {
    return c->loc >= stringLength(c->input);
}

char compilerNext(Compiler *c) {
    assert(!compilerIsEnd(c));
    return c->input[c->loc++];
}

void compilerFree(Compiler *c) {
    stringFree(c->input);
    c->input = NULL;
    c->loc = 0;
}

Vec(Op) compile(Compiler *c, bool in_loop) {
    Vec(Op) out = VEC_NEW(Op);
    while(!compilerIsEnd(c)) {
        switch(compilerNext(c)) {
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
                loop.as.loop_body = compile(c, true);
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

typedef struct tape {
    uint32_t size;
    char *data;
    char *ptr;
} Tape;

Tape tapeNew(uint32_t size) {
    Tape t;
    t.size = size;
    t.data = calloc(1, sizeof(*t.data));
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

void execute(Vec(Op) program, Tape *tape) {
    for(uint32_t i = 0; i < VEC_LENGTH(program); ++i) {
        //for(int i = 0; i < 5; ++i) {
        //    printf("[%u]", tape->data[i]);
        //}
        //putchar('\n');
        Op *op = &program[i];
        switch(op->type) {
            case OP_INCREMENT:
                ++*tape->ptr;
                break;
            case OP_DECREMENT:
                --*tape->ptr;
                break;
            case OP_FORWARD:
                tapeMovePtr(tape, 1);
                break;
            case OP_BACKWARD:
                tapeMovePtr(tape, -1);
                break;
            case OP_READ:
                *tape->ptr = getchar();
                break;
            case OP_WRITE:
                putchar(*tape->ptr);
                break;
            case OP_LOOP:
                while(*tape->ptr) {
                    execute(op->as.loop_body, tape);
                }
                break;
            default:
                fprintf(stderr, "Error: unkown op:\n");
                opPrint(stderr, *op);
                assert(0);
                return;
        }
    }
}

#define TAPE_SIZE 30000
int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "Usage: %s [code]\n", argv[0]);
        return 1;
    }
    Compiler compiler = compilerNew(argv[1]);
    Vec(Op) program = compile(&compiler, false);
    compilerFree(&compiler);
    if(!program) {
        return 1;
    }
    //VEC_ITERATE(op, program) {
    //    opPrint(stdout, *op); putchar('\n');
    //}
    Tape tape = tapeNew(TAPE_SIZE);
    execute(program, &tape);
    tapeFree(&tape);
    VEC_ITERATE(op, program) { opFree(*op); }
    VEC_FREE(program);
    return 0;
}
