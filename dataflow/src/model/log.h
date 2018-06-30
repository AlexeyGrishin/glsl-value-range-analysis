#pragma once
#define NOLOG

#ifdef TEST
  #include "stdio.h"
#else
  #ifdef NOLOG
    #define printf(...)
    #define fflush(...)
  #else
    #include <cheerp/clientlib.h>
    #include <cheerp/client.h>
    [[cheerp::genericjs]] void consoleLog(const char*);
    #define printf(x) consoleLog(x) 
    #define fflush(...)
  #endif
#endif