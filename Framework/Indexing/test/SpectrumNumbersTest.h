#ifndef MANTID_INDEXING_SPECTRUMNUMBERSTEST_H_
#define MANTID_INDEXING_SPECTRUMNUMBERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/SpectrumNumbers.h"

using namespace Mantid;
using Indexing::SpectrumNumbers;

class SpectrumNumbersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumNumbersTest *createSuite() {
    return new SpectrumNumbersTest();
  }
  static void destroySuite(SpectrumNumbersTest *suite) { delete suite; }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING((SpectrumNumbers{1, 2, 3}));
  }

  void test_constructor_not_strictly_ascending() {
    TS_ASSERT_THROWS(SpectrumNumbers({1, 2, 1}), std::runtime_error);
    TS_ASSERT_THROWS(SpectrumNumbers({1, 2, 2}), std::runtime_error);
  }

  void test_size() { TS_ASSERT_EQUALS(SpectrumNumbers({1, 2, 3}).size(), 3); }

  void test_spectrumNumbers() {
    std::vector<specnum_t> spectrumNumbers{1, 2, 3};
    auto ptr = spectrumNumbers.data();
    SpectrumNumbers testee(std::move(spectrumNumbers));
    auto &nums = testee.data();
    TS_ASSERT_EQUALS(nums, (std::vector<specnum_t>{1, 2, 3}));
    // Was data moved?
    TS_ASSERT_EQUALS(nums.data(), ptr);
  }
};

#endif /* MANTID_INDEXING_SPECTRUMNUMBERSTEST_H_ */
