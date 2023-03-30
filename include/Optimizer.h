#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "Vec.h"
#include "Ops.h"

// Note: ownership of [prog] is taken.
Vec(Op) optimize(Vec(Op) prog);

#endif // OPTIMIZER_H
