#pragma once
#ifdef TEST
#include "stdio.h"
#else
#define printf(...) 
#define fflush(...)
#endif


#define MAX_RANGE_CHANGES 100
#define MAX_ARGS 14
#define MAX_VARIABLES 400
#define MAX_BRANCHES 100
#define MAX_WARNINGS 100

typedef unsigned int CmdId;
typedef unsigned int BranchId;
typedef unsigned int VarId;

template<class T>
struct Array {
    T* items;
    unsigned int count;
    Array(unsigned int count) { allocate(count); }
    Array(T* items, unsigned int count): items(items), count(count) {}

    void allocate(unsigned int count) { items = new T[count]; this->count = count; }
    void free() { delete[] items; }
};

#define MAIN_BRANCH 0

//reproduces bug!
//#define NO_COPY_CC 
