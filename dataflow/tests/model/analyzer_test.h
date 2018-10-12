#pragma  once
#include "gtest/gtest.h"
#include "model/analyzer.h"
#include "model/opcodes.h"

#define ADDARG4(cmd,arg) cmd.addArgument(arg);cmd.addArgument(0);cmd.addArgument(0);cmd.addArgument(0);

#define ASSERT_PROCESS(expectedCode, command) ASSERT_EQ(expectedCode, analyzer.processCommand(&command));

void print(DataFlowAnalyzer& analyzer) {
    auto warnings = analyzer.getWarnings();

    for (auto& w: warnings) {
        printf("!! warning when call %d@branch#%d: arg %d(var %d) has range %f-%f but expected %f-%f\n", 
            w.call, w.branchId, w.argumentNr, w.varId, 
            w.actualRange.left, w.actualRange.right, 
            w.expectedRange.left, w.expectedRange.right);
    }

    auto branches = analyzer.getBranches();

    for (auto& b: branches) {
        printf("-- branch %d (parent %d, %s) created for var %d changed in %d\n", b.id, b.parentId, b.active ? "active" : "inactive", b.varId, b.cmdId);
    }

    unsigned int changesCount = 0;
    auto changes = analyzer.getChanges();

    for (auto& c: changes) {
        printf("@@ var %d @ %d, cmd %d, change to %f-%f %s\n", 
            c.varId, c.branchId, c.cmdId, c.newRange.left, c.newRange.right,
            c.revertable ? "[revertable]" : ""
        );
    }
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

TEST(Analyzer, Test_Dependencies) {
        DataFlowAnalyzer analyzer;

    Command cmdDefine1(1, _define_op);      //var1 = [0,1]
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(0, 1));
    
    Command cmdDefine2(2, _define_op);      //var2 = 0.5
    cmdDefine2.addArgument(2);
    cmdDefine2.setRange(TypeRange(0.5));
    
    Command cmdDefine3(3, _define_op);      //var3
    cmdDefine3.addArgument(3);

    Command cmdDefine4(4, _define_op);      //var4
    cmdDefine4.addArgument(4);

    Command cmdPlus(5, plus_op);            //var3 = var1+var2
    ADDARG4(cmdPlus, 3);
    ADDARG4(cmdPlus, 1);
    ADDARG4(cmdPlus, 2);

    Command cmdDefine5(6, _define_op);      //var5= 1
    cmdDefine5.addArgument(5);
    cmdDefine5.setRange(TypeRange(1));

    Command cmdLt(7, lt_op);                //var4 = var3<1
    cmdLt.addArgument(4);
    ADDARG4(cmdLt, 3);
    ADDARG4(cmdLt, 5);

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine2);
    analyzer.processCommand(&cmdDefine3);
    analyzer.processCommand(&cmdDefine4);
    analyzer.processCommand(&cmdPlus);
    analyzer.processCommand(&cmdDefine5);
    analyzer.processCommand(&cmdLt);
    //print(analyzer);

    //compare result
    ASSERT_EQ(TypeRange(1), *analyzer.getRange(1, 4));
    ASSERT_EQ(TypeRange(0), *analyzer.getRange(2, 4));

    //compared value (var3)
    ASSERT_EQ(TypeRange(0.5,1,INCLUDE_LEFT), *analyzer.getRange(1, 3));
    ASSERT_EQ(TypeRange(1,1.5), *analyzer.getRange(2, 3));

    //dependent value (var1)
    ASSERT_EQ(TypeRange(0,0.5,INCLUDE_LEFT), *analyzer.getRange(1, 1));
    ASSERT_EQ(TypeRange(0.5, 1), *analyzer.getRange(2, 1));

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



