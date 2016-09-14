#ifndef MANTID_INDEXING_GROUPTEST_H_
#define MANTID_INDEXING_GROUPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/Group.h"
#include "MantidIndexing/IndexInfo.h"

using namespace Mantid;
using namespace Indexing;

class GroupTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupTest *createSuite() { return new GroupTest(); }
  static void destroySuite(GroupTest *suite) { delete suite; }

  void test_size_mismatch_fail() {
    IndexInfo source({1, 2, 3}, {{10}, {20}, {30}});
    std::vector<std::vector<size_t>> grouping{{0}, {1}, {2}};
    std::vector<specnum_t> specNums{4, 5};
    TS_ASSERT_THROWS(group(source, std::move(specNums), grouping),
                     std::runtime_error);
    TS_ASSERT_EQUALS(specNums.size(), 2);
  }

  void test_no_grouping() {
    IndexInfo source({1, 2, 3}, {{10}, {20}, {30}});
    std::vector<std::vector<size_t>> grouping{{0}, {1}, {2}};
    auto result = group(source, {4, 5, 6}, grouping);
    TS_ASSERT_EQUALS(result.size(), 3);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 4);
    TS_ASSERT_EQUALS(result.spectrumNumber(1), 5);
    TS_ASSERT_EQUALS(result.spectrumNumber(2), 6);
    TS_ASSERT_EQUALS(result.detectorIDs(0), std::vector<detid_t>{10});
    TS_ASSERT_EQUALS(result.detectorIDs(1), std::vector<detid_t>{20});
    TS_ASSERT_EQUALS(result.detectorIDs(2), std::vector<detid_t>{30});
  }

  void test_swap_ids() {
    IndexInfo source({1, 2, 3}, {{10}, {20}, {30}});
    std::vector<std::vector<size_t>> grouping{{1}, {0}, {2}};
    auto result = group(source, {1, 2, 3}, grouping);
    TS_ASSERT_EQUALS(result.size(), 3);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(result.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(result.spectrumNumber(2), 3);
    TS_ASSERT_EQUALS(result.detectorIDs(0), std::vector<detid_t>{20});
    TS_ASSERT_EQUALS(result.detectorIDs(1), std::vector<detid_t>{10});
    TS_ASSERT_EQUALS(result.detectorIDs(2), std::vector<detid_t>{30});
  }

  void test_extract() {
    IndexInfo source({1, 2, 3}, {{10}, {20}, {30}});
    std::vector<std::vector<size_t>> grouping{{1}};
    auto result = group(source, {1}, grouping);
    TS_ASSERT_EQUALS(result.size(), 1);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(result.detectorIDs(0), std::vector<detid_t>{20});
  }

  void test_group() {
    IndexInfo source({1, 2, 3}, {{10}, {20}, {30}});
    std::vector<std::vector<size_t>> grouping{{0, 2}, {1}};
    auto result = group(source, {1, 2}, grouping);
    TS_ASSERT_EQUALS(result.size(), 2);
    TS_ASSERT_EQUALS(result.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(result.spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(result.detectorIDs(0), std::vector<detid_t>({10, 30}));
    TS_ASSERT_EQUALS(result.detectorIDs(1), std::vector<detid_t>{20});
  }
};

#endif /* MANTID_INDEXING_GROUPTEST_H_ */
