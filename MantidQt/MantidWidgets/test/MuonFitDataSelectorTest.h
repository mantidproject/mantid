#ifndef MANTID_MANTIDWIDGETS_MUONFITDATASELECTORTEST_H_
#define MANTID_MANTIDWIDGETS_MUONFITDATASELECTORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMantidWidgets/MuonFitDataSelector.h"

using Mantid::MantidWidgets::MuonFitDataSelector;

class MuonFitDataSelectorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonFitDataSelectorTest *createSuite() {
    return new MuonFitDataSelectorTest();
  }
  static void destroySuite(MuonFitDataSelectorTest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_MANTIDWIDGETS_MUONFITDATASELECTORTEST_H_ */