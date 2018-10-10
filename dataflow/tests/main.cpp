// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "all.h"
#include "gtest/gtest.h"
#include "model/ops/builtin_ops.h"

int main(int argc, char** argv) {
  registerBuiltinOps();
 ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}