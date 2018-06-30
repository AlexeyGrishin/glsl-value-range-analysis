#pragma once

#include "defs.h"
#include <cmath>
//todo: avoid std, if only string is used. redo with old good char*
//todo: with -nostdlib it is 13K, with - 500K O_o. better to build without, but will have to not use std::list and other stuff

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
	TypeRange(double left, double right) : flag(INCLUDE_BOTH), left(left), right(right) {}

	bool isSingle() const { return left == right;  }
	bool includes(double n) const;
	bool includes(const TypeRange& anotherRange) const;
	bool isValid() const;

	TypeRange operator+(const TypeRange& another) const;
	TypeRange operator-(const TypeRange& another) const;
	TypeRange operator*(const TypeRange& another) const;
	TypeRange operator/(const TypeRange& another) const;

	TypeRange& operator=(const TypeRange& another);
	bool operator==(const TypeRange& another) const;
	bool operator!=(const TypeRange& another) const;

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