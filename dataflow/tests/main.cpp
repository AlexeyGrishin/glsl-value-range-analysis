#include "all.h"
#include "gtest/gtest.h"
#include "model/ops/builtin_ops.h"

int main(int argc, char** argv) {
  registerBuiltinOps();
 ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}