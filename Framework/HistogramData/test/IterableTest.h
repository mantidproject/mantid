#ifndef MANTID_HISTOGRAMDATA_ITERABLETEST_H_
#define MANTID_HISTOGRAMDATA_ITERABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/VectorOf.h"

using Mantid::HistogramData::detail::Iterable;
using Mantid::HistogramData::detail::VectorOf;

class IterableTester : public VectorOf<IterableTester, std::vector<double>>,
                       public Iterable<IterableTester> {
  using VectorOf<IterableTester, std::vector<double>>::VectorOf;
  using VectorOf<IterableTester, std::vector<double>>::operator=;
};

class IterableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IterableTest *createSuite() { return new IterableTest(); }
  static void destroySuite(IterableTest *suite) { delete suite; }

  void test_index_operator() {
    IterableTester testee = {1.0, 2.0, 4.0};
    TS_ASSERT_EQUALS(testee[0], 1.0);
    TS_ASSERT_EQUALS(testee[1], 2.0);
    TS_ASSERT_EQUALS(testee[2], 4.0);
  }

  void test_cbegin() {
    IterableTester test{};
    TS_ASSERT_THROWS_NOTHING(cbegin(IterableTester(0)));
    TS_ASSERT_THROWS_NOTHING(cbegin(IterableTester{2}));
    TS_ASSERT_THROWS_NOTHING(cbegin(IterableTester{2, 0.1}));
  }

  void test_cend() {
    TS_ASSERT_THROWS_NOTHING(cend(IterableTester(0)));
    TS_ASSERT_THROWS_NOTHING(cend(IterableTester{2}));
    TS_ASSERT_THROWS_NOTHING(cend(IterableTester{2, 0.1}));
  }

  void test_begin_end_arithmetics() {
    IterableTester testee1(0);
    TS_ASSERT_EQUALS(cbegin(testee1), cend(testee1));

    IterableTester testee2(1);
    TS_ASSERT_DIFFERS(cbegin(testee2), cend(testee2));
    TS_ASSERT_EQUALS(cbegin(testee2) + 1, cend(testee2));

    IterableTester testee3(3);
    TS_ASSERT_DIFFERS(cbegin(testee3), cend(testee3));
    TS_ASSERT_EQUALS(cend(testee3) - cbegin(testee3), 3);
    TS_ASSERT_EQUALS(cbegin(testee3) + 3, cend(testee3));
  }

  void test_values() {
    IterableTester testee = {1.0, 2.0, 4.0};
    auto it = cbegin(testee);
    TS_ASSERT_EQUALS(*it, 1.0);
    TS_ASSERT_EQUALS(*(++it), 2.0);
    TS_ASSERT_EQUALS(*(++it), 4.0);
    TS_ASSERT_EQUALS(++it, cend(testee));
  }

  void test_front_back() {
    IterableTester testee{1.0, 2.0, 4.0};
    IterableTester clone(testee);
    TS_ASSERT_EQUALS(testee.front(), 1.0);
    TS_ASSERT_EQUALS(testee.back(), 4.0);
    // Check if sharing is preserved
    TS_ASSERT_EQUALS(&clone[0], &testee[0]);
  }
};

#endif /* MANTID_HISTOGRAMDATA_ITERABLETEST_H_ */
