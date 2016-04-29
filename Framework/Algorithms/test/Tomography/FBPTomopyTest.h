#ifndef MANTID_ALGORITHMS_FBPTOMOPYTEST_H_
#define MANTID_ALGORITHMS_FBPTOMOPYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Tomography/FBPTomopy.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Exception.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class FBPTomopyTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FBPTomopyTest *createSuite() { return new FBPTomopyTest(); }

  static void destroySuite(FBPTomopyTest *suite) { delete suite; }

  void test_run() {}
};

#endif /* MANTID_ALGORITHM_FBPTOMOPYTEST_H_ */
