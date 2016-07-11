#ifndef MANTID_INDEXING_SPECTRUMNUMBERTRANSLATORTEST_H_
#define MANTID_INDEXING_SPECTRUMNUMBERTRANSLATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/RoundRobinPartitioning.h"
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

  SpectrumNumberTranslator makeTranslator(int ranks, int rank) {
    auto numbers = {2, 1, 4, 5};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    return {spectrumNumbers, RoundRobinPartitioning(ranks, rank)};
  }

  std::vector<SpectrumNumber>
  makeSpectrumNumbers(std::initializer_list<int64_t> init) {
    return std::vector<SpectrumNumber>(init.begin(), init.end());
  }

  template <class... T>
  std::vector<SpectrumNumber> makeSpectrumNumbers(T &&... args) {
    return std::vector<SpectrumNumber>(std::forward<T>(args)...);
    // std::vector<int64_t> numbers(std::forward<T>(args)...);
    // return std::vector<SpectrumNumber>(numbers.begin(), numbers.end());
  }

  void test_construct() {
    auto numbers = {1, 2, 3, 4};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    TS_ASSERT_THROWS_NOTHING(SpectrumNumberTranslator(
        spectrumNumbers, RoundRobinPartitioning(1, 0)));
  }

  void test_makeIndexSet_full_1_rank() {
    auto translator = makeTranslator(1, 0);
    auto set = translator.makeIndexSet();
    TS_ASSERT_EQUALS(set.size(), 4);
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
    TS_ASSERT_EQUALS(set.size(), 2);
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 1);
  }

  void test_makeIndexSet_partial_1_rank() {
    auto translator = makeTranslator(1, 0);
    auto set1 = translator.makeIndexSet(makeSpectrumNumbers({1, 2}));
    TS_ASSERT_EQUALS(set1.size(), 2);
    TS_ASSERT_EQUALS(set1[0], 0);
    TS_ASSERT_EQUALS(set1[1], 1);
    auto set2 = translator.makeIndexSet(makeSpectrumNumbers({4, 5}));
    TS_ASSERT_EQUALS(set2.size(), 2);
    TS_ASSERT_EQUALS(set2[0], 2);
    TS_ASSERT_EQUALS(set2[1], 3);
  }

  void test_makeIndexSet_partial_3_ranks_range_checks() {
    auto translator = makeTranslator(3, 1);
    TS_ASSERT_THROWS(translator.makeIndexSet(makeSpectrumNumbers({0})),
                     std::out_of_range);
    // Index is not on this rank but it is correct.
    TS_ASSERT_THROWS_NOTHING(translator.makeIndexSet(makeSpectrumNumbers({2})));
  }

  void test_makeIndexSet_partial_3_ranks() {
    auto translator = makeTranslator(3, 1);
    // 1 is on this rank
    auto set1 = translator.makeIndexSet(makeSpectrumNumbers({1, 2}));
    TS_ASSERT_EQUALS(set1.size(), 1);
    TS_ASSERT_EQUALS(set1[0], 0);
    // 4 is on this rank
    auto set2 = translator.makeIndexSet(makeSpectrumNumbers({4, 5}));
    TS_ASSERT_EQUALS(set2.size(), 1);
    TS_ASSERT_EQUALS(set2[0], 1);
  }
};

#endif /* MANTID_INDEXING_SPECTRUMNUMBERTRANSLATORTEST_H_ */
