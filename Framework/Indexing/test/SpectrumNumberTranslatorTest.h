#ifndef MANTID_INDEXING_SPECTRUMNUMBERTRANSLATORTEST_H_
#define MANTID_INDEXING_SPECTRUMNUMBERTRANSLATORTEST_H_

#include <cxxtest/TestSuite.h>

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

  SpectrumNumberTranslator makeTranslator() {
    // 2 -> 0
    // 1 -> 1
    // 4 -> 2
    // 5 -> 3
    auto numbers = {2, 1, 4, 5};
    std::vector<SpectrumNumber> spectrumNumbers(numbers.begin(), numbers.end());
    std::vector<size_t> indices{0, 1, 2, 3};
    return {spectrumNumbers, indices};
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
    std::vector<size_t> indices{0, 1, 2, 3};
    TS_ASSERT_THROWS_NOTHING(
        SpectrumNumberTranslator(spectrumNumbers, indices));
  }

  void test_makeIndexSet_full() {
    auto translator = makeTranslator();
    auto set = translator.makeIndexSet();
    TS_ASSERT_EQUALS(set.size(), 4);
    TS_ASSERT_EQUALS(set[0], 0);
    TS_ASSERT_EQUALS(set[1], 1);
    TS_ASSERT_EQUALS(set[2], 2);
    TS_ASSERT_EQUALS(set[3], 3);
  }

  void test_makeIndexSet_partial() {
    auto translator = makeTranslator();
    auto set1 = translator.makeIndexSet(makeSpectrumNumbers({1, 2}));
    TS_ASSERT_EQUALS(set1.size(), 2);
    TS_ASSERT_EQUALS(set1[0], 0);
    TS_ASSERT_EQUALS(set1[1], 1);
    auto set2 = translator.makeIndexSet(makeSpectrumNumbers({4, 5}));
    TS_ASSERT_EQUALS(set2.size(), 2);
    TS_ASSERT_EQUALS(set2[0], 2);
    TS_ASSERT_EQUALS(set2[1], 3);
  }
};

#endif /* MANTID_INDEXING_SPECTRUMNUMBERTRANSLATORTEST_H_ */
