#ifndef MERSENNETWISTERTEST_H_
#define MERSENNETWISTERTEST_H_

#include "MantidKernel/MersenneTwister.h"
#include <cxxtest/TestSuite.h>

#include <iomanip>

using Mantid::Kernel::MersenneTwister;

class MersenneTwisterTest : public CxxTest::TestSuite {

public:
  void test_That_Object_Construction_Does_Not_Throw() {
    TS_ASSERT_THROWS_NOTHING(MersenneTwister(1));
  }

  void test_That_Next_For_Given_Seed_Returns_Same_Value() {
    size_t seed(212437999);
    MersenneTwister gen_1(seed), gen_2(seed);

    TS_ASSERT_EQUALS(gen_1.nextValue(), gen_2.nextValue());
  }

  void test_That_Next_For_Different_Seeds_Returns_Different_Values() {
    long seed_1(212437999), seed_2(247021340);
    MersenneTwister gen_1(seed_1), gen_2(seed_2);

    TS_ASSERT_DIFFERS(gen_1.nextValue(), gen_2.nextValue());
  }

  void test_That_A_Given_Seed_Produces_Expected_Sequence() {
    MersenneTwister randGen(1);
    randGen.setSeed(39857239);
    assertSequenceCorrectForSeed_39857239(randGen);
  }

  void test_That_A_Restart_Gives_Same_Sequence_Again_From_Start() {
    MersenneTwister randGen(1);
    randGen.setSeed(39857239);
    assertSequenceCorrectForSeed_39857239(randGen);
    randGen.restart();
    assertSequenceCorrectForSeed_39857239(randGen);
  }

  void test_That_A_Restore_Without_Save_Does_The_Same_As_Restart() {
    MersenneTwister randGen(39857239);
    assertSequenceCorrectForSeed_39857239(randGen);
    randGen.restore();
    assertSequenceCorrectForSeed_39857239(randGen);
  }

  void
  test_That_Save_Then_Call_Next_Value_And_Restore_Gives_Sequence_From_Saved_Point() {
    MersenneTwister randGen(1);
    doNextValueCalls(10,
                     randGen); // Move away from start so not the same as reset

    const unsigned int ncheck(50);
    randGen.save();
    std::vector<double> firstValues = doNextValueCalls(50, randGen);
    randGen.restore();
    std::vector<double> secondValues = doNextValueCalls(50, randGen);

    for (unsigned int i = 0; i < ncheck; ++i) {
      TS_ASSERT_EQUALS(firstValues[i], secondValues[i]);
    }
  }

  void
  test_Second_Restore_Without_A_Save_In_Between_Takes_Generator_Back_To_Saved_Point() {
    MersenneTwister randGen(1);
    doNextValueCalls(10,
                     randGen); // Move away from start so not the same as reset

    const unsigned int ncheck(50);
    randGen.save();
    std::vector<double> firstValues = doNextValueCalls(50, randGen);
    randGen.restore();
    doNextValueCalls(50, randGen);
    randGen.restore();
    std::vector<double> thirdValues = doNextValueCalls(50, randGen);

    for (unsigned int i = 0; i < ncheck; ++i) {
      TS_ASSERT_EQUALS(firstValues[i], thirdValues[i]);
    }
  }

  void test_That_Default_Range_Produces_Numbers_Between_Zero_And_One() {
    MersenneTwister randGen(12345);
    // Test 20 numbers
    for (std::size_t i = 0; i < 20; ++i) {
      double r = randGen.nextValue();
      TS_ASSERT(r >= 0.0 && r <= 1.0);
    }
  }

  void test_That_A_Default_Range_Produces_Numbers_Within_This_Range() {
    long seed(15423894);
    const double start(2.5), end(5.);
    MersenneTwister randGen(seed, start, end);
    // Test 20 numbers
    for (std::size_t i = 0; i < 20; ++i) {
      const double r = randGen.nextValue();
      TS_ASSERT(r >= start && r <= end);
    }
  }

  void
  test_That_A_Given_Range_Produces_Numbers_Within_That_Range_For_Doubles() {
    long seed(15423894);
    const double start(0.), end(1.);
    MersenneTwister randGen(seed, start, end);
    // Test 20 numbers
    for (std::size_t i = 0; i < 20; ++i) {
      const double localStart(2.5), localEnd(3.5);
      const double r = randGen.nextValue(localStart, localEnd);
      TS_ASSERT(r >= localStart && r <= localEnd);
    }
  }

  void test_That_A_Given_Range_Produces_Numbers_Within_That_Range_For_Ints() {
    long seed(15423894);
    const int start(1), end(6);
    MersenneTwister randGen(seed);
    // Test 20 numbers
    for (std::size_t i = 0; i < 20; ++i) {
      const int r = randGen.nextInt(start, end);
      TS_ASSERT(r >= start && r <= end);
    }
  }

  void test_That_nextPoint_returns_1_Value() {
    MersenneTwister randGen(12345);
    // Test 20 numbers
    for (std::size_t i = 0; i < 20; ++i) {
      const std::vector<double> point = randGen.nextPoint();
      TS_ASSERT_EQUALS(point.size(), 1);
    }
  }

private:
  void assertSequenceCorrectForSeed_39857239(MersenneTwister &randGen) {
    double expectedValues[10] = {0.597970068269, 0.923726578038, 0.46738053759,
                                 0.204503614938, 0.885743656775, 0.532315163407,
                                 0.849185494256, 0.294648804097, 0.435378050559,
                                 0.222489577528};
    // Check 10 numbers
    for (std::size_t i = 0; i < 10; ++i) {
      TS_ASSERT_DELTA(randGen.nextValue(), expectedValues[i], 1e-12);
    }
    std::cerr << "\n";
  }

  std::vector<double> doNextValueCalls(const unsigned int ncalls,
                                       MersenneTwister &randGen) {
    std::vector<double> values(ncalls, 0.0);
    for (unsigned int i = 0; i < ncalls; ++i) {
      values[i] = randGen.nextValue();
    }
    return values;
  }
};

#endif
