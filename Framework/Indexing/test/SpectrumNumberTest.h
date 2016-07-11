#ifndef MANTID_INDEXING_SPECTRUMNUMBERTEST_H_
#define MANTID_INDEXING_SPECTRUMNUMBERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/SpectrumNumber.h"

using Mantid::Indexing::SpectrumNumber;

class SpectrumNumberTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumNumberTest *createSuite() { return new SpectrumNumberTest(); }
  static void destroySuite(SpectrumNumberTest *suite) { delete suite; }

  void test_construct() {
    SpectrumNumber number(42);
    TS_ASSERT_EQUALS(number, 42);
  }

  void test_operator_equals() {
    SpectrumNumber data(42);
    SpectrumNumber same(42);
    SpectrumNumber different(100);
    TS_ASSERT(data == data);
    TS_ASSERT(data == same);
    TS_ASSERT(!(data == different));
    TS_ASSERT(data == 42);
    TS_ASSERT(!(data == 100));
  }

  void test_operator_not_equals() {
    SpectrumNumber data(42);
    SpectrumNumber same(42);
    SpectrumNumber different(100);
    TS_ASSERT(!(data != data));
    TS_ASSERT(!(data != same));
    TS_ASSERT(data != different);
    TS_ASSERT(!(data != 42));
    TS_ASSERT(data != 100);
  }
};

#endif /* MANTID_INDEXING_SPECTRUMNUMBERTEST_H_ */
