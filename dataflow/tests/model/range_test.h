#pragma  once
#include "gtest/gtest.h"
#include "model/range.h"
#include "stdio.h"
#include "model/ops/range_ops.h"

std::ostream& operator<<(std::ostream& stream, TypeRange const& range)
{
    if (range.flag == IGNORE) {
        stream << "[?]";
        return stream;
    }
    stream << ((range.flag & INCLUDE_LEFT) ? "[" : "(");
    stream << range.left;
    if (range.left != range.right) {
        stream << "," << range.right;
    }
    stream << ((range.flag & INCLUDE_RIGHT) ? "]" : ")");  
    return stream;
}

TEST(Range, Simple)
{
       TypeRange range(4, 5);
       ASSERT_TRUE(range.includes(4));
       ASSERT_TRUE(range.includes(4.5));
       ASSERT_TRUE(range.includes(5));
       ASSERT_FALSE(range.includes(3.99));
       ASSERT_FALSE(range.includes(5.01));
}
TEST(Range, Simple_LeftExcluded)
{
       TypeRange range(4, 5, INCLUDE_RIGHT);
       ASSERT_FALSE(range.includes(4));
       ASSERT_TRUE(range.includes(4.5));
       ASSERT_TRUE(range.includes(5));
}
TEST(Range, Simple_RightExcluded)
{
       TypeRange range(4, 5, INCLUDE_LEFT);
       ASSERT_TRUE(range.includes(4));
       ASSERT_TRUE(range.includes(4.5));
       ASSERT_FALSE(range.includes(5));
}
TEST(Range, Simple_BothExcluded)
{
       TypeRange range(4, 5, EXCLUDE_BOTH);
       ASSERT_FALSE(range.includes(4));
       ASSERT_TRUE(range.includes(4.5));
       ASSERT_FALSE(range.includes(5));
}
TEST(Range, Simple_Single)
{
       TypeRange single(6);
       ASSERT_FALSE(single.includes(5.99));
       ASSERT_TRUE(single.includes(6));
       ASSERT_FALSE(single.includes(6.01));
       ASSERT_TRUE(single.isSingle());
}
TEST(Range, Simple_Infinity) {
       TypeRange infinite;
       ASSERT_TRUE(infinite.includes(-10000));
       ASSERT_TRUE(infinite.includes(10000));
}
TEST(Range, Sum_Simple) {
       TypeRange r1(1, 2);
       TypeRange r2(4, 5);
       TypeRange sum(r1 + r2);
       ASSERT_TRUE(sum.includes(5));
       ASSERT_TRUE(sum.includes(7));
       ASSERT_DOUBLE_EQ(5, sum.left);
       ASSERT_DOUBLE_EQ(7, sum.right);
       ASSERT_EQ(INCLUDE_BOTH, sum.flag);
}
TEST(Range, Sum_LeftExcluded) {
       TypeRange r1(1, 2);
       TypeRange r2(4, 5, INCLUDE_RIGHT);
       TypeRange sum(r1 + r2);
       ASSERT_FALSE(sum.includes(5));
       ASSERT_TRUE(sum.includes(5.0001));
       ASSERT_TRUE(sum.includes(7));
       ASSERT_DOUBLE_EQ(5, sum.left);
       ASSERT_DOUBLE_EQ(7, sum.right);
       ASSERT_EQ(INCLUDE_RIGHT, sum.flag);
}
TEST(Range, Sum_LeftInfinite) {
       TypeRange r1(-INFINITY, 2);
       TypeRange r2(4, 5);
       TypeRange sum(r1 + r2);
       ASSERT_TRUE(sum.includes(-1000));
       ASSERT_TRUE(sum.includes(5));
       ASSERT_TRUE(sum.includes(7));
       ASSERT_FALSE(sum.includes(7.0001));
       ASSERT_DOUBLE_EQ(-INFINITY, sum.left);
       ASSERT_DOUBLE_EQ(7, sum.right);
}
TEST(Range, Mul) {
       TypeRange r1(-1, 2);
       TypeRange r2(-10, 3);
       TypeRange mul(r1*r2);
       ASSERT_FALSE(mul.includes(-21));
       ASSERT_TRUE(mul.includes(-20));
       ASSERT_TRUE(mul.includes(-19));
       ASSERT_TRUE(mul.includes(10));
       ASSERT_FALSE(mul.includes(10.1));
}
TEST(Range, Mul_RightInfinite) {
       TypeRange r1(1, 2);
       TypeRange r2(3, INFINITY);
       TypeRange mul(r1*r2);
       ASSERT_FALSE(mul.includes(2.9));
       ASSERT_TRUE(mul.includes(3));
       ASSERT_TRUE(mul.includes(5000));
}
TEST(Range, Mul_RightInfinite_Vs_Negative) {
       TypeRange r1(-1, 2);
       TypeRange r2(3, INFINITY);
       TypeRange mul(r1*r2);
       ASSERT_TRUE(mul.includes(-5000));
       ASSERT_TRUE(mul.includes(5000));
}
TEST(Range, Div_Simple) {
       TypeRange r1(10, 20);
       TypeRange s1(5);
       TypeRange div(r1 / s1);
       ASSERT_FALSE(div.includes(1.9));
       ASSERT_TRUE(div.includes(2));
       ASSERT_TRUE(div.includes(3));
       ASSERT_TRUE(div.includes(4));
       ASSERT_FALSE(div.includes(4.1));
}


