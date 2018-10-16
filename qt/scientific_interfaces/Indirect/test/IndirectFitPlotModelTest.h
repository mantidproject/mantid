#ifndef MANTID_INDIRECTFITPLOTMODELTEST_H_
#define MANTID_INDIRECTFITPLOTMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitPlotModel.h"

class IndirectFitPlotModelTest : public CxxTest::TestSuite {
public:
  static IndirectFitPlotModelTest *createSuite() {
    return new IndirectFitPlotModelTest();
  }

  static void destroySuite(IndirectFitPlotModelTest *suite) { delete suite; }

  void test_test() {}
};

#endif
