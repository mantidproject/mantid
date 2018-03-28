#ifndef NDPSEUDORANDOMNUMBERGENERATORTEST_H_
#define NDPSEUDORANDOMNUMBERGENERATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/NDPseudoRandomNumberGenerator.h"
#include "MantidKernel/MersenneTwister.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using Mantid::Kernel::NDRandomNumberGenerator;

class NDPseudoRandomNumberGeneratorTest : public CxxTest::TestSuite {
private:
  using NDGenerator_sptr = boost::shared_ptr<NDRandomNumberGenerator>;

public:
  void test_That_Next_Always_Returns_ND_Size_Array() {
    NDGenerator_sptr ndRand = createTestGenerator(12345);
    for (int i = 0; i < 20; ++i) {
      std::vector<double> point = ndRand->nextPoint();
      TS_ASSERT_EQUALS(static_cast<unsigned int>(point.size()), 3);
    }
  }

  void test_That_Restart_Is_Passed_On_Correctly() {
    NDGenerator_sptr ndRand = createTestGenerator(12345);
    std::vector<double> firstPoint = ndRand->nextPoint();
    TS_ASSERT_THROWS_NOTHING(ndRand->restart());
    std::vector<double> firstPointAfterReset = ndRand->nextPoint();
    for (size_t i = 0; i < firstPoint.size(); ++i) {
      TS_ASSERT_EQUALS(firstPoint[i], firstPointAfterReset[i]);
    }
  }

  void test_That_Range_Of_SingleValue_Generator_Is_Respected() {
    const double start(2.1), end(3.4);
    NDGenerator_sptr ndRand = createTestGenerator(12345, start, end);
    std::vector<double> firstPoint = ndRand->nextPoint();
    for (size_t i = 0; i < firstPoint.size(); ++i) {
      TS_ASSERT(firstPoint[i] >= start && firstPoint[i] <= end);
    }
  }

  void test_Save_Call_Restore_Gives_Sequence_From_Saved_Point() {
    NDGenerator_sptr ndRand = createTestGenerator(12345);
    doNextValueCalls(25, *ndRand); // Move from start to test it doesn't just go
                                   // back to beginning
    ndRand->save();
    const size_t ncheck(20);
    auto firstValues = doNextValueCalls(ncheck, *ndRand);
    ndRand->restore();
    auto secondValues = doNextValueCalls(ncheck, *ndRand);

    for (size_t i = 0; i < ncheck; ++i) {
      TS_ASSERT(boost::algorithm::equals(firstValues[i], secondValues[i]));
    }
  }

private:
  boost::shared_ptr<NDRandomNumberGenerator>
  createTestGenerator(const size_t seedValue, const double start = -1.0,
                      const double end = -1.0) {
    using namespace Mantid::Kernel;
    using NDMersenneTwister = NDPseudoRandomNumberGenerator<MersenneTwister>;
    const unsigned int ndims(3);
    if (start > 0.0 && end > 0.0)
      return boost::make_shared<NDMersenneTwister>(ndims, seedValue, start,
                                                   end);
    else
      return boost::make_shared<NDMersenneTwister>(ndims, seedValue);
  }

  std::vector<std::vector<double>>
  doNextValueCalls(const unsigned int ncalls,
                   NDRandomNumberGenerator &randGen) {
    std::vector<std::vector<double>> values(ncalls);
    for (unsigned int i = 0; i < ncalls; ++i) {
      values[i] = randGen.nextPoint();
    }
    return values;
  }
};

#endif // NDPSEUDORANDOMNUMBERGENERATORTEST_H_
