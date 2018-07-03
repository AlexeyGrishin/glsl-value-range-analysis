//todo: move here structs and enums
#pragma once

#define MAX_RANGE_CHANGES 100
#define MAX_ARGS 14
#define MAX_VARIABLES 400
#define MAX_BRANCHES 100
#define MAX_WARNINGS 100

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))


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

enum TypeReason {
    TR_Operation = 1,
    TR_Branch = 2,
    TR_BackPropagation = 3
};

#define MAIN_BRANCH 0

//reproduces bug!
//#define NO_COPY_CC 
