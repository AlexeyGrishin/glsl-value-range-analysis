#pragma  once
#include "gtest/gtest.h"
#include "model/analyzer.h"
#include "model/opcodes.h"

#define ADDARG4(cmd,arg) cmd.addArgument(arg);cmd.addArgument(0);cmd.addArgument(0);cmd.addArgument(0);


void print(DataFlowAnalyzer& analyzer) {
    Array<Warning> warnings = analyzer.getWarnings();

    for (int i = 0; i < warnings.count; i++) {
        Warning& w = warnings.items[i];
        printf("!! warning when call %d@branch#%d: arg %d(var %d) has range %f-%f but expected %f-%f\n", 
            w.call, w.branchId, w.argumentNr, w.varId, 
            w.actualRange.left, w.actualRange.right, 
            w.expectedRange.left, w.expectedRange.right);
    }

    Array<Branch> branches = analyzer.getBranches();

    for (int i = 0; i < branches.count; i++) {
        Branch& b = branches.items[i];
        printf("-- branch %d (parent %d, %s) created for var %d changed in %d\n", b.id, b.parentId, b.active ? "active" : "inactive", b.varId, b.cmdId);
    }

    unsigned int changesCount = 0;
    Array<VariableChange> changes = analyzer.getChanges();


    for (int i = 0; i < changes.count; i++) {
        VariableChange& c = changes.items[i];
        printf("@@ var %d @ %d, cmd %d, change to %f-%f\n", 
            c.varId, c.branchId, c.cmdId, c.newRange.left, c.newRange.right
        );
    }

    warnings.free();
    changes.free();
    branches.free();
}

TEST(Analuzer, Creation) 
{
    {
        DataFlowAnalyzer analyzer;
    }

    //ok
}

TEST(Analyzer, SimpleScenario) 
{
    DataFlowAnalyzer analyzer;

    Command cmdDefine1(1, _define_op);      //var1 = [0,1]
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(0, 1));
    Command cmdDefine2(2, _define_op);      //var2 = [10,20]
    cmdDefine2.addArgument(2);
    cmdDefine2.setRange(TypeRange(10, 20));
    Command cmdDefine3(3, _define_op);      //var3
    cmdDefine3.addArgument(3);

    Command cmdPlus(4, plus_op);            //var3 = var1 + var2
    ADDARG4(cmdPlus, 3)
    ADDARG4(cmdPlus, 1)
    ADDARG4(cmdPlus, 2)
    cmdPlus.setRange(TypeRange(-12345,12345,(RangeFlag)255));

    Command cmdSin(5, sin_op);              //var1 = sin(var3)
    ADDARG4(cmdSin, 1)
    ADDARG4(cmdSin, 3)

    Command output(6, _output_op);          //output(var1,var2,var3,null)
    output.addArgument(1);
    output.addArgument(2);
    output.addArgument(3);
    output.addArgument(0);

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine2);
    analyzer.processCommand(&cmdDefine3);
    analyzer.processCommand(&cmdPlus);
    analyzer.processCommand(&cmdSin);
    analyzer.processCommand(&output);
    print(analyzer);

    //assert - warnings
    //assert - variable types
}


TEST(Analyzer, TestBranches_WithoutIf) {
    DataFlowAnalyzer analyzer;

    Command cmdDefine1(1, _define_op);      //var1 = [0,1]
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(0, 1));
    
    Command cmdDefine2(2, _define_op);      //var2 = 0.5
    cmdDefine2.addArgument(2);
    cmdDefine2.setRange(TypeRange(0.5));
    
    Command cmdDefine3(3, _define_op);
    cmdDefine3.addArgument(3);

    Command cmdLt(4, lt_op);        // var3 = var1 < var2
    cmdLt.addArgument(3);
    ADDARG4(cmdLt, 1);
    ADDARG4(cmdLt, 2);

    Command cmdDefine4(5, _define_op);  //var4
    cmdDefine4.addArgument(4);

    Command cmdAdd(6, plus_op);          //var4 = var1+var1
    ADDARG4(cmdAdd, 4);
    ADDARG4(cmdAdd, 1);
    ADDARG4(cmdAdd, 1);

    Command cmdOut(7, _output_op);      //output(var4,null,null,null)
    cmdOut.addArgument(4);

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine2);
    analyzer.processCommand(&cmdDefine3);
    analyzer.processCommand(&cmdLt);
    analyzer.processCommand(&cmdDefine4);
    analyzer.processCommand(&cmdAdd);
    analyzer.processCommand(&cmdOut);

    print(analyzer);
}

TEST(Analyzer, TestBranches_If) {
    DataFlowAnalyzer analyzer;

    Command cmdDefine1(1, _define_op);      //var1 = [0,1]
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(0, 1));
    
    Command cmdDefine2(2, _define_op);      //var2 = 0.5
    cmdDefine2.addArgument(2);
    cmdDefine2.setRange(TypeRange(0.5));
    
    Command cmdDefine3(3, _define_op);
    cmdDefine3.addArgument(3);

    Command cmdLt(4, lt_op);        // var3 = var1 < var2
    cmdLt.addArgument(3);
    ADDARG4(cmdLt, 1);
    ADDARG4(cmdLt, 2);

    Command cmdIf(5, _ifeq_op);     //if var3 == true (var1<var2)
    cmdIf.addArgument(3);
    cmdIf.setRange(TypeRange(1));

    Command cmdDefine4(6, _define_op);  //var4
    cmdDefine4.addArgument(4);

    Command cmdAdd(7, plus_op);          //var4 = var1+var1
    ADDARG4(cmdAdd, 4);
    ADDARG4(cmdAdd, 1);
    ADDARG4(cmdAdd, 1);

    Command endIf(8, _endif_op);
    endIf.addArgument(3);

    Command cmdIf2(9, _ifeq_op);     //if var3 != true (var1>=var2)
    cmdIf2.addArgument(3);
    cmdIf2.setRange(TypeRange(0));

    Command cmdDefine5(11, _define_op);  //var5 = 0.333
    cmdDefine5.addArgument(5);
    cmdDefine5.setRange(TypeRange(0.333));

    Command cmdAssign(12, assign_op);   //var4 = var5 
    ADDARG4(cmdAssign, 4)
    ADDARG4(cmdAssign, 5)

    Command endIf2(13, _endif_op);
    endIf2.addArgument(3);

    Command cmdOut(14, _output_op);      //output(var4,null,null,null)
    cmdOut.addArgument(4);

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine2);
    analyzer.processCommand(&cmdDefine3);
    analyzer.processCommand(&cmdLt);
    analyzer.processCommand(&cmdIf);
    analyzer.processCommand(&cmdDefine4);
    analyzer.processCommand(&cmdAdd);
    analyzer.processCommand(&endIf);
    analyzer.processCommand(&cmdIf2);
    cmdDefine4.cmdId = endIf.cmdId + 1;
    analyzer.processCommand(&cmdDefine4);
    analyzer.processCommand(&cmdDefine5);
    analyzer.processCommand(&cmdAssign);
    analyzer.processCommand(&endIf2);
    analyzer.processCommand(&cmdOut);

    print(analyzer);
}