TEST(Range, IncludeAnother_Inside) {
    TypeRange r1(10, 20);
    TypeRange r2(12, 18);
    ASSERT_TRUE(r1.includes(r2));
}

TEST(Range, IncludeAnother_Equal) {
    TypeRange r1(10, 20);
    TypeRange r2(10, 20);
    ASSERT_TRUE(r1.includes(r2));
}

TEST(Range, IncludeAnother_LeftEdgeOutside) {
    TypeRange r1(10, 20);
    TypeRange r2(9, 18);
    ASSERT_FALSE(r1.includes(r2));
}

TEST(Range, IncludeAnother_RightEdgeOutside) {
    TypeRange r1(10, 20);
    TypeRange r2(12, 21);
    ASSERT_FALSE(r1.includes(r2));
}

TEST(Range, IncludeAnother_Outside) {
    TypeRange r1(10, 20);
    TypeRange r2(9, 22);
    ASSERT_FALSE(r1.includes(r2));
}

TEST(Range, IncludeAnother_EqualLeftExcluded) {
    TypeRange r1(10, 20);
    TypeRange r2(10, 20, INCLUDE_RIGHT);
    ASSERT_TRUE(r1.includes(r2));
}

TEST(Range, IncludeAnother_EqualLeftExcluded2) {
    TypeRange r1(10, 20, INCLUDE_RIGHT);
    TypeRange r2(10, 20);
    ASSERT_FALSE(r1.includes(r2));
}

TEST(RangeOps, Sin) {
    TypeRange r1;
    TypeRange rout = RangeOps::sin(r1);
    ASSERT_EQ(rout, TypeRange(-1,1));
}

TEST(RangeOps, GetLeft) {
    TypeRange r1(0, 10);
    
    ASSERT_EQ(RangeOps::getLeftExcluding(r1, 5), TypeRange(0, 5, INCLUDE_LEFT));
    ASSERT_EQ(RangeOps::getLeftIncluding(r1, 5), TypeRange(0, 5, INCLUDE_BOTH));
}

TEST(RangeOps, GetLeft_FromExcluded) {
    TypeRange r1(0, 10, INCLUDE_RIGHT);
    
    ASSERT_EQ(RangeOps::getLeftExcluding(r1, 5), TypeRange(0, 5, EXCLUDE_BOTH));
    ASSERT_EQ(RangeOps::getLeftIncluding(r1, 5), TypeRange(0, 5, INCLUDE_RIGHT));
}

TEST(RangeOps, GetLeft_Outside) {
    TypeRange r1(0, 10);

    ASSERT_EQ(r1, RangeOps::getLeftExcluding(r1, 15));
    ASSERT_FALSE(RangeOps::getLeftExcluding(r1, -1).isValid());
}

TEST(RangeOps, GetRight) {
    TypeRange r1(0, 10);
    
    ASSERT_EQ(RangeOps::getRightExcluding(r1, 5), TypeRange(5, 10, INCLUDE_RIGHT));
    ASSERT_EQ(RangeOps::getRightIncluding(r1, 5), TypeRange(5, 10, INCLUDE_BOTH));
}

TEST(RangeOps, GetRight_FromExcluded) {
    TypeRange r1(0, 10, INCLUDE_LEFT);
    
    ASSERT_EQ(RangeOps::getRightExcluding(r1, 5), TypeRange(5, 10, EXCLUDE_BOTH));
    ASSERT_EQ(RangeOps::getRightIncluding(r1, 5), TypeRange(5, 10, INCLUDE_LEFT));
}

TEST(RangeOps, GetRight_Outside) {
    TypeRange r1(0, 10);

    ASSERT_EQ(r1, RangeOps::getRightExcluding(r1, -1));
    ASSERT_FALSE(RangeOps::getRightExcluding(r1, 15).isValid());
}

TEST(RangeOps, GetLeft_Edge) {
    TypeRange r1(0, 10);
    ASSERT_EQ(TypeRange(0), RangeOps::getLeftIncluding(r1, 0));
    ASSERT_FALSE(RangeOps::getLeftExcluding(r1, 0).isValid());
}

TEST(RangeOps, GetRight_Edge) {
    TypeRange r1(-10, 0);
    ASSERT_EQ(TypeRange(0), RangeOps::getRightIncluding(r1, 0));
    ASSERT_FALSE(RangeOps::getRightExcluding(r1, 0).isValid());
}

