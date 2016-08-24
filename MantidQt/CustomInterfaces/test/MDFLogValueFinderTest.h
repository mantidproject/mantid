#ifndef MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDERTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFLogValueFinder.h"

using MantidQt::CustomInterfaces::MDFLogValueFinder;

class MDFLogValueFinderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDFLogValueFinderTest *createSuite() {
    return new MDFLogValueFinderTest();
  }
  static void destroySuite(MDFLogValueFinderTest *suite) { delete suite; }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTIDQT_CUSTOMINTERFACES_MDFLOGVALUEFINDERTEST_H_ */