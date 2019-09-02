// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_SPECTRUMNUMBERTRANSLATORTEST_H_
#define MANTID_INDEXING_SPECTRUMNUMBERTRANSLATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/RoundRobinPartitioner.h"
#include "MantidIndexing/SpectrumNumberTranslator.h"

using namespace Mantid;
using namespace Indexing;

class SpectrumNumberTranslatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumNumberTranslatorTest *createSuite() {
    return new SpectrumNumberTranslatorTest();
  }
  static void destroySuite(SpectrumNumberTranslatorTest *suite) {
    delete suite;
  }

  std::unique_ptr<SpectrumNumberTranslator> makeTranslator(int ranks,
                                                           int rank) {
    // SpectrumNumber       2 1 4 5
    // GlobalSpectrumIndex  0 1 2 3
    // for 3 ranks:
    // Rank                 0 1 2 0
    // Local index          0 0 0 1
    auto numbers = {2, 1, 4, 5};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    return std::make_unique<SpectrumNumberTranslator>(
        spectrumNumbers,
        RoundRobinPartitioner(
            ranks, PartitionIndex(0),
            Partitioner::MonitorStrategy::CloneOnEachPartition,
            std::vector<GlobalSpectrumIndex>{}),
        PartitionIndex(rank));
  }

  std::vector<SpectrumNumber>
  makeSpectrumNumbers(std::initializer_list<int32_t> init) {
    return std::vector<SpectrumNumber>(init.begin(), init.end());
  }

  template <class... T>
  std::vector<SpectrumNumber> makeSpectrumNumbers(T &&... args) {
    return std::vector<SpectrumNumber>(std::forward<T>(args)...);
  }

  std::vector<GlobalSpectrumIndex>
  makeGlobalSpectrumIndices(std::initializer_list<int64_t> init) {
    return std::vector<GlobalSpectrumIndex>(init.begin(), init.end());
  }

  void test_construct() {
    auto numbers = {1, 2, 3, 4};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    TS_ASSERT_THROWS_NOTHING(SpectrumNumberTranslator(
        spectrumNumbers,
        RoundRobinPartitioner(
            1, PartitionIndex(0),
            Partitioner::MonitorStrategy::CloneOnEachPartition,
            std::vector<GlobalSpectrumIndex>{}),
        PartitionIndex(0)));
  }

  void test_construct_empty() {
    std::vector<SpectrumNumber> spectrumNumbers;
    TS_ASSERT_THROWS_NOTHING(SpectrumNumberTranslator(
        spectrumNumbers,
        RoundRobinPartitioner(
            1, PartitionIndex(0),
            Partitioner::MonitorStrategy::CloneOnEachPartition,
            std::vector<GlobalSpectrumIndex>{}),
        PartitionIndex(0)));
  }

  void test_construct_bad_spectrum_numbers() {
    auto numbers = {1, 2, 3, 3};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    // This works, but functionality is limited, see tests below.
    TS_ASSERT_THROWS_NOTHING(SpectrumNumberTranslator(
        spectrumNumbers,
        RoundRobinPartitioner(
            1, PartitionIndex(0),
            Partitioner::MonitorStrategy::CloneOnEachPartition,
            std::vector<GlobalSpectrumIndex>{}),
        PartitionIndex(0)));
  }

  void test_construct_parent() {
    auto numbers = {1, 2, 3, 4};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    SpectrumNumberTranslator parent(
        spectrumNumbers,
        RoundRobinPartitioner(
            1, PartitionIndex(0),
            Partitioner::MonitorStrategy::CloneOnEachPartition,
            std::vector<GlobalSpectrumIndex>{}),
        PartitionIndex(0));

    TS_ASSERT_THROWS_NOTHING(SpectrumNumberTranslator(spectrumNumbers, parent));
    spectrumNumbers.erase(spectrumNumbers.begin() + 1);
    TS_ASSERT_THROWS_NOTHING(SpectrumNumberTranslator(spectrumNumbers, parent));
    spectrumNumbers.erase(spectrumNumbers.begin());
    TS_ASSERT_THROWS_NOTHING(SpectrumNumberTranslator(spectrumNumbers, parent));
    spectrumNumbers.erase(spectrumNumbers.begin());
    TS_ASSERT_THROWS_NOTHING(SpectrumNumberTranslator(spectrumNumbers, parent));
    spectrumNumbers.erase(spectrumNumbers.begin());
    TS_ASSERT_THROWS_NOTHING(SpectrumNumberTranslator(spectrumNumbers, parent));
  }

  void test_construct_parent_reorder() {
    auto numbers = {1, 2, 3, 4};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    SpectrumNumberTranslator parent(
        spectrumNumbers,
        RoundRobinPartitioner(
            1, PartitionIndex(0),
            Partitioner::MonitorStrategy::CloneOnEachPartition,
            std::vector<GlobalSpectrumIndex>{}),
        PartitionIndex(0));

    std::iter_swap(spectrumNumbers.begin(), spectrumNumbers.end() - 1);
    SpectrumNumberTranslator reordered(spectrumNumbers, parent);
    TS_ASSERT_EQUALS(reordered.spectrumNumber(0), 4);
    TS_ASSERT_EQUALS(reordered.spectrumNumber(3), 1);
  }

  void test_construct_parent_bad_spectrum_numbers() {
    auto numbers = {1, 2, 3, 4};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    SpectrumNumberTranslator parent(
        spectrumNumbers,
        RoundRobinPartitioner(
            1, PartitionIndex(0),
            Partitioner::MonitorStrategy::CloneOnEachPartition,
            std::vector<GlobalSpectrumIndex>{}),
        PartitionIndex(0));

    spectrumNumbers[1] = 7; // 7 is not in parent.
    TS_ASSERT_THROWS(SpectrumNumberTranslator(spectrumNumbers, parent),
                     const std::out_of_range &);
  }

  void test_access_bad_spectrum_numbers() {
    auto numbers = {1, 2, 3, 3};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    SpectrumNumberTranslator translator(
        spectrumNumbers,
        RoundRobinPartitioner(
            1, PartitionIndex(0),
            Partitioner::MonitorStrategy::CloneOnEachPartition,
            std::vector<GlobalSpectrumIndex>{}),
        PartitionIndex(0));

    TS_ASSERT_THROWS_NOTHING(translator.spectrumNumber(0));
    // Accessing full set works, does not require spectrum numbers.
    TS_ASSERT_THROWS_NOTHING(translator.makeIndexSet());
    TS_ASSERT_EQUALS(translator.makeIndexSet().size(), 4);
    // Access via spectrum numbers fails.
    TS_ASSERT_THROWS(
        translator.makeIndexSet(SpectrumNumber(2), SpectrumNumber(3)),
        const std::logic_error &);
    TS_ASSERT_THROWS(translator.makeIndexSet(makeSpectrumNumbers({1})),
                     const std::logic_error &);
    // Access via global spectrum index works.
    TS_ASSERT_THROWS_NOTHING(translator.makeIndexSet(GlobalSpectrumIndex(1),
                                                     GlobalSpectrumIndex(2)));
    TS_ASSERT_THROWS_NOTHING(
        translator.makeIndexSet(makeGlobalSpectrumIndices({1})));
  }

  void test_spectrum_numbers_order_preserved() {
    auto numbers = {1, 0, 4, -1};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    SpectrumNumberTranslator translator(
        spectrumNumbers,
        RoundRobinPartitioner(
            1, PartitionIndex(0),
            Partitioner::MonitorStrategy::CloneOnEachPartition,
            std::vector<GlobalSpectrumIndex>{}),
        PartitionIndex(0));

    TS_ASSERT_EQUALS(translator.makeIndexSet(makeSpectrumNumbers({1}))[0], 0);
    TS_ASSERT_EQUALS(translator.makeIndexSet(makeSpectrumNumbers({0}))[0], 1);
    TS_ASSERT_EQUALS(translator.makeIndexSet(makeSpectrumNumbers({4}))[0], 2);
    TS_ASSERT_EQUALS(translator.makeIndexSet(makeSpectrumNumbers({-1}))[0], 3);
  }

  void test_globalSize() {
    TS_ASSERT_EQUALS(makeTranslator(1, 0)->globalSize(), 4);
    TS_ASSERT_EQUALS(makeTranslator(2, 0)->globalSize(), 4);
  }

  void test_localSize() {
    TS_ASSERT_EQUALS(makeTranslator(1, 0)->localSize(), 4);
    TS_ASSERT_EQUALS(makeTranslator(2, 0)->localSize(), 2);
  }

  void test_spectrumNumber() {
    auto numbers = {1, 0, 4, -1};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    SpectrumNumberTranslator translator(
        spectrumNumbers,
        RoundRobinPartitioner(
            1, PartitionIndex(0),
            Partitioner::MonitorStrategy::CloneOnEachPartition,
            std::vector<GlobalSpectrumIndex>{}),
        PartitionIndex(0));

    TS_ASSERT_EQUALS(translator.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(translator.spectrumNumber(1), 0);
    TS_ASSERT_EQUALS(translator.spectrumNumber(2), 4);
    TS_ASSERT_EQUALS(translator.spectrumNumber(3), -1);
  }

  void test_makeIndexSet_full_1_rank() {
    auto translator = makeTranslator(1, 0);
    auto set = translator->makeIndexSet();
    TS_ASSERT_EQUALS(set.size(), 4);
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 1);
    TS_ASSERT_EQUALS(set[2], 2);
    TS_ASSERT_EQUALS(set[3], 3);
  }

  void test_makeIndexSet_full_3_ranks() {
    auto set0 = makeTranslator(3, 0)->makeIndexSet();
    TS_ASSERT_EQUALS(set0.size(), 2);
    TS_ASSERT_EQUALS(set0[0], 0);
    TS_ASSERT_EQUALS(set0[1], 1);
    auto set1 = makeTranslator(3, 1)->makeIndexSet();
    TS_ASSERT_EQUALS(set1.size(), 1);
    TS_ASSERT_EQUALS(set1[0], 0);
    auto set2 = makeTranslator(3, 2)->makeIndexSet();
    TS_ASSERT_EQUALS(set2.size(), 1);
    TS_ASSERT_EQUALS(set2[0], 0);
  }

  void test_makeIndexSet_minmax_range_failures() {
    auto t = makeTranslator(1, 0);
    TS_ASSERT_THROWS(t->makeIndexSet(SpectrumNumber(0), SpectrumNumber(5)),
                     const std::out_of_range &);
    TS_ASSERT_THROWS(t->makeIndexSet(SpectrumNumber(1), SpectrumNumber(6)),
                     const std::out_of_range &);
    TS_ASSERT_THROWS(t->makeIndexSet(SpectrumNumber(1), SpectrumNumber(3)),
                     const std::out_of_range &);
  }

  void test_makeIndexSet_minmax_full_1_rank() {
    auto translator = makeTranslator(1, 0);
    auto set = translator->makeIndexSet(SpectrumNumber(1), SpectrumNumber(5));
    TS_ASSERT_EQUALS(set.size(), 4);
    // IndexSet is ordered by spectrum number.
    TS_ASSERT_EQUALS(set[0], 1);
    TS_ASSERT_EQUALS(set[1], 0);
    TS_ASSERT_EQUALS(set[2], 2);
    TS_ASSERT_EQUALS(set[3], 3);
  }

  void test_makeIndexSet_minmax_partial_1_rank() {
    auto translator = makeTranslator(1, 0);
    auto set = translator->makeIndexSet(SpectrumNumber(2), SpectrumNumber(4));
    TS_ASSERT_EQUALS(set.size(), 2);
    // Spectrum numbers are not ordered so there is a gap in the indices.
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 2);
  }

  void test_makeIndexSet_minmax_full_3_ranks() {
    auto translator = makeTranslator(3, 0);
    auto set = translator->makeIndexSet(SpectrumNumber(1), SpectrumNumber(5));
    TS_ASSERT_EQUALS(set.size(), 2);
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 1);
  }

  void test_makeIndexSet_minmax_partial_3_ranks() {
    auto translator = makeTranslator(3, 0);
    auto set = translator->makeIndexSet(SpectrumNumber(4), SpectrumNumber(5));
    TS_ASSERT_EQUALS(set.size(), 1);
    TS_ASSERT_EQUALS(set[0], 1);
  }

  void test_makeIndexSet_minmax_3_ranks_no_overlap() {
    // Rank 0 has spectrum numbers 2 and 5
    auto t0 = makeTranslator(3, 0);
    TS_ASSERT_EQUALS(
        t0->makeIndexSet(SpectrumNumber(1), SpectrumNumber(1)).size(), 0);
    TS_ASSERT_EQUALS(
        t0->makeIndexSet(SpectrumNumber(4), SpectrumNumber(4)).size(), 0);
    // Rank 1 has spectrum numbers 1
    auto t1 = makeTranslator(3, 1);
    TS_ASSERT_EQUALS(
        t1->makeIndexSet(SpectrumNumber(2), SpectrumNumber(5)).size(), 0);
    // Rank 2 has spectrum numbers 4
    auto t2 = makeTranslator(3, 2);
    TS_ASSERT_EQUALS(
        t2->makeIndexSet(SpectrumNumber(1), SpectrumNumber(2)).size(), 0);
    TS_ASSERT_EQUALS(
        t2->makeIndexSet(SpectrumNumber(5), SpectrumNumber(5)).size(), 0);
  }

  void test_makeIndexSet_minmax_GlobalSpectrumIndex_param_check_3_ranks() {
    auto t = makeTranslator(3, 1);
    TS_ASSERT_THROWS(
        t->makeIndexSet(GlobalSpectrumIndex(1), GlobalSpectrumIndex(0)),
        const std::logic_error &);
    TS_ASSERT_THROWS(
        t->makeIndexSet(GlobalSpectrumIndex(0), GlobalSpectrumIndex(4)),
        const std::out_of_range &);
    TS_ASSERT_THROWS(
        t->makeIndexSet(GlobalSpectrumIndex(5), GlobalSpectrumIndex(4)),
        const std::logic_error &);
    // -1 converted to size_t is positive and >> 1
    TS_ASSERT_THROWS(
        t->makeIndexSet(GlobalSpectrumIndex(-1), GlobalSpectrumIndex(1)),
        const std::logic_error &);
    // -1 converted to size_t is > 0 but out of range
    TS_ASSERT_THROWS(
        t->makeIndexSet(GlobalSpectrumIndex(0), GlobalSpectrumIndex(-1)),
        const std::out_of_range &);
  }

  void test_makeIndexSet_minmax_GlobalSpectrumIndex_3_ranks() {
    auto translator = makeTranslator(3, 0);
    auto set = translator->makeIndexSet(GlobalSpectrumIndex(0),
                                        GlobalSpectrumIndex(3));
    TS_ASSERT_EQUALS(set.size(), 2);
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 1);
    set = translator->makeIndexSet(GlobalSpectrumIndex(2),
                                   GlobalSpectrumIndex(3));
    TS_ASSERT_EQUALS(set.size(), 1);
    TS_ASSERT_EQUALS(set[0], 1);
  }

  void test_makeIndexSet_partial_1_rank() {
    auto translator = makeTranslator(1, 0);
    auto set1 = translator->makeIndexSet(makeSpectrumNumbers({1, 2}));
    TS_ASSERT_EQUALS(set1.size(), 2);
    // Order of spectrum numbers preserved.
    TS_ASSERT_EQUALS(set1[0], 1);
    TS_ASSERT_EQUALS(set1[1], 0);
    auto set2 = translator->makeIndexSet(makeSpectrumNumbers({4, 5}));
    TS_ASSERT_EQUALS(set2.size(), 2);
    TS_ASSERT_EQUALS(set2[0], 2);
    TS_ASSERT_EQUALS(set2[1], 3);
  }

  void test_makeIndexSet_partial_3_ranks_range_checks() {
    auto translator = makeTranslator(3, 1);
    TS_ASSERT_THROWS(translator->makeIndexSet(makeSpectrumNumbers({0})),
                     const std::out_of_range &);
    // Index is not on this rank but it is correct.
    TS_ASSERT_THROWS_NOTHING(
        translator->makeIndexSet(makeSpectrumNumbers({2})));
  }

  void test_makeIndexSet_partial_3_ranks() {
    auto translator = makeTranslator(3, 0);
    // 2 is on this rank
    auto set1 = translator->makeIndexSet(makeSpectrumNumbers({1, 2}));
    TS_ASSERT_EQUALS(set1.size(), 1);
    TS_ASSERT_EQUALS(set1[0], 0);
    // 5 is on this rank
    auto set2 = translator->makeIndexSet(makeSpectrumNumbers({4, 5}));
    TS_ASSERT_EQUALS(set2.size(), 1);
    TS_ASSERT_EQUALS(set2[0], 1);
  }

  void test_makeIndexSet_GlobalSpectrumIndex_partial_1_rank() {
    auto translator = makeTranslator(1, 0);
    auto set1 = translator->makeIndexSet(makeGlobalSpectrumIndices({0, 2}));
    TS_ASSERT_EQUALS(set1.size(), 2);
    TS_ASSERT_EQUALS(set1[0], 0);
    TS_ASSERT_EQUALS(set1[1], 2);
    auto set2 = translator->makeIndexSet(makeGlobalSpectrumIndices({1, 3}));
    TS_ASSERT_EQUALS(set2.size(), 2);
    TS_ASSERT_EQUALS(set2[0], 1);
    TS_ASSERT_EQUALS(set2[1], 3);
  }

  void test_makeIndexSet_GlobalSpectrumIndex_partial_3_ranks_range_checks() {
    auto t = makeTranslator(3, 0);
    TS_ASSERT_THROWS(t->makeIndexSet(makeGlobalSpectrumIndices({-1})),
                     const std::out_of_range &);
    TS_ASSERT_THROWS(t->makeIndexSet(makeGlobalSpectrumIndices({4})),
                     const std::out_of_range &);
    // Index is not on this rank but it is correct.
    TS_ASSERT_THROWS_NOTHING(t->makeIndexSet(makeGlobalSpectrumIndices({1})));
    TS_ASSERT_EQUALS(t->makeIndexSet(makeGlobalSpectrumIndices({1})).size(), 0);
  }

  void test_makeIndexSet_GlobalSpectrumIndex_partial_3_ranks() {
    auto translator = makeTranslator(3, 0);
    // 0 is on this rank
    auto set1 = translator->makeIndexSet(makeGlobalSpectrumIndices({0, 1}));
    TS_ASSERT_EQUALS(set1.size(), 1);
    TS_ASSERT_EQUALS(set1[0], 0);
    // 3 is on this rank
    auto set2 = translator->makeIndexSet(makeGlobalSpectrumIndices({2, 3}));
    TS_ASSERT_EQUALS(set2.size(), 1);
    TS_ASSERT_EQUALS(set2[0], 1);
  }

  void test_construct_parent_3_ranks() {
    auto parent = makeTranslator(3, 0);
    auto numbers = {2, 1, 4, 5};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());

    SpectrumNumberTranslator translator1(spectrumNumbers, *parent);
    TS_ASSERT_EQUALS(translator1.globalSize(), 4);
    TS_ASSERT_EQUALS(translator1.localSize(), 2); // 2 and 5 are on this rank.

    spectrumNumbers.erase(spectrumNumbers.begin());
    SpectrumNumberTranslator translator2(spectrumNumbers, *parent);
    TS_ASSERT_EQUALS(translator2.globalSize(), 3);
    TS_ASSERT_EQUALS(translator2.localSize(), 1);

    spectrumNumbers.erase(spectrumNumbers.begin());
    SpectrumNumberTranslator translator3(spectrumNumbers, *parent);
    TS_ASSERT_EQUALS(translator3.globalSize(), 2);
    TS_ASSERT_EQUALS(translator3.localSize(), 1);

    spectrumNumbers.erase(spectrumNumbers.end() - 1);
    SpectrumNumberTranslator translator4(spectrumNumbers, *parent);
    TS_ASSERT_EQUALS(translator4.globalSize(), 1);
    TS_ASSERT_EQUALS(translator4.localSize(), 0);
  }
};

#endif /* MANTID_INDEXING_SPECTRUMNUMBERTRANSLATORTEST_H_ */
