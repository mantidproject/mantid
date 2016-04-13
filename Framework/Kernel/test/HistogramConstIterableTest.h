#ifndef MANTID_KERNEL_HISTOGRAM_CONSTITERABLETEST_H_
#define MANTID_KERNEL_HISTOGRAM_CONSTITERABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Histogram/ConstIterable.h"
#include "MantidKernel/Histogram/VectorOf.h"

using namespace Mantid;
using namespace Kernel;

class ConstIterableTester : public VectorOf<ConstIterableTester>,
                            public ConstIterable<ConstIterableTester> {
  using VectorOf<ConstIterableTester>::VectorOf;
  using VectorOf<ConstIterableTester>::operator=;
};

class HistogramConstIterableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramConstIterableTest *createSuite() {
    return new HistogramConstIterableTest();
  }
  static void destroySuite(HistogramConstIterableTest *suite) { delete suite; }

  void test_index_operator() {
    ConstIterableTester testee = {1.0, 2.0, 4.0};
    TS_ASSERT_EQUALS(testee[0], 1.0);
    TS_ASSERT_EQUALS(testee[1], 2.0);
    TS_ASSERT_EQUALS(testee[2], 4.0);
  }

  void test_cbegin() {
    ConstIterableTester test{};
    TS_ASSERT_THROWS_NOTHING(cbegin(ConstIterableTester(0)));
    TS_ASSERT_THROWS_NOTHING(cbegin(ConstIterableTester{2}));
    TS_ASSERT_THROWS_NOTHING(cbegin(ConstIterableTester{2, 0.1}));
  }

  void test_cend() {
    TS_ASSERT_THROWS_NOTHING(cend(ConstIterableTester(0)));
    TS_ASSERT_THROWS_NOTHING(cend(ConstIterableTester{2}));
    TS_ASSERT_THROWS_NOTHING(cend(ConstIterableTester{2, 0.1}));
  }

  void test_begin_end_arithmetics() {
    ConstIterableTester testee1(0);
    TS_ASSERT_EQUALS(cbegin(testee1), cend(testee1));

    ConstIterableTester testee2(1);
    TS_ASSERT_DIFFERS(cbegin(testee2), cend(testee2));
    TS_ASSERT_EQUALS(cbegin(testee2) + 1, cend(testee2));

    ConstIterableTester testee3(3);
    TS_ASSERT_DIFFERS(cbegin(testee3), cend(testee3));
    TS_ASSERT_EQUALS(cend(testee3) - cbegin(testee3), 3);
    TS_ASSERT_EQUALS(cbegin(testee3) + 3, cend(testee3));
  }

  void test_values() {
    ConstIterableTester testee = {1.0, 2.0, 4.0};
    auto it = cbegin(testee);
    TS_ASSERT_EQUALS(*it, 1.0);
    TS_ASSERT_EQUALS(*(++it), 2.0);
    TS_ASSERT_EQUALS(*(++it), 4.0);
    TS_ASSERT_EQUALS(++it, cend(testee));
  }
};

#endif /* MANTID_KERNEL_HISTOGRAM_CONSTITERABLETEST_H_ */
