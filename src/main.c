#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "common.h"
#include "Vec.h"
#include "Strings.h"
#include "Ops.h"
#include "Compiler.h"
#include "Optimizer.h"
#include "Interpreter.h"

static void compile_to_c(FILE *out, Vec(Op) prog) {
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
    fprintf(stderr, "    [code]    Execute code directly from the first argument.\n");
    fprintf(stderr, "    -f [file] Execute a file.\n");
    fprintf(stderr, "    -c [file] Compile a file to C code.\n");
    fprintf(stderr, "    -o        Optimize the program.\n");
    fprintf(stderr, "    -d        Dump the compiled (and optimized if '-o' set) instructions.\n");
}

typedef struct options {
    char *input_file;
    bool compile_to_c;
    bool optimize;
    bool dump_instructions;
} Options;

static bool parse_arguments(Options *opts, int argc, char **argv) {
    if(argc < 2) {
        fputs("Error: insufficient arguments.\n", stderr);
        usage(argv[0]);
        return false;
    }

    int opt;
    bool had_error = false;
    while((opt = getopt(argc, argv, "f:c:hod")) != -1) {
        switch(opt) {
            case 'h':
                usage(argv[0]);
                return false;
            case 'f':
                opts->input_file = optarg;
                break;
            case 'c':
                opts->input_file = optarg;
                opts->compile_to_c = true;
                break;
            case 'o':
                opts->optimize = true;
                break;
            case 'd':
                opts->dump_instructions = true;
                break;
            case '?':
                had_error = true;
                break;
            default:
                UNREACHABLE();
        }
    }
    return !had_error; // had_error == true ? false : true
}

#define TAPE_SIZE 30000
int main(int argc, char **argv) {
    if(argc < 2) {
        fputs("Error: insufficient arguments.\n", stderr);
        usage(argv[0]);
        return 1;
    }
    Options opts = {
        .input_file = NULL,
        .compile_to_c = false,
        .optimize = false,
        .dump_instructions = false
    };
    if(!parse_arguments(&opts, argc, argv)) {
        return 1;
    }
    String input;
    if(opts.input_file) {
        input = stringNew(16); // 16 is just a random number.
        if(!read_file(&input, opts.input_file)) {
            fprintf(stderr, "Error: failed to read file '%s'!\n", opts.input_file);
            stringFree(input);
            return 1;
        }
    } else {
        input = stringCopy(argv[optind]);
    }
    Compiler compiler = compilerNew(input);
    stringFree(input);
    Vec(Op) program = compile(&compiler);
    compilerFree(&compiler);
    if(!program) {
        return 1;
    }
    if(opts.optimize) {
        program = optimize(program);
    }
    if(opts.dump_instructions) {
        VEC_ITERATE(op, program) {
            opPrint(stdout, *op);
            putchar('\n');
        }
    }
    if(opts.compile_to_c) {
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
        interpreterExecute(program, &tape);
        tapeFree(&tape);
    }
    VEC_ITERATE(op, program) { opFree(op); }
    VEC_FREE(program);
    return 0;
}
