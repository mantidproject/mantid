#ifndef LOADMCSTASNEXUSTEST_H_
#define LOADMCSTASNEXUSTEST_H_

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMcStasNexus.h"
// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this
// test case.
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidDataHandling/LoadInstrument.h"
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

//
// test does:
//
class LoadMcStasNexusTest : public CxxTest::TestSuite {
public:
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void testExec() {
    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    outputSpace = "LoadMcStasNexusTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), std::runtime_error);

    // Now set it...
    // specify name of file to load workspace from
    inputFile = "mcstas.n5";
    algToBeTested.setPropertyValue("Filename", inputFile);

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());
    //
    //  test workspace created by LoadMcStasNexus
    WorkspaceGroup_sptr output =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace);
    TS_ASSERT_EQUALS(output->getNumberOfEntries(), 4);
    // int ii;
    // std::cin >> ii;
    MatrixWorkspace_sptr outputItem1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace + "_1");
    TS_ASSERT_EQUALS(outputItem1->getNumberHistograms(), 1);
    MatrixWorkspace_sptr outputItem2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace + "_2");
    TS_ASSERT_EQUALS(outputItem2->getNumberHistograms(), 128);
  }

private:
  LoadMcStasNexus algToBeTested;
  std::string inputFile;
  std::string outputSpace;
};

#endif /*LOADMCSTASNEXUSTEST_H_*/
