#ifndef OPS_H
#define OPS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

// Note: update op_type_str():op_names[] in Ops.c when adding/removing ops.
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

typedef struct op_iterator {
    Vec(Op) ops;
    uint32_t idx;
} OpIterator;

Op opNew(OpType type);
void opFree(Op *op);
void opPrint(FILE *to, Op op);

bool is_x_op(OpType op_type);
OpType x_op_from_op(OpType op);
OpType remove_x_from_x_op(OpType x_op);

OpIterator opIteratorNew(Vec(Op) ops);
void opIteratorFree(OpIterator *iter);
uint32_t opIteratorCurrentIdx(OpIterator *iter);
Op *opIteratorNextOrNull(OpIterator *iter);

#endif // OPS_H
