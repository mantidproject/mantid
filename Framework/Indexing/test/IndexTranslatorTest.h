#ifndef MANTID_INDEXING_INDEXTRANSLATORTEST_H_
#define MANTID_INDEXING_INDEXTRANSLATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/IndexTranslator.h"

using namespace Mantid;
using namespace Indexing;

class IndexTranslatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IndexTranslatorTest *createSuite() {
    return new IndexTranslatorTest();
  }
  static void destroySuite(IndexTranslatorTest *suite) { delete suite; }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(
        IndexTranslator(SpectrumNumbers{1, 2, 3}, DetectorIDs{0, 1, 2}));
    TS_ASSERT_THROWS_NOTHING(IndexTranslator(SpectrumNumbers{1, 2, 3}));
  }

  void test_constructor_size_mismatch() {
    TS_ASSERT_THROWS(
        IndexTranslator(SpectrumNumbers{1, 2, 3}, DetectorIDs{0, 1}),
        std::runtime_error);
  }

  void test_size() {
    TS_ASSERT_EQUALS(
        IndexTranslator(SpectrumNumbers{1, 2, 3}, DetectorIDs{0, 1, 2}).size(),
        3);
  }

  void test_spectrumNumbers() {
    SpectrumNumbers spectrumNumbers{1, 2, 3};
    auto ptr = spectrumNumbers.data().data();
    IndexTranslator t(std::move(spectrumNumbers), DetectorIDs{0, 1, 2});
    auto &nums = t.spectrumNumbers();
    TS_ASSERT_EQUALS(nums, (std::vector<specnum_t>{1, 2, 3}));
    // Was data moved?
    TS_ASSERT_EQUALS(nums.data(), ptr);
  }

  void test_detectorIDs() {
    std::vector<std::vector<detid_t>> detectorIDs{{1}, {2, 1, 2}, {4, 3}};
    IndexTranslator t(SpectrumNumbers{1, 2, 3},
                      DetectorIDs(std::move(detectorIDs)));
    TS_ASSERT_EQUALS(t.detectorIDs(),
                     (std::vector<std::vector<detid_t>>{{1}, {1, 2}, {3, 4}}));
  }

  void test_detectorIDs_moved_if_sorted_and_unique() {
    std::vector<std::vector<detid_t>> detectorIDs{{1}, {1, 2}, {3, 4}};
    auto ptr = detectorIDs.data();
    IndexTranslator t(SpectrumNumbers{1, 2, 3},
                      DetectorIDs(std::move(detectorIDs)));
    TS_ASSERT_EQUALS(t.detectorIDs().data(), ptr);
  }
};

#endif /* MANTID_INDEXING_INDEXTRANSLATORTEST_H_ */
