// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
/// static Logger definition
Logger g_log("LoadCalFileTest");
} // namespace

class LoadCalFileTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    g_log.notice("\ntest_Init...");

    LoadCalFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    g_log.notice("\ntest_exec...");

    // Name of the output workspace.
    std::string outWSName("LoadCalFileTest");

    LoadCalFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InstrumentName", "GEM"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MakeGroupingWorkspace", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MakeOffsetsWorkspace", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MakeMaskWorkspace", true));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CalFilename", "offsets_2006_cycle064.cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("WorkspaceName", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    std::string title = "offsets_2006_cycle064.cal";

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    GroupingWorkspace_sptr groupWS;
    TS_ASSERT_THROWS_NOTHING(groupWS =
                                 AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(outWSName + "_group"));
    TS_ASSERT(groupWS);
    if (!groupWS)
      return;
    TS_ASSERT_EQUALS(groupWS->getTitle(), title);
    TS_ASSERT_EQUALS(int(groupWS->getValue(101001)), 2);
    TS_ASSERT_EQUALS(int(groupWS->getValue(715079)), 7);
    // Check if filename is saved
    TS_ASSERT_EQUALS(alg.getPropertyValue("CalFilename"), groupWS->run().getProperty("Filename")->value());

    OffsetsWorkspace_sptr offsetsWS;
    TS_ASSERT_THROWS_NOTHING(offsetsWS =
                                 AnalysisDataService::Instance().retrieveWS<OffsetsWorkspace>(outWSName + "_offsets"));
    TS_ASSERT(offsetsWS);
    if (!offsetsWS)
      return;
    TS_ASSERT_EQUALS(offsetsWS->getTitle(), title);
    TS_ASSERT_DELTA(offsetsWS->getValue(101001), -0.0497075, 1e-7);
    TS_ASSERT_DELTA(offsetsWS->getValue(714021), 0.0007437, 1e-7);
    // Check if filename is saved
    TS_ASSERT_EQUALS(alg.getPropertyValue("CalFilename"), offsetsWS->run().getProperty("Filename")->value());

    SpecialWorkspace2D_sptr maskWS;
    TS_ASSERT_THROWS_NOTHING(maskWS =
                                 AnalysisDataService::Instance().retrieveWS<SpecialWorkspace2D>(outWSName + "_mask"));
    TS_ASSERT(maskWS);
    if (!maskWS)
      return;
    TS_ASSERT_EQUALS(maskWS->getTitle(), title);
    /*
    TS_ASSERT_EQUALS( int(maskWS->getValue(101001)), 1 );
    TS_ASSERT_EQUALS( int(maskWS->getValue(101003)), 0 );
    TS_ASSERT_EQUALS( int(maskWS->getValue(101008)), 0 );
    TS_ASSERT_EQUALS( int(maskWS->getValue(715079)), 1 );
    */
    TS_ASSERT_EQUALS(int(maskWS->getValue(101001)), 0);
    TS_ASSERT_EQUALS(int(maskWS->getValue(101003)), 1);
    TS_ASSERT_EQUALS(int(maskWS->getValue(101008)), 1);
    TS_ASSERT_EQUALS(int(maskWS->getValue(715079)), 0);
    const auto &detectorInfo = maskWS->detectorInfo();
    TS_ASSERT(!detectorInfo.isMasked(detectorInfo.indexOf(101001)));
    TS_ASSERT(detectorInfo.isMasked(detectorInfo.indexOf(101003)));
    TS_ASSERT(detectorInfo.isMasked(detectorInfo.indexOf(101008)));
    TS_ASSERT(!detectorInfo.isMasked(detectorInfo.indexOf(715079)));
    // Check if filename is saved
    TS_ASSERT_EQUALS(alg.getPropertyValue("CalFilename"), maskWS->run().getProperty("Filename")->value());
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName + "_group");
    AnalysisDataService::Instance().remove(outWSName + "_offsets");
    AnalysisDataService::Instance().remove(outWSName + "_mask");
  }
};

class LoadCalFileTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    // Since we have no control over the cal file size
    // instead we setup lots of load algorithms and run it
    // multiple times to create a stable time for this test
    loadAlgPtrArray.resize(numberOfIterations);
    for (auto &vectorItem : loadAlgPtrArray) {
      vectorItem = setupAlg();
    }
  }

  void testLoadCalFilePerformance() {
    g_log.notice("\ntestLoadCalFilePerformance...");

    for (int i = 0; i < numberOfIterations; i++) {
      TS_ASSERT_THROWS_NOTHING(loadAlgPtrArray[i]->execute());
    }
  }

  void tearDown() override {
    for (auto &i : loadAlgPtrArray) {
      delete i;
    }
    loadAlgPtrArray.clear();

    AnalysisDataService::Instance().remove(outWSName);
  }

private:
  const int numberOfIterations = 5; // Controls performance test speed
  std::vector<LoadCalFile *> loadAlgPtrArray;

  const std::string outWSName = "LoadCalFileTest";

  LoadCalFile *setupAlg() {

    LoadCalFile *loadAlg = new LoadCalFile;

    loadAlg->initialize();
    loadAlg->setPropertyValue("InstrumentName", "GEM");
    loadAlg->setProperty("MakeGroupingWorkspace", true);
    loadAlg->setProperty("MakeOffsetsWorkspace", true);
    loadAlg->setProperty("MakeMaskWorkspace", true);
    loadAlg->setPropertyValue("CalFilename", "offsets_2006_cycle064.cal");
    loadAlg->setPropertyValue("WorkspaceName", outWSName);

    loadAlg->setRethrows(true);

    return loadAlg;
  }
};
