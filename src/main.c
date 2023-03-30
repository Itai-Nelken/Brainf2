#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include "Vec.h"
#include "Strings.h"

#define UNREACHABLE() (fprintf(stderr, "\nUnreachable state!\n"), abort())

typedef enum op_type {
    OP_INCREMENT, OP_INCREMENT_X,
    OP_DECREMENT, OP_DECREMENT_X,
    OP_FORWARD, OP_FORWARD_X,
    OP_BACKWARD, OP_BACKWARD_X,
    OP_READ,
    OP_WRITE,
    OP_LOOP
} OpType;

typedef struct op {
    OpType type;
    union {
        Vec(struct op) loop_body;
        uint32_t x;
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
        "OP_INCREMENT", "OP_INCREMENT_X",
        "OP_DECREMENT", "OP_DECREMENT_X",
        "OP_FORWARD", "OP_FORWARD_X",
        "OP_BACKWARD", "OP_BACKWARD_X",
        "OP_READ",
        "OP_WRITE",
        "OP_LOOP"
    };
    return op_names[op];
}

static bool is_x_op(OpType);
static void op_print_internal(FILE *to, Op op, uint8_t depth) {
    // depth * 2 so for each depth level, 2 spaces are printed.
    for(uint8_t i = 0; i < depth * 2; ++i) fputc(' ', to);
    fprintf(to, "%s", op_type_str(op.type));
    if(op.type == OP_LOOP) {
        VEC_ITERATE(op2, op.as.loop_body) {
            fputc('\n', to);
            op_print_internal(to, *op2, depth + 1);
        }
    } else if(is_x_op(op.type)) {
        fprintf(to, ", %u", op.as.x);
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

typedef struct op_iterator {
    Vec(Op) ops;
    uint32_t idx;
} OpIterator;

static OpIterator opIteratorNew(Vec(Op) ops) {
    return (OpIterator){
        .ops = ops,
        .idx = 0
    };
}

static void opIteratorFree(OpIterator *iter) {
    iter->ops = NULL;
    iter->idx = 0;
}

static uint32_t opIteratorCurrentIdx(OpIterator *iter) {
    return iter->idx - 1;
}

static Op *opIteratorNextOrNull(OpIterator *iter) {
    if(iter->idx < VEC_LENGTH(iter->ops)) {
        return &iter->ops[iter->idx++];
    }
    return NULL;
}

#define WINDOW_SIZE 2
typedef Op *Window[WINDOW_SIZE];

static void window_slide(Window window, OpIterator *iter) {
    static_assert(WINDOW_SIZE == 2, "window_slide() expects WINDOW_SIZE to be 2");
    window[0] = window[1];
    window[1] = opIteratorNextOrNull(iter);
}

static bool window_is_empty(Window window) {
    return window[0] == NULL && window[1] == NULL;
}

static bool is_x_op(OpType op_type) {
    switch(op_type) {
        case OP_INCREMENT_X:
        case OP_DECREMENT_X:
        case OP_FORWARD_X:
        case OP_BACKWARD_X:
            return true;
        default:
            break;
    }
    return false;
}

static OpType x_op_from_op(OpType op) {
    switch(op) {
        case OP_INCREMENT:
            return OP_INCREMENT_X;
        case OP_DECREMENT:
            return OP_DECREMENT_X;
        case OP_FORWARD:
            return OP_FORWARD_X;
        case OP_BACKWARD:
            return OP_BACKWARD_X;
        default:
            break;
    }
    UNREACHABLE();
}

static OpType remove_x_from_x_op(OpType x_op) {
    switch(x_op) {
        case OP_INCREMENT_X:
            return OP_INCREMENT;
        case OP_DECREMENT_X:
            return OP_DECREMENT;
        case OP_FORWARD_X:
            return OP_FORWARD;
        case OP_BACKWARD_X:
            return OP_BACKWARD;
        default:
            break;
    }
    UNREACHABLE();
}

static bool is_optimizable_op_pair(OpType a, OpType b) {
    if(a == b) {
        switch(a) {
            case OP_INCREMENT:
            case OP_DECREMENT:
            case OP_FORWARD:
            case OP_BACKWARD:
                return true;
            default:
                return false;
        }
    } else if(is_x_op(a) && remove_x_from_x_op(a) == b) {
        return true;
    } else if(is_x_op(b) && remove_x_from_x_op(b) == a) {
        return true;
    }
    return false;
}

static Op make_optimized_op(Op *a, Op *b) {
    if(a->type == b->type) {
        Op op = opNew(x_op_from_op(a->type));
        op.as.x = 2;
        return op;
    } else if(is_x_op(a->type)) {
        Op op = opNew(a->type);
        op.as.x = a->as.x + 1;
        return op;
    }
    UNREACHABLE();
}

struct optimized_op {
    Op op;
    uint32_t start, end;
};

static struct optimized_op *find_optimized_op(Vec(struct optimized_op) ops, uint32_t start_idx) {
    VEC_ITERATE(op, ops) {
        if(op->start == start_idx) {
            return op;
        }
    }
    return NULL;
}

// Note: ownership of [prog] is taken.
Vec(Op) optimize(Vec(Op) prog) {
    // Can't optimize less than 2 ops.
    if(VEC_LENGTH(prog) < 2) {
        return prog;
    }

    Window window = {NULL, NULL};
    OpIterator iter = opIteratorNew(prog);

    // fill the window (Note: assumes window length is 2)
    window_slide(window, &iter);
    window_slide(window, &iter);

    Vec(struct optimized_op) optimized_ops = VEC_NEW(struct optimized_op);
    while(!window_is_empty(window)) {
        if(window[0] && window[1]) {
            if(window[0]->type == OP_LOOP) {
                window[0]->as.loop_body = optimize(window[0]->as.loop_body);
            } else if(window[1]->type == OP_LOOP) {
                window[1]->as.loop_body = optimize(window[1]->as.loop_body);
            } else if(is_optimizable_op_pair(window[0]->type, window[1]->type)) {
                struct optimized_op optimized_op = (struct optimized_op){
                    .op = make_optimized_op(window[0], window[1]),
                    .start = opIteratorCurrentIdx(&iter) - 1,
                    .end = opIteratorCurrentIdx(&iter)
                };
                if(optimized_op.op.as.x > 2) {
                    struct optimized_op prev = VEC_POP(optimized_ops);
                    optimized_op.start = prev.start;
                    optimized_op.end = opIteratorCurrentIdx(&iter);
                }
                VEC_PUSH(optimized_ops, optimized_op);
                window[1] = &optimized_op.op;
            }
        }
        window_slide(window, &iter);
    }
    opIteratorFree(&iter);

    //VEC_ITERATE(op, optimized_ops) {
    //    printf(">%u:%u - ", op->start, op->end); opPrint(stdout, op->op); putchar('\n');
    //}

    Vec(Op) out = VEC_NEW(Op);
    VEC_FOREACH(i, prog) {
        struct optimized_op *optimized_op = find_optimized_op(optimized_ops, i);
        if(optimized_op != NULL) {
            VEC_PUSH(out, optimized_op->op);
            i += optimized_op->end - optimized_op->start;
        } else {
            VEC_PUSH(out, prog[i]);
        }
    }

    VEC_FREE(optimized_ops);
    VEC_FREE(prog);
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
                    execute(op->as.loop_body, tape);
                }
                break;
            default:
                fprintf(stderr, "Error: unkown op:\n");
                opPrint(stderr, *op);
                UNREACHABLE();
        }
    }
}

