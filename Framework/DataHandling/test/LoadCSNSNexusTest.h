// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//      NScD Oak Ridge National Laboratory, European Spallation Source,
//      Institut Laue - Langevin
//      &  CSNS, Institute of High Energy Physics，Chinese Academy of Sciences.
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOADCSNSNEXUSTEST_H_
#define LOADCSNSNEXUSTEST_H_

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadCSNSNexus.h"
// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this
// test case.
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

//
// test does:
//
class LoadCSNSNexusTest : public CxxTest::TestSuite {
public:
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void testExec() {
    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    outputSpace = "LoadCSNSNexusTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);

    // Now set it...
    // specify name of file to load workspace from
    inputFile = "CSNS_GPPD_test.nxs";
    algToBeTested.setPropertyValue("Filename", inputFile);
    algToBeTested.setPropertyValue("Instrument", "GPPD");
    // algToBeTested.setPropertyValue("LoadBank",true );
    algToBeTested.setPropertyValue("Bankname", "module322");
    // algToBeTested.setPropertyValue("LoadEvent",false );
    // algToBeTested.setPropertyValue("LoadMonitor",true );
    algToBeTested.setPropertyValue("Monitorname", "monitor2");

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());
    //
    //  test workspace created by LoadCSNSNexus
    // WorkspaceGroup_sptr output =
    //    AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace);
    // TS_ASSERT_EQUALS(output->getNumberOfEntries(), 4);
    // int ii;
    // std::cin >> ii;
    MatrixWorkspace_sptr outputItem1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace + "_1");
    TS_ASSERT_EQUALS(outputItem1->getNumberHistograms(), 5328);
    MatrixWorkspace_sptr outputItem2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace + "_2");
    TS_ASSERT_EQUALS(outputItem2->getNumberHistograms(), 1024);

    AnalysisDataService::Instance().remove(outputSpace + "_1");
    AnalysisDataService::Instance().remove(outputSpace + "_2");
  }

private:
  LoadCSNSNexus algToBeTested;
  std::string inputFile;
  std::string outputSpace;
};

class LoadCSNSNexusTestPerformance : public CxxTest::TestSuite {
public:
  static LoadCSNSNexusTestPerformance *createSuite() {
    return new LoadCSNSNexusTestPerformance();
  }

  static void destroySuite(LoadCSNSNexusTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    if (!loadCSNSNexusAlg.isInitialized())
      loadCSNSNexusAlg.initialize();

    outputSpace = "LoadCSNSNexusTest";
    loadCSNSNexusAlg.setPropertyValue("OutputWorkspace", outputSpace);

    inputFile = "CSNS_GPPD_test.nxs";
    loadCSNSNexusAlg.setPropertyValue("Filename", inputFile);
    loadCSNSNexusAlg.setPropertyValue("Bankname", "module322");
    loadCSNSNexusAlg.setPropertyValue("Monitorname", "monitor2");
  }
  void tearDown() override {
    AnalysisDataService::Instance().remove("LoadCSNSNexusTest_1");
    AnalysisDataService::Instance().remove("LoadCSNSNexusTest_2");
  }

  void testExec() { loadCSNSNexusAlg.execute(); }

private:
  LoadCSNSNexus loadCSNSNexusAlg;
  std::string inputFile;
  std::string outputSpace;
};

#endif /*LOADCSNSNEXUSTEST_H_*/
