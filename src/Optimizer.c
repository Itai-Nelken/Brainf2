#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "Vec.h"
#include "Ops.h"
#include "Optimizer.h"

typedef Op *Window[2];

static void window_slide(Window window, OpIterator *iter) {
    window[0] = window[1];
    window[1] = opIteratorNextOrNull(iter);
}

static bool window_is_empty(Window window) {
    return window[0] == NULL && window[1] == NULL;
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
                struct optimized_op optimized_op = {
                    .op = make_optimized_op(window[0], window[1]),
                    .start = opIteratorCurrentIdx(&iter) - 1,
                    .end = opIteratorCurrentIdx(&iter)
                };
                if(optimized_op.op.as.x > 2) {
                    struct optimized_op prev = VEC_POP(optimized_ops);
                    optimized_op.start = prev.start;
                    // optimized_op.end is already correctly set above.
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
        // Warning: do NOT access 'prog[i]' here because 'i' might be out of bounds
        //          if an optimized op was pushed to 'out'.
    }

    VEC_FREE(optimized_ops);
    VEC_FREE(prog);
    return out;
}