TEST(Analyzer, TestStep) {
    DataFlowAnalyzer analyzer;

    Command cmdDefine1(1, _define_op);      //var1 = [0,1]
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(0, 1));
    
    Command cmdDefine2(2, _define_op);      //var2 = 0.5
    cmdDefine2.addArgument(2);
    cmdDefine2.setRange(TypeRange(0.5));
    
    Command cmdDefine3(3, _define_op);      //var3
    cmdDefine3.addArgument(3);

    Command cmdDefine4(4, _define_op);      //var4
    cmdDefine4.addArgument(4);

    Command cmdStep(5, step_op);        //var3, var4, _, _ = step(0.5, var1, var1, _, _)
    cmdStep.addArgument(3);
    cmdStep.addArgument(4);
    cmdStep.addArgument(0);
    cmdStep.addArgument(0);
    cmdStep.addArgument(2);
    cmdStep.addArgument(0);
    cmdStep.addArgument(0);
    cmdStep.addArgument(0);
    cmdStep.addArgument(1);
    cmdStep.addArgument(1);
    cmdStep.addArgument(0);
    cmdStep.addArgument(0);

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine2);
    analyzer.processCommand(&cmdDefine3);
    analyzer.processCommand(&cmdDefine4);
    analyzer.processCommand(&cmdStep);

    print(analyzer);
    ASSERT_EQ(5, analyzer.getBranches().size());

    //todo: for now it is ok, but actually I need ALL variant combinations, like var3=1, var4=0, etc.
    ASSERT_EQ(TypeRange(0), *analyzer.getRange(1, 3));
    ASSERT_EQ(TypeRange(0), *analyzer.getRange(1, 4));
    ASSERT_EQ(TypeRange(1), *analyzer.getRange(2, 3));
    ASSERT_EQ(TypeRange(1), *analyzer.getRange(2, 4));
    ASSERT_EQ(TypeRange(0), *analyzer.getRange(3, 3));
    ASSERT_EQ(TypeRange(0), *analyzer.getRange(3, 4));
    ASSERT_EQ(TypeRange(1), *analyzer.getRange(4, 3));
    ASSERT_EQ(TypeRange(1), *analyzer.getRange(4, 4));

}

TEST(Analyzer, WatchEndWatchIgnore) {
    DataFlowAnalyzer analyzer;

    //const1 = 0.5
    Command cmdDefine1(1, _define_op);
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(0.5));
    //a = 0
    Command cmdDefine2(2, _define_op); 
    cmdDefine2.addArgument(2);
    cmdDefine2.setRange(TypeRange(0));
    //_watch a
    Command cmdWatch3(3, _watch_op);
    cmdWatch3.addArgument(2);
    //a = a + 0.5
    Command cmdPlus4(4, plus_op);
    ADDARG4(cmdPlus4, 2)
    ADDARG4(cmdPlus4, 2)
    ADDARG4(cmdPlus4, 1)
    //_ignore i
    Command cmdIgnore5(5, _ignore_op);
    cmdIgnore5.addArgument(2);
    //a = a + 0.5
    Command cmdPlus6(6, plus_op);
    ADDARG4(cmdPlus6, 2)
    ADDARG4(cmdPlus6, 2)
    ADDARG4(cmdPlus6, 1)
    //_endwatch i
    Command cmdEnd7(7, _endwatch_op);
    cmdEnd7.addArgument(2);
    //a = a - 0.5
    Command cmdMinus8(8, minus_op);
    ADDARG4(cmdMinus8, 2)
    ADDARG4(cmdMinus8, 2)
    ADDARG4(cmdMinus8, 1)

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine2);
    analyzer.processCommand(&cmdWatch3);
    analyzer.processCommand(&cmdPlus4);
    analyzer.processCommand(&cmdIgnore5);
    analyzer.processCommand(&cmdPlus6);
    analyzer.processCommand(&cmdEnd7);
    analyzer.processCommand(&cmdMinus8);

    print(analyzer);
    ASSERT_EQ(1, analyzer.getBranches().size());
    ASSERT_EQ(TypeRange(0), *analyzer.getRange(MAIN_BRANCH, 2));
    
}

