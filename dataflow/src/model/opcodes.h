
#pragma once
enum OpCode {
    plus_op = 1,
    minus_op = 2,
    mul_op = 3,
    div_op = 4,
    sin_op = 5,
    cos_op = 6,
    atan_op = 7,
    texture2D_op = 8,
    step_op = 9,
    length_op = 10,
    lt_op = 11,
    gt_op = 12,
    lte_op = 13,
    gte_op = 14,
    eq_op = 15,
    assign_op = 16,
    max_op = 17,
    min_op = 18,
    fract_op = 19,
    power_op = 20,
    mix_op = 21,
    dot_op = 22,
    clamp_op = 23,
    normalize_op = 24,
    smoothstep_op = 25,
    floor_op = 26,
    cross_op = 27,
    unary_minus_op = 28,
    or_op = 29,
    and_op = 30,
    _define_op = 31,
    _forget_op = 32,
    _ifeq_op = 33,
    _endif_op = 34,
    _output_op = 35,
    _watch_op = 36,
    _ignore_op = 37,
    _endwatch_op = 38
};
#define MAX_OPS 39
