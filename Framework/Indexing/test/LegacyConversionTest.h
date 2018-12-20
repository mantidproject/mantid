#ifndef MANTID_INDEXING_LEGACYCONVERSIONTEST_H_
#define MANTID_INDEXING_LEGACYCONVERSIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/LegacyConversion.h"

using namespace Mantid::Indexing;

class LegacyConversionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LegacyConversionTest *createSuite() {
    return new LegacyConversionTest();
  }
  static void destroySuite(LegacyConversionTest *suite) { delete suite; }

  void test_makeSpectrumNumberVector_empty() {
    std::vector<int32_t> data{};
    TS_ASSERT_EQUALS(makeSpectrumNumberVector(data).size(), 0);
  }
  void test_makeSpectrumNumberVector() {
    std::vector<int32_t> data{1, 2, 4};
    const auto specNums = makeSpectrumNumberVector(data);
    TS_ASSERT_EQUALS(specNums.size(), 3);
    TS_ASSERT_EQUALS(specNums[0], 1);
    TS_ASSERT_EQUALS(specNums[1], 2);
    TS_ASSERT_EQUALS(specNums[2], 4);
  }
};

#endif /* MANTID_INDEXING_LEGACYCONVERSIONTEST_H_ */
