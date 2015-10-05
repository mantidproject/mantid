#ifndef MANTID_CRYSTAL_SAVEPEAKSFILETEST_H_
#define MANTID_CRYSTAL_SAVEPEAKSFILETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidCrystal/SavePeaksFile.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Crystal;

class SavePeaksFileTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SavePeaksFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // TODO: Write a test when the algo is complete
  }
};

#endif /* MANTID_CRYSTAL_SAVEPEAKSFILETEST_H_ */