TEST(Analyzer, WatchEndWatchIgnore_Branches) {
    DataFlowAnalyzer analyzer;
    //const1 = 0.5
    Command cmdDefine1(1, _define_op);
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(0.5));

    //const2 = 1
    Command cmdDefine2(2, _define_op);
    cmdDefine2.addArgument(2);
    cmdDefine2.setRange(TypeRange(1));
    //a3 = [0, 1]
    Command cmdDefine3(3, _define_op);
    cmdDefine3.addArgument(3);
    cmdDefine3.setRange(TypeRange(0, 1));
    //_watch a3
    Command cmdWatch4(4, _watch_op);
    cmdWatch4.addArgument(3);
    //a = a + 0.5  // [0.5, 1.5]
    Command cmdPlus5(5, plus_op);
    ADDARG4(cmdPlus5, 3);
    ADDARG4(cmdPlus5, 3);
    ADDARG4(cmdPlus5, 1);
    //_if a > 1    // [0.5, 1] vs (1, 1.5]
    Command cmdDefine6(6, _define_op);
    cmdDefine6.addArgument(4);
    Command cmdGt7(7, gt_op);
    cmdGt7.addArgument(4);
    ADDARG4(cmdGt7, 3);
    ADDARG4(cmdGt7, 2);
    Command cmdIf8(8, _ifeq_op);
    cmdIf8.addArgument(4);
    cmdIf8.setRange(TypeRange(1));
    //_ignore i
    Command cmdIgnore9(9, _ignore_op);
    cmdIgnore9.addArgument(3);
    //_endif
    Command cmdEndIf10(10, _endif_op);
    cmdEndIf10.addArgument(4);
    //a = a - 0.5  // [0, 0.5] vs (1, 1.5]
    Command cmdMinus11(11, minus_op);
    ADDARG4(cmdMinus11, 3);
    ADDARG4(cmdMinus11, 3);
    ADDARG4(cmdMinus11, 1);
    //_endwatch i  
    Command cmdEndWatch12(12, _endwatch_op);
    cmdEndWatch12.addArgument(3);

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine2);
    analyzer.processCommand(&cmdDefine3);
    analyzer.processCommand(&cmdWatch4);
    analyzer.processCommand(&cmdPlus5);
    analyzer.processCommand(&cmdDefine6);
    analyzer.processCommand(&cmdGt7);
    analyzer.processCommand(&cmdIf8);
    analyzer.processCommand(&cmdIgnore9);
    analyzer.processCommand(&cmdEndIf10);
    analyzer.processCommand(&cmdMinus11);
    analyzer.processCommand(&cmdEndWatch12);

    print(analyzer);
    ASSERT_EQ(3, analyzer.getBranches().size());
    ASSERT_EQ(TypeRange(0, 0.5), *analyzer.getRange(1, 3));
    ASSERT_EQ(TypeRange(1, 1.5, INCLUDE_RIGHT), *analyzer.getRange(2, 3));

}


TEST(Analyzer, Lt_Branches) {
    DataFlowAnalyzer analyzer;
    //const1 = 0.5
    Command cmdDefine1(1, _define_op);
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(0.5));
    //var2 = [0,1];
    Command cmdDefine2(2, _define_op);
    cmdDefine2.addArgument(2);
    cmdDefine2.setRange(TypeRange(0, 1));
    //var3 = var2 < const1
    Command cmdDefine3(3, _define_op);
    cmdDefine3.addArgument(3);

    Command cmdLt4(4, lt_op);
    cmdLt4.addArgument(3);
    ADDARG4(cmdLt4, 2);
    ADDARG4(cmdLt4, 1);

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine2);
    analyzer.processCommand(&cmdDefine3);
    analyzer.processCommand(&cmdLt4);

    print(analyzer);
    ASSERT_EQ(3, analyzer.getBranches().size());
    ASSERT_EQ(TypeRange(0, 0.5, INCLUDE_LEFT), *analyzer.getRange(1, 2));
    ASSERT_EQ(TypeRange(0.5,1), *analyzer.getRange(2, 2));
}

TEST(Analyzer, Lt_AlwaysSingleBranch) {
    DataFlowAnalyzer analyzer;
    //const1 = 2
    Command cmdDefine1(1, _define_op);
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(2));
    //var2 = [0,1];
    Command cmdDefine2(2, _define_op);
    cmdDefine2.addArgument(2);
    cmdDefine2.setRange(TypeRange(0, 1));
    //var3 = var2 < const1
    Command cmdDefine3(3, _define_op);
    cmdDefine3.addArgument(3);

    Command cmdLt4(4, lt_op);
    cmdLt4.addArgument(3);
    ADDARG4(cmdLt4, 2);
    ADDARG4(cmdLt4, 1);

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine2);
    analyzer.processCommand(&cmdDefine3);
    analyzer.processCommand(&cmdLt4);

    print(analyzer);
    //expected - branch is not created
    //ASSERT_EQ(1, analyzer.getBranches().size());
    //ASSERT_EQ(TypeRange(0, 1), *analyzer.getRange(0, 2));
    //actual - single branch is craeted.
    ASSERT_EQ(2, analyzer.getBranches().size());
    ASSERT_EQ(TypeRange(0, 1), *analyzer.getRange(1, 2));
}

TEST(Analyzer, Lt_SingleBranch_Consts) {
    DataFlowAnalyzer analyzer;
    //var1 = 1
    Command cmdDefine1(1, _define_op);
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(1));
    //var3 = var2 < const1
    Command cmdDefine3(3, _define_op);
    cmdDefine3.addArgument(3);
    Command cmdLt4(4, lt_op);
    cmdLt4.addArgument(3);
    ADDARG4(cmdLt4, 1);
    ADDARG4(cmdLt4, 1);

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine3);
    analyzer.processCommand(&cmdLt4);

    print(analyzer);
    //expected - branch is not created
    //ASSERT_EQ(1, analyzer.getBranches().size());
    //ASSERT_EQ(TypeRange(0, 1), *analyzer.getRange(0, 2));
    //actual - single branch is craeted.
    ASSERT_EQ(2, analyzer.getBranches().size());
    ASSERT_EQ(TypeRange(1), *analyzer.getRange(1, 1));

}

