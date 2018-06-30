
#pragma once
enum OpCode {
    plus_op = 1,
    minus_op = 2,
    mul_op = 3,
    div_op = 4,
    sin_op = 5,
    cos_op = 6,
    texture2D_op = 7,
    step_op = 8,
    length_op = 9,
    lt_op = 10,
    gt_op = 11,
    lte_op = 12,
    gte_op = 13,
    eq_op = 14,
    assign_op = 15,
    _define_op = 16,
    _forget_op = 17,
    _ifeq_op = 18,
    _endif_op = 19,
    _output_op = 20
};
#define MAX_OPS 21