void compile_to_c(FILE *out, Vec(Op) prog) {
    VEC_ITERATE(op, prog) {
        switch(op->type) {
            case OP_INCREMENT:
                fputs("++*ptr;\n", out);
                break;
            case OP_INCREMENT_X:
                fprintf(out, "*ptr += %u;\n", op->as.x);
                break;
            case OP_DECREMENT:
                fputs("--*ptr;\n", out);
                break;
            case OP_DECREMENT_X:
                fprintf(out, "*ptr -= %u;\n", op->as.x);
                break;
            case OP_FORWARD:
                fputs("++ptr;\n", out);
                break;
            case OP_FORWARD_X:
                fprintf(out, "ptr += %u;\n", op->as.x);
                break;
            case OP_BACKWARD:
                fputs("--ptr;\n", out);
                break;
            case OP_BACKWARD_X:
                fprintf(out, "ptr -= %u;\n", op->as.x);
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
                UNREACHABLE();
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
    fprintf(stderr, "    -c [file] compile a file to C code.\n");
    fprintf(stderr, "    -o        Optimize the program.\n");
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
    bool opt_optimize = false;
    while((opt = getopt(argc, argv, "f:c:ho")) != -1) {
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
            case 'o':
                opt_optimize = true;
                break;
            default:
                continue;
        }
    }
    String input;
    if(opt_input_file) {
        input = stringNew(16); // 16 is just a random number.
        if(!read_file(&input, opt_input_file)) {
            fprintf(stderr, "Error: failed to read file '%s'!\n", opt_input_file);
        }
    } else {
        input = stringCopy(argv[optind]);
    }
    Compiler compiler = compilerNew(input);
    stringFree(input);
    Vec(Op) program = compile(&compiler, false);
    compilerFree(&compiler);
    if(!program) {
        return 1;
    }
    if(opt_optimize) {
        program = optimize(program);
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
