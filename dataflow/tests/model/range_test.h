#pragma once
#include "model/range.h"
#include "gtest/gtest.h"

TEST(Range, One) {
    Range r;
    ASSERT_TRUE(r.includes(1.0));
}