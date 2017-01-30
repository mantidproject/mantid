#ifndef MANTID_INDEXING_INDEXINFOTEST_H_
#define MANTID_INDEXING_INDEXINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/make_cow.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid;
using namespace Indexing;

class IndexInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IndexInfoTest *createSuite() { return new IndexInfoTest(); }
  static void destroySuite(IndexInfoTest *suite) { delete suite; }

  void test_size_constructor() { TS_ASSERT_THROWS_NOTHING(IndexInfo(3)); }

  void test_size_constructor_sets_correct_indices() {
    IndexInfo info(3);
    TS_ASSERT_EQUALS(info.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(info.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(info.spectrumNumber(2), 3);
    TS_ASSERT(info.detectorIDs(0).empty());
    TS_ASSERT(info.detectorIDs(1).empty());
    TS_ASSERT(info.detectorIDs(2).empty());
  }

  void test_vector_constructor() {
    TS_ASSERT_THROWS_NOTHING(IndexInfo({3, 2, 1}, {{}, {10}, {20, 30}}));
  }

  void test_vector_constructor_sets_correct_indices() {
    IndexInfo info({3, 2, 1}, {{}, {10}, {20, 30}});
    TS_ASSERT_EQUALS(info.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(info.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(info.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(info.detectorIDs(0), (std::vector<DetectorID>{}));
    TS_ASSERT_EQUALS(info.detectorIDs(1), (std::vector<DetectorID>{10}));
    TS_ASSERT_EQUALS(info.detectorIDs(2), (std::vector<DetectorID>{20, 30}));
  }

  void test_vector_constructor_size_mismatch() {
    TS_ASSERT_THROWS(IndexInfo({3, 2, 1}, {{}, {10}}), std::runtime_error);
  }

  void test_size() { TS_ASSERT_EQUALS(IndexInfo(3).size(), 3); }

  void test_copy() {
    const IndexInfo info({3, 2, 1}, {{}, {10}, {20, 30}});
    auto copy(info);
    TS_ASSERT_EQUALS(info.size(), 3);
    TS_ASSERT_EQUALS(copy.size(), 3);
    TS_ASSERT_EQUALS(copy.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(copy.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(copy.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(copy.detectorIDs(0), (std::vector<DetectorID>{}));
    TS_ASSERT_EQUALS(copy.detectorIDs(1), (std::vector<DetectorID>{10}));
    TS_ASSERT_EQUALS(copy.detectorIDs(2), (std::vector<DetectorID>{20, 30}));
  }

  void test_move() {
    IndexInfo info({3, 2, 1}, {{}, {10}, {20, 30}});
    auto moved(std::move(info));
    TS_ASSERT_EQUALS(info.size(), 0);
    TS_ASSERT_EQUALS(moved.size(), 3);
    TS_ASSERT_EQUALS(moved.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(moved.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(moved.spectrumNumber(2), 1);
    TS_ASSERT_EQUALS(moved.detectorIDs(0), (std::vector<DetectorID>{}));
    TS_ASSERT_EQUALS(moved.detectorIDs(1), (std::vector<DetectorID>{10}));
    TS_ASSERT_EQUALS(moved.detectorIDs(2), (std::vector<DetectorID>{20, 30}));
  }

  void test_setSpectrumNumbers_size_mismatch() {
    IndexInfo t(3);
    TS_ASSERT_THROWS(t.setSpectrumNumbers({1, 2}), std::runtime_error);
  }

  void test_setDetectorIDs_size_mismatch() {
    IndexInfo t(3);
    TS_ASSERT_THROWS(t.setDetectorIDs({1, 2}), std::runtime_error);
  }

  void test_setSpectrumNumbers() {
    IndexInfo t(3);
    TS_ASSERT_THROWS_NOTHING(t.setSpectrumNumbers({3, 4, 5}));
    TS_ASSERT_EQUALS(t.spectrumNumber(0), 3);
    TS_ASSERT_EQUALS(t.spectrumNumber(1), 4);
    TS_ASSERT_EQUALS(t.spectrumNumber(2), 5);
  }

  void test_setDetectorIDs() {
    IndexInfo t(3);
    TS_ASSERT_THROWS_NOTHING(t.setDetectorIDs({6, 7, 8}));
    TS_ASSERT_EQUALS(t.detectorIDs(0), std::vector<DetectorID>{6});
    TS_ASSERT_EQUALS(t.detectorIDs(1), std::vector<DetectorID>{7});
    TS_ASSERT_EQUALS(t.detectorIDs(2), std::vector<DetectorID>{8});
  }

  void test_setSpectrumDefinitions_setting_nullptr_fails() {
    // This might be supported in the future but is not needed now and might
    // break some things, so we forbid this for now.
    IndexInfo info(3);
    Kernel::cow_ptr<std::vector<SpectrumDefinition>> defs{nullptr};
    TS_ASSERT_THROWS(info.setSpectrumDefinitions(defs), std::runtime_error);
  }

  void test_setSpectrumDefinitions_size_mismatch() {
    IndexInfo info(3);
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(2);
    TS_ASSERT_THROWS(info.setSpectrumDefinitions(defs), std::runtime_error);
  }

  void test_setSpectrumDefinitions() {
    IndexInfo info(3);
    const auto defs = Kernel::make_cow<std::vector<SpectrumDefinition>>(3);
    TS_ASSERT_THROWS_NOTHING(info.setSpectrumDefinitions(defs));
    TS_ASSERT_EQUALS(info.spectrumDefinitions().get(), defs.get());
  }
};

#endif /* MANTID_INDEXING_INDEXINFOTEST_H_ */
