#pragma once

#include "defs.h"
#include "log.h"
#include <cmath>

enum RangeFlag {
    IGNORE = 0xff,
    EXCLUDE_BOTH = 0x00,
    INCLUDE_LEFT = 0x01,
    INCLUDE_RIGHT = 0x02,
    INCLUDE_BOTH = 0x03
};

struct TypeRange {
    RangeFlag flag;
    double left;
    double right;

    TypeRange() : flag(EXCLUDE_BOTH), left(-INFINITY), right(INFINITY) {}
    #ifndef NO_COPY_CC
    TypeRange(const TypeRange& another): flag(another.flag), left(another.left), right(another.right) {}
    #endif
    TypeRange(RangeFlag flag) : flag(flag), left(0), right(0) {}
    TypeRange(double single) : flag(INCLUDE_BOTH), left(single), right(single) {}
    TypeRange(double left, double right, RangeFlag flag) : flag(flag), left(left), right(right) {}
    constexpr TypeRange(double left, double right) : flag(INCLUDE_BOTH), left(left), right(right) {}

    bool isSingle() const;
    bool includes(double n) const;
    bool includes(const TypeRange& anotherRange) const;
    bool isValid() const;

    TypeRange operator+(const TypeRange& another) const;
    TypeRange operator-(const TypeRange& another) const;
    TypeRange operator*(const TypeRange& another) const;
    TypeRange operator/(const TypeRange& another) const;
    TypeRange operator-() const;

    TypeRange& operator=(const TypeRange& another);
    bool operator==(const TypeRange& another) const;
    bool operator!=(const TypeRange& another) const;

    operator bool() const;

    #ifdef TEST
    void print() const {
        if (flag == IGNORE) {
            printf("[?]");
            return;
        }
        if (flag & INCLUDE_LEFT) printf("["); else printf("(");
        printf("%.3f", left);
        if (left != right) {
            printf(",%.3f", right);
        }
        if (flag & INCLUDE_RIGHT) printf("]"); else printf(")");
    }
    void println() const {
        print(); printf("\n");
    }
    #else
    void print() const {}
    void println() const {}
    #endif
    
};

//todo: maybe use TypeRange with IGNORE flag instead of invalid one
constexpr TypeRange InvalidRange(1, -1);
constexpr TypeRange Zero(0, 0);