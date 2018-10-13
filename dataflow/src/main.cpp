// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <cheerp/clientlib.h>
#include <cheerp/client.h>
#include "model/defs.h"
#include "model/opcodes.h"
#include "model/range.h"
#include "model/command.h"
#include "model/analyzer.h"
#include "model/ops.h"
#include "model/ops/builtin_ops.h"

#include <vector>

namespace client {
    [[cheerp::genericjs]]
    Object*  createRange(double left, double right, int flag);
}

[[cheerp::genericjs]]
client::Object*  createRange(const TypeRange& range) {
    return client::createRange(range.left, range.right, range.flag);
}

class [[cheerp::genericjs]] [[cheerp::jsexport]] DataFlowApi {
private:
    DataFlowAnalyzer* analyzer;
public:
    DataFlowApi(): analyzer(nullptr) {}

    void init() {
        analyzer = new DataFlowAnalyzer();
    }

    void fini() {
        delete analyzer;
    }

    int addCommand(CmdId cmdId, OpCode opCode, client::Array* varIds, double left, double right, int flag) {
        Command cmd(cmdId, opCode);
        TypeRange range(left, right, (RangeFlag)flag);
        cmd.setRange(range);
        auto ref = cheerp::makeArrayRef(varIds);
        for(int i=0; i<ref->get_length(); i++) {
            cmd.addArgument((int)((double)*ref[i]));
        }
        return analyzer->processCommand(&cmd);
    }

    void getReport(client::Array* report) {
        auto warnings = analyzer->getWarnings();

        for (auto& w: warnings) {
            client::Object* object = new client::Object();
            object->set_("type", new client::String("warning"));
            object->set_("opCode", new client::Number(w.call));
            object->set_("cmdId", new client::Number(w.cmdId));
            object->set_("branchId", new client::Number(w.branchId));
            object->set_("argumentNr", new client::Number(w.argumentNr));
            object->set_("varId", new client::Number(w.varId));
            object->set_("actualRange", createRange(w.actualRange));
            object->set_("expectedRange", createRange(w.expectedRange));
            report->push(object);
        }

        auto branches = analyzer->getBranches();

        for (auto& b: branches) {
            client::Object* object = new client::Object();
            object->set_("type", new client::String("branch"));
            object->set_("branchId", new client::Number(b.id));
            object->set_("parentId", new client::Number(b.parentId));
            object->set_("cmdId", new client::Number(b.cmdId));
            object->set_("varId", new client::Number(b.varId));
            object->set_("active", new client::Number(b.active));
            report->push(object);
        }

        auto changes = analyzer->getChanges();
        for (auto& c: changes) {
            client::Object* object = new client::Object();
            object->set_("type", new client::String("change"));
            object->set_("branchId", new client::Number(c.branchId));
            object->set_("cmdId", new client::Number(c.cmdId));
            object->set_("varId", new client::Number(c.varId));
            object->set_("range", createRange(c.newRange));
            object->set_("revertable", new client::Number(c.revertable));
            switch (c.reason) {
                case TR_Operation:
                    object->set_("reason", new client::String("operation"));
                    break;
                case TR_Branch:
                    object->set_("reason", new client::String("branch"));
                    break;
                case TR_BackPropagation:
                    object->set_("reason", new client::String("back_propagation"));
                    break;
            }
            report->push(object);            
        }
    }
};

[[cheerp::genericjs]] void consoleLog(const char* str) {
        client::console.log(str);
}

[[cheerp::genericjs]] void consoleLog(unsigned int i) {
        client::console.log(i);
}

[[cheerp::genericjs]] void consoleLog(double d) {
        client::console.log(d);
}

int main() {
    //nothing to do
    registerBuiltinOps();
}

