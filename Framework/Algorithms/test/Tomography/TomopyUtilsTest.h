#ifndef MANTID_ALGORITHMS_TOMOPYUTILS_H_
#define MANTID_ALGORITHMS_TOMOPYUTILS_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Tomography/tomopy/utils.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class TomopyUtilsTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TomopyUtilsTest *createSuite() { return new TomopyUtilsTest(); }

  static void destroySuite(TomopyUtilsTest *suite) { delete suite; }

  void test_run() {}
};

#endif /* MANTID_ALGORITHM_TOMOPYUTILS_H_ */
