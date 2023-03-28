#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
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

void opFree(Op *op) {
    if(op->type == OP_LOOP) {
        VEC_ITERATE(op2, op->as.loop_body) {
            opFree(op2);
        }
        VEC_FREE(op->as.loop_body);
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

void execute(Vec(Op) program, Tape *tape) {
    VEC_ITERATE(op, program) {
        //for(int i = 0; i < 5; ++i) {
        //    printf("[%u]", tape->data[i]);
        //}
        //putchar('\n');
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

void compile_to_c(FILE *out, Vec(Op) prog) {
    VEC_ITERATE(op, prog) {
        switch(op->type) {
            case OP_INCREMENT:
                fputs("++*ptr;\n", out);
                break;
            case OP_DECREMENT:
                fputs("--*ptr;\n", out);
                break;
            case OP_FORWARD:
                fputs("++ptr;\n", out);
                break;
            case OP_BACKWARD:
                fputs("--ptr;\n", out);
                break;
            case OP_READ:
                fputs("*ptr = getchar();\n", out);
                break;
            case OP_WRITE:
                fputs("putchar(*ptr);\n", out);
                break;
            case OP_LOOP:
                fputs("while(*ptr) {\n", out);
                compile_to_c(out, op->as.loop_body);
                fputs("}\n", out);
                break;
            default:
                fprintf(stderr, "Error: unkown op:\n");
                opPrint(stderr, *op); putchar('\n');
                assert(0);
                return;
        }
    }
}

static bool read_file(String *buffer, const char *path) {
    FILE *f = fopen(path, "r");
    if(!f) {
        return false;
    }

    char read_buffer[1024] = {0};
    while(fgets(read_buffer, sizeof(read_buffer)/sizeof(read_buffer[0]), f) != NULL) {
        stringAppend(buffer, "%s", read_buffer);
    }

    if(fclose(f) == EOF) {
        return false;
    }
    return true;
}

static inline void usage(const char *argv0) {
    fprintf(stderr, "Usage: %s [options]\n", argv0);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    [code]    execute code directly from the first argument.\n");
    fprintf(stderr, "    -f [file] execute a file.\n");
    fprintf(stderr, "    -c [file] compile a file to optimized C code.\n");
}

#define TAPE_SIZE 30000
int main(int argc, char **argv) {
    if(argc < 2) {
        fputs("Error: insufficient arguments.\n", stderr);
        usage(argv[0]);
        return 1;
    }
    int opt;
    char *opt_input_file = NULL;
    bool opt_compile_to_c = false;
    while((opt = getopt(argc, argv, "f:c:h")) != -1) {
        switch(opt) {
            case 'h':
                usage(argv[0]);
                return 1;
            case 'f':
                opt_input_file = optarg;
                break;
            case 'c':
                opt_input_file = optarg;
                opt_compile_to_c = true;
                break;
            default:
                continue;
        }
    }
    String input;
    if(opt_input_file) {
        input = stringNew(16); // 16 is just a random number.
        if(!read_file(&input, argv[2])) {
            fprintf(stderr, "Error: failed to read file '%s'!\n", argv[2]);
        }
    } else {
        input = stringCopy(argv[1]);
    }
    Compiler compiler = compilerNew(input);
    stringFree(input);
    Vec(Op) program = compile(&compiler, false);
    compilerFree(&compiler);
    if(!program) {
        return 1;
    }
    //VEC_ITERATE(op, program) {
    //    opPrint(stdout, *op); putchar('\n');
    //}

    if(opt_compile_to_c) {
        FILE *out = fopen("brainf.out.c", "w");
        assert(out);
        fputs("#include <stdio.h>\n", out);
        fputs("static char tape[30000] = {0};\n", out);
        fputs("static char *ptr = tape;\n", out);
        fputs("int main(void) {\n", out);
        compile_to_c(out, program);
        fputs("return 0;\n}\n", out);
        assert(fclose(out) == 0);
    } else {
        Tape tape = tapeNew(TAPE_SIZE);
        execute(program, &tape);
        tapeFree(&tape);
    }
    VEC_ITERATE(op, program) { opFree(op); }
    VEC_FREE(program);
    return 0;
}