TEST(RangeOps, GetLeftRight_Consts) {
    TypeRange r1(0.7);
    ASSERT_EQ(TypeRange(0.7), RangeOps::getRightIncluding(r1, 0.7));
    ASSERT_EQ(TypeRange(0.7), RangeOps::getLeftIncluding(r1, 0.7));
    ASSERT_FALSE(RangeOps::getLeftExcluding(r1, 0.7).isValid());
    ASSERT_FALSE(RangeOps::getRightExcluding(r1, 0.7).isValid());
}

TEST(RangeOps, Min) {
    TypeRange r1(0, 20);
    TypeRange r2(5, 15);
    ASSERT_EQ(TypeRange(0, 15), RangeOps::min(r1, r2));
}

TEST(RangeOps, Min_Inf) {
    TypeRange r1(-INFINITY, 20);
    TypeRange r2(5, 15);
    ASSERT_EQ(TypeRange(-INFINITY, 15), RangeOps::min(r1, r2));
}

TEST(RangeOps, Clamp) {
    TypeRange r1(0, 20);
    ASSERT_EQ(TypeRange(5, 15), RangeOps::clamp(r1, TypeRange(5), TypeRange(15)));
}

TEST(RangeOps, Length_2) {
    TypeRange r1(1, 3);
    TypeRange r2(0, 4);
    TypeRange zero(0, 0);
    ASSERT_EQ(TypeRange(1, 5), RangeOps::length(r1, r2, zero, zero));
}

TEST(RangeOps, Length_2_NegativeRange) {
    TypeRange r1(-1, -3);
    TypeRange r2(-2, 4);
    TypeRange zero(0, 0);
    ASSERT_EQ(TypeRange(1, 5), RangeOps::length(r1, r2, zero, zero));
}

TEST(RangeOps, Floor) {
    TypeRange r1(1.3, 4.9);
    ASSERT_EQ(TypeRange(1, 4), RangeOps::floor(r1));
}

TEST(RangeOps, Fract) {
    TypeRange r1(1.3, 4.9);
    ASSERT_EQ(TypeRange(0, 1, INCLUDE_LEFT), RangeOps::fract(r1));
}

TEST(RangeOps, Mix_Const) {
    //mix([0,10], [20,40], 0.5)
    //0-20   10 - min
    //2-20   11
    //0-40   20
    //10-20  15
    //10-40  25 - max
    TypeRange r1(0, 10);
    TypeRange r2(20, 40);
    TypeRange mix(0.5);
    ASSERT_EQ(TypeRange(10, 25), RangeOps::mix(r1, r2, mix));
}

TEST(RangeOps, Mix_Consts) {
    TypeRange r1(10);
    TypeRange r2(20);
    TypeRange mix(0.2);
    ASSERT_EQ(TypeRange(12), RangeOps::mix(r1, r2, mix));
}

TEST(RangeOps, Mix_Consts_Reverse) {
    TypeRange r1(20);
    TypeRange r2(10);
    TypeRange mix(0.2);
    ASSERT_EQ(TypeRange(18), RangeOps::mix(r1, r2, mix));
}

TEST(RangeOps, Mix_Full) {
    TypeRange r1(0, 10);
    TypeRange r2(20, 40);
    TypeRange mix(0, 1);
    ASSERT_EQ(TypeRange(0, 40), RangeOps::mix(r1, r2, mix));
}

TEST(RangeOps, Mix_Full_Includes) {
    TypeRange r1(0, 10);
    TypeRange r2(10, 40);
    TypeRange mix(0, 1);
    ASSERT_EQ(TypeRange(0, 40), RangeOps::mix(r1, r2, mix));
}

TEST(RangeOps, Mix_Range) {
    TypeRange r1(0, 10);
    TypeRange r2(10, 40);
    ASSERT_EQ(TypeRange(0, 25), RangeOps::mix(r1, r2, TypeRange(0, 0.5)));
}

TEST(RangeOps, Power_RightIsConst) {
    TypeRange r1(2, 3);
    TypeRange pwr(3);

    ASSERT_EQ(TypeRange(8, 27), RangeOps::power(r1, pwr));
}

TEST(RangeOps, Power_RightIsRange) {
    TypeRange r1(2, 3);
    TypeRange pwr(2,3);

    ASSERT_EQ(TypeRange(4, 27), RangeOps::power(r1, pwr));
}

TEST(RangeOps, Power_Around1) {
    TypeRange r1(0.1, 10);
    TypeRange pwr(2, 3);
    TypeRange actual = RangeOps::power(r1, pwr);
    TypeRange expected = TypeRange(0.001, 1000);
    ASSERT_EQ(expected, actual);
}


TEST(Range, Boolean) {
    ASSERT_TRUE((bool)(TypeRange(1)));
    ASSERT_FALSE((bool)(TypeRange(0)));
    ASSERT_TRUE((bool)(TypeRange(0,1)));
}

//todo: test for minus(?)