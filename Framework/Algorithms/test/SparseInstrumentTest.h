#ifndef SPARSEINSTRUMENTTEST_H_
#define SPARSEINSTRUMENTTEST_H_

#include "MantidAlgorithms/SampleCorrections/SparseInstrument.h"

class SparseInstrumentTest : public CxxTest::TestSuite {
public:
  static SparseInstrumentTest *createSuite() {
    return new SparseInstrumentTest();
  }

  static void destroySuite(SparseInstrumentTest *suite) {
    delete suite;
  }

  void test_Fail() {
    TS_FAIL("TODO tests");
  }
};

#endif // SPARSEINSTRUMENTTEST_H_
