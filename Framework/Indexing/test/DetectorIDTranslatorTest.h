#ifndef MANTID_INDEXING_DETECTORIDTRANSLATORTEST_H_
#define MANTID_INDEXING_DETECTORIDTRANSLATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/DetectorIDTranslator.h"
#include "MantidIndexing/RoundRobinPartitioning.h"

using namespace Mantid::Indexing;

class DetectorIDTranslatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorIDTranslatorTest *createSuite() {
    return new DetectorIDTranslatorTest();
  }
  static void destroySuite(DetectorIDTranslatorTest *suite) { delete suite; }

  DetectorIDTranslator makeTranslator(int ranks, int rank) {
    std::vector<std::pair<SpectrumNumber, std::vector<DetectorID>>> spectra;
    spectra.emplace_back(SpectrumNumber{2}, makeDetectorIDs({0}));
    spectra.emplace_back(SpectrumNumber{1}, makeDetectorIDs({2}));
    spectra.emplace_back(SpectrumNumber{4}, makeDetectorIDs({4, 6}));
    spectra.emplace_back(SpectrumNumber{5}, makeDetectorIDs({8}));
    return {spectra, RoundRobinPartitioning(
                         ranks, PartitionIndex(0),
                         Partitioning::MonitorStrategy::CloneOnEachPartition,
                         std::vector<SpectrumNumber>{}),
            PartitionIndex(rank)};
  }

  std::vector<DetectorID> makeDetectorIDs(std::initializer_list<int64_t> init) {
    return std::vector<DetectorID>(init.begin(), init.end());
  }

  template <class... T> std::vector<DetectorID> makeDetectorIDs(T &&... args) {
    return std::vector<DetectorID>(std::forward<T>(args)...);
  }

  void test_construct() {
    auto detectorIDs = makeDetectorIDs({0});
    std::vector<std::pair<SpectrumNumber, std::vector<DetectorID>>> spectra;
    spectra.emplace_back(SpectrumNumber{1}, detectorIDs);
    TS_ASSERT_THROWS_NOTHING(DetectorIDTranslator(
        spectra, RoundRobinPartitioning(
                     1, PartitionIndex(0),
                     Partitioning::MonitorStrategy::CloneOnEachPartition,
                     std::vector<SpectrumNumber>{}),
        PartitionIndex(0)));
  }

  void test_makeIndexSet_full_1_rank() {
    auto translator = makeTranslator(1, 0);
    auto set = translator.makeIndexSet();
    TS_ASSERT_EQUALS(set.size(), 5);
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 1);
    TS_ASSERT_EQUALS(set[2], 2);
    TS_ASSERT_EQUALS(set[3], 3);
  }

  void test_makeIndexSet_full_3_ranks() {
    auto translator = makeTranslator(3, 1);
    auto set = translator.makeIndexSet();
    // spectrumNumbers 1,2,4,5:
    // 1 % 3 = 1, 4 % 3 = 1
    // detector IDs are thus 2,4,6
    TS_ASSERT_EQUALS(set.size(), 3);
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 1);
    TS_ASSERT_EQUALS(set[2], 2);
  }

  void test_makeIndexSet_partial_1_rank() {
    auto translator = makeTranslator(1, 0);
    auto set1 = translator.makeIndexSet(makeDetectorIDs({0, 2}));
    TS_ASSERT_EQUALS(set1.size(), 2);
    TS_ASSERT_EQUALS(set1[0], 0);
    TS_ASSERT_EQUALS(set1[1], 1);
    auto set2 = translator.makeIndexSet(makeDetectorIDs({4, 6, 8}));
    TS_ASSERT_EQUALS(set2.size(), 3);
    TS_ASSERT_EQUALS(set2[0], 2);
    TS_ASSERT_EQUALS(set2[1], 3);
    TS_ASSERT_EQUALS(set2[2], 4);
  }

  void test_makeIndexSet_partial_3_ranks_range_checks() {
    auto translator = makeTranslator(3, 1);
    TS_ASSERT_THROWS(translator.makeIndexSet(makeDetectorIDs({1})),
                     std::out_of_range);
    // Index is not on this rank but it is correct.
    TS_ASSERT_THROWS_NOTHING(translator.makeIndexSet(makeDetectorIDs({0})));
  }

  void test_makeIndexSet_partial_3_ranks() {
    auto translator = makeTranslator(3, 1);
    // 2 is on this rank
    auto set1 = translator.makeIndexSet(makeDetectorIDs({0, 2}));
    TS_ASSERT_EQUALS(set1.size(), 1);
    TS_ASSERT_EQUALS(set1[0], 0);
    // 4 and 6 are on this rank
    auto set2 = translator.makeIndexSet(makeDetectorIDs({4, 6, 8}));
    TS_ASSERT_EQUALS(set2.size(), 2);
    TS_ASSERT_EQUALS(set2[0], 1);
    TS_ASSERT_EQUALS(set2[1], 2);
  }
};

#endif /* MANTID_INDEXING_DETECTORIDTRANSLATORTEST_H_ */
