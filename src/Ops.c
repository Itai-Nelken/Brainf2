#include <stdio.h>
#include <stddef.h> // NULL
#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "Vec.h"
#include "Ops.h"

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

bool is_x_op(OpType op_type) {
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

OpType x_op_from_op(OpType op) {
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

OpType remove_x_from_x_op(OpType x_op) {
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


OpIterator opIteratorNew(Vec(Op) ops) {
    return (OpIterator){
        .ops = ops,
        .idx = 0
    };
}

void opIteratorFree(OpIterator *iter) {
    iter->ops = NULL;
    iter->idx = 0;
}

uint32_t opIteratorCurrentIdx(OpIterator *iter) {
    return iter->idx - 1;
}

Op *opIteratorNextOrNull(OpIterator *iter) {
    if(iter->idx < VEC_LENGTH(iter->ops)) {
        return &iter->ops[iter->idx++];
    }
    return NULL;
}