TEST(Analyzer, UnaryMinus) {
    DataFlowAnalyzer analyzer;

    //const var1 = 0,1
    Command cmdDefine1(1, _define_op);
    cmdDefine1.addArgument(1);
    cmdDefine1.setRange(TypeRange(0, 1));
    //const var2 = -var1
    Command cmdDefine2(2, _define_op);
    cmdDefine2.addArgument(2);
    Command cmdUnary3(3, unary_minus_op);
    ADDARG4(cmdUnary3, 2);
    ADDARG4(cmdUnary3, 1);
    //const var3 = -0.5
    Command cmdDefine4(4, _define_op);
    cmdDefine4.addArgument(3);
    cmdDefine4.setRange(TypeRange(-0.5));
    //tmp4 = var2 < var3    
    Command cmdDefine5(5, _define_op);
    cmdDefine5.addArgument(4);
    Command cmdLt6(6, lt_op);
    cmdLt6.addArgument(4);
    ADDARG4(cmdLt6, 2);
    ADDARG4(cmdLt6, 3);

    analyzer.processCommand(&cmdDefine1);
    analyzer.processCommand(&cmdDefine2);
    analyzer.processCommand(&cmdUnary3);
    analyzer.processCommand(&cmdDefine4);
    analyzer.processCommand(&cmdDefine5);
    analyzer.processCommand(&cmdLt6);

    //branch 1 - var2 = [-1, -0.5), var1 = (0.5, 1]
    //branch 2 - var2 = [-0.5, 0], var2 = [0, 0.5]

    print(analyzer);

    ASSERT_EQ(3, analyzer.getBranches().size());
    ASSERT_EQ(TypeRange(-1, -0.5, INCLUDE_LEFT), *analyzer.getRange(1, 2));
    ASSERT_EQ(TypeRange(0.5, 1, INCLUDE_RIGHT), *analyzer.getRange(1, 1));

    ASSERT_EQ(TypeRange(-0.5, 0), *analyzer.getRange(2, 2));
    ASSERT_EQ(TypeRange(0, 0.5), *analyzer.getRange(2, 1));
}

TEST(Analyzer, Errors) {

    DataFlowAnalyzer analyzer;

    Command cmdDefine1(1, _define_op);
    cmdDefine1.addArgument(1);

    Command cmdDefine2(2, _define_op);
    cmdDefine2.addArgument(2);
    
    Command cmdPlus_unknownOutVar3(3, plus_op);
    ADDARG4(cmdPlus_unknownOutVar3, 3);
    ADDARG4(cmdPlus_unknownOutVar3, 1);
    ADDARG4(cmdPlus_unknownOutVar3, 2);

    Command cmdPlus_unknownInVarFirst4(4, plus_op);
    ADDARG4(cmdPlus_unknownInVarFirst4, 2);
    ADDARG4(cmdPlus_unknownInVarFirst4, 3);
    ADDARG4(cmdPlus_unknownInVarFirst4, 2);

    Command cmdPlus_unknownInVarSecond5(5, plus_op);
    ADDARG4(cmdPlus_unknownInVarSecond5, 2);
    ADDARG4(cmdPlus_unknownInVarSecond5, 3);
    ADDARG4(cmdPlus_unknownInVarSecond5, 2);

    Command cmdPlus_moreOutVars6(6, plus_op);
    cmdPlus_moreOutVars6.addArgument(1);
    cmdPlus_moreOutVars6.addArgument(1);
    cmdPlus_moreOutVars6.addArgument(0);
    cmdPlus_moreOutVars6.addArgument(0);
    ADDARG4(cmdPlus_moreOutVars6, 2);
    ADDARG4(cmdPlus_moreOutVars6, 2);

    Command unknownCommand7(7, (OpCode)999);

    ASSERT_PROCESS(PR_OK, cmdDefine1);
    ASSERT_PROCESS(PR_OK, cmdDefine2);
    ASSERT_PROCESS(PR_UNKNOWN_VAR, cmdPlus_unknownOutVar3);
    ASSERT_PROCESS(PR_UNKNOWN_VAR, cmdPlus_unknownInVarFirst4);
    ASSERT_PROCESS(PR_UNKNOWN_VAR, cmdPlus_unknownInVarSecond5);
    ASSERT_PROCESS(PR_ABSENT_ARGUMENT, cmdPlus_moreOutVars6);
    ASSERT_PROCESS(PR_UNKNOWN_OP, unknownCommand7);


}