#include <cheerp/clientlib.h>
#include <cheerp/client.h>
#include "model/defs.h"
#include "model/opcodes.h"
#include "model/range.h"
#include "model/command.h"
#include "model/analyzer.h"
#include "model/ops.h"

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
    DataFlowApi(): analyzer(NULL) {}

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
    void getSomething(client::Array* report) {
        report->push(new client::Number(123));
    }

    void getReport(client::Array* report) {
        Array<Warning> warnings = analyzer->getWarnings();

        for (int i = 0; i < warnings.count; i++) {
            Warning& w = warnings.items[i];
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

        Array<Branch> branches = analyzer->getBranches();

        for (int i = 0; i < branches.count; i++) {
            Branch& b = branches.items[i];
            client::Object* object = new client::Object();
            object->set_("type", new client::String("branch"));
            object->set_("branchId", new client::Number(b.id));
            object->set_("parentId", new client::Number(b.parentId));
            object->set_("cmdId", new client::Number(b.cmdId));
            object->set_("varId", new client::Number(b.varId));
            object->set_("active", new client::Number(b.active));
            report->push(object);
        }

        Array<VariableChange> changes = analyzer->getChanges();


        for (int i = 0; i < changes.count; i++) {
            VariableChange& c = changes.items[i];
            client::Object* object = new client::Object();
            object->set_("type", new client::String("change"));
            object->set_("branchId", new client::Number(c.branchId));
            object->set_("cmdId", new client::Number(c.cmdId));
            object->set_("varId", new client::Number(c.varId));
            object->set_("range", createRange(c.newRange));
            report->push(object);            
        }

        warnings.free();
        changes.free();
        branches.free();
    }
};

[[cheerp::genericjs]] void consoleLog(const char* str)
{
    	client::console.log(str);
}

int main() {
    //nothing to do
}

