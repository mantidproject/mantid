#ifndef SOBOLSEQUENCETEST_H_
#define SOBOLSEQUENCETEST_H_

#include "MantidKernel/SobolSequence.h"
#include <cxxtest/TestSuite.h>

#include <boost/algorithm/string/predicate.hpp>

using Mantid::Kernel::SobolSequence;

class SobolSequenceTest : public CxxTest::TestSuite {

public:
  void test_That_Object_Construction_Does_Not_Throw() {
    TS_ASSERT_THROWS_NOTHING(SobolSequence(1));
  }

  void test_That_Next_For_Two_Generators_Returns_Same_Value() {
    SobolSequence gen_1(3), gen_2(3);
    const std::vector<double> seq1 = gen_1.nextPoint();
    const std::vector<double> seq2 = gen_2.nextPoint();

    TS_ASSERT(boost::algorithm::equals(seq1, seq2));
  }

  void test_That_A_Given_Number_Of_Dimensions_Produces_Expected_Sequence() {
    SobolSequence randGen(5);

    assert_Sequence_Is_As_Expected_For_Five_Dimensions(randGen);
  }

  void test_That_Restart_Produces_The_Sequence_From_The_Beginning() {
    SobolSequence randGen(5);
    doNextValueCalls(20, randGen);
    randGen.restart();

    assert_Sequence_Is_As_Expected_For_Five_Dimensions(randGen);
  }

  void test_Save_Call_Restore_Gives_Sequence_From_Saved_Point() {
    SobolSequence randGen(5);
    doNextValueCalls(25, randGen); // Move from start to test it doesn't just go
                                   // back to beginning
    randGen.save();
    const size_t ncheck(20);
    auto firstValues = doNextValueCalls(ncheck, randGen);
    randGen.restore();
    auto secondValues = doNextValueCalls(ncheck, randGen);

    for (size_t i = 0; i < ncheck; ++i) {
      TS_ASSERT(boost::algorithm::equals(firstValues[i], secondValues[i]));
    }
  }

  void
  test_Save_Call_Restore_Call_Then_Restore_Gives_Sequence_From_Saved_Point() {
    SobolSequence randGen(5);
    doNextValueCalls(25, randGen); // Move from start to test it doesn't just go
                                   // back to beginning
    randGen.save();
    const size_t ncheck(20);
    auto firstValues = doNextValueCalls(ncheck, randGen);
    randGen.restore();
    doNextValueCalls(ncheck, randGen);
    randGen.restore();
    auto thirdValues = doNextValueCalls(ncheck, randGen);

    for (size_t i = 0; i < ncheck; ++i) {
      TS_ASSERT(boost::algorithm::equals(firstValues[i], thirdValues[i]));
    }
  }

private:
  std::vector<std::vector<double>> doNextValueCalls(const unsigned int ncalls,
                                                    SobolSequence &randGen) {
    std::vector<std::vector<double>> values(ncalls);
    for (unsigned int i = 0; i < ncalls; ++i) {
      values[i] = randGen.nextPoint();
    }
    return values;
  }

  void
  assert_Sequence_Is_As_Expected_For_Five_Dimensions(SobolSequence &randGen) {
    double expectedValues[3][5] = {
        {0.5, 0.5, 0.5, 0.5, 0.5},
        {0.75, 0.25, 0.75, 0.25, 0.75},
        {0.25, 0.75, 0.25, 0.75, 0.25},
    };
    for (auto &expectedValue : expectedValues) {
      const std::vector<double> randPoint = randGen.nextPoint();
      for (std::size_t j = 0; j < 5; ++j) {
        TS_ASSERT_DELTA(randPoint[j], expectedValue[j], 1e-12);
      }
    }
  }
};

class SobolSequenceTestPerformance : public CxxTest::TestSuite {
public:
  void test_Large_Number_Of_Next_Point_Calls() {
    const unsigned int ndimensions(14);
    SobolSequence generator(ndimensions);
    const size_t ncalls = 10000000;
    size_t sumSizes(0); // Make sure the optimizer actuall does the loop
    for (size_t i = 0; i < ncalls; ++i) {
      const std::vector<double> &point = generator.nextPoint();
      sumSizes += point.size();
    }
    TS_ASSERT(sumSizes > 0);
  }
};

#endif
