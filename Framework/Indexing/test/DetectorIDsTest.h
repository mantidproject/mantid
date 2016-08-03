#ifndef MANTID_INDEXING_DETECTORIDSTEST_H_
#define MANTID_INDEXING_DETECTORIDSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/DetectorIDs.h"

using namespace Mantid;
using Indexing::DetectorIDs;

class DetectorIDsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorIDsTest *createSuite() { return new DetectorIDsTest(); }
  static void destroySuite(DetectorIDsTest *suite) { delete suite; }

  void test_constructor() { TS_ASSERT_THROWS_NOTHING((DetectorIDs{1, 2, 3})); }

  void test_size() { TS_ASSERT_EQUALS(DetectorIDs({1, 2, 3}).size(), 3); }

  void test_detectorIDs() {
    std::vector<std::vector<detid_t>> detectorIDs{{1}, {2, 1, 2}, {4, 3}};
    DetectorIDs testee(std::move(detectorIDs));
    TS_ASSERT_EQUALS(testee.data(),
                     (std::vector<std::vector<detid_t>>{{1}, {1, 2}, {3, 4}}));
  }

  void test_detectorIDs_moved_if_sorted_and_unique() {
    std::vector<std::vector<detid_t>> detectorIDs{{1}, {1, 2}, {3, 4}};
    auto ptr = detectorIDs.data();
    DetectorIDs testee(std::move(detectorIDs));
    TS_ASSERT_EQUALS(testee.data().data(), ptr);
  }
};

#endif /* MANTID_INDEXING_DETECTORIDSTEST_H_ */
