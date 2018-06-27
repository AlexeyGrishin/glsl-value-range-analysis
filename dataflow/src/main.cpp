#include "model/range.h"
#include <cheerp/clientlib.h>
#include <cheerp/client.h>

[[cheerp::genericjs]] void consoleLog(const char* str)
{
    	client::console.log(str);
}

int main() {
    Range r;
    if (r.includes(1.0)) {
        consoleLog("hello there");
    } else {
        consoleLog("general kenobi");
    }
    return 1;
}

