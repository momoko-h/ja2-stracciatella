#include <math.h>
#include "Random.h"
#include "gtest/gtest.h"

static const int32_t TESTITERATIONS = 100 * 1000;

TEST(Random, RandomInExpectedRange)
{
  // This doesn't get called when ja2 is started with -unittests
  InitializeRandom();

  int32_t outOfBoundsCount = 0;
  for (int32_t i = 0; i < TESTITERATIONS; ++i)
    if (Random(50) >= 50) ++outOfBoundsCount;

  EXPECT_EQ(outOfBoundsCount, 0);
}

TEST(Random, Random50PercentOdd)
{
  int32_t oddCount = 0;
  for (int32_t i = 0; i < TESTITERATIONS; ++i)
    if (Random(10000) % 2 == 1) ++oddCount;

  int32_t expected = TESTITERATIONS / 2;
  int32_t deviation = abs(expected - oddCount);
  int32_t _2Std = 2 * sqrt(TESTITERATIONS / 4);
  EXPECT_LE(deviation, _2Std);
}

TEST(Random, ZeroChanceAlwaysFalse)
{
  int32_t zeroChanceIsTrue = 0;
  for (int32_t i = 0; i < TESTITERATIONS; ++i)
    if (Chance(0)) ++zeroChanceIsTrue;

  EXPECT_EQ(zeroChanceIsTrue, 0);
}

TEST(Random, OneChanceNotAlwaysFalse)
{
  // Check for off-by-one errors, IOW make sure Chance(1)
  // isn't the same as Chance(0).
  int32_t OneChanceIsTrue = 0;
  for (int32_t i = 0; i < 10000; ++i)
    if (Chance(1)) ++OneChanceIsTrue;

  EXPECT_GT(OneChanceIsTrue, 0);
}

TEST(Random, _100ChanceAlwaysTrue)
{
  int32_t _100ChanceIsFalse = 0;
  for (int32_t i = 0; i < TESTITERATIONS; ++i)
    if (!Chance(100)) ++_100ChanceIsFalse;

  EXPECT_EQ(_100ChanceIsFalse, 0);
}

TEST(Random, _50ChanceWithin2StdDev)
{
  int32_t trueCount = 0;
  for (int32_t i = 0; i < TESTITERATIONS; ++i)
    if (Chance(50)) ++trueCount;

  int32_t expected = TESTITERATIONS / 2;
  int32_t deviation = abs(expected - trueCount);
  int32_t _2Std = 2 * sqrt(TESTITERATIONS / 4);
  EXPECT_LE(deviation, _2Std);
}

TEST(Random, PreRandomInExpectedRange)
{
  int32_t outOfBoundsCount = 0;
  for (int32_t i = 0; i < TESTITERATIONS; ++i)
    if (PreRandom(50) >= 50) ++outOfBoundsCount;

  EXPECT_EQ(outOfBoundsCount, 0);
}

TEST(Random, PreRandom50PercentOdd)
{
  int32_t oddCount = 0;
  for (int32_t i = 0; i < TESTITERATIONS; ++i)
    if (PreRandom(10000) % 2 == 1) ++oddCount;

  int32_t expected = TESTITERATIONS / 2;
  int32_t deviation = abs(expected - oddCount);
  int32_t _2Std = 2 * sqrt(TESTITERATIONS / 4);
  EXPECT_LE(deviation, _2Std);
}

TEST(Random, ZeroPreChanceAlwaysFalse)
{
  int32_t zeroPreChanceIsTrue = 0;
  for (int32_t i = 0; i < TESTITERATIONS; ++i)
    if (PreChance(0)) ++zeroPreChanceIsTrue;

  EXPECT_EQ(zeroPreChanceIsTrue, 0);
}

TEST(Random, OnePreChanceNotAlwaysFalse)
{
  int32_t OnePreChanceIsTrue = 0;
  for (int32_t i = 0; i < 10000; ++i)
    if (Chance(1)) ++OnePreChanceIsTrue;

  EXPECT_GT(OnePreChanceIsTrue, 0);
}

TEST(Random, _100PreChanceAlwaysTrue)
{
  int32_t _100PreChanceIsFalse = 0;
  for (int32_t i = 0; i < TESTITERATIONS; ++i)
    if (!PreChance(100)) ++_100PreChanceIsFalse;

  EXPECT_EQ(_100PreChanceIsFalse, 0);
}

TEST(Random, _50PreChanceWithin2StdDev)
{
  int32_t trueCount = 0;
  for (int32_t i = 0; i < TESTITERATIONS; ++i)
    if (PreChance(50)) ++trueCount;

  int32_t expected = TESTITERATIONS / 2;
  int32_t deviation = abs(expected - trueCount);
  int32_t _2Std = 2 * sqrt(TESTITERATIONS / 4);
  EXPECT_LE(deviation, _2Std);
}
