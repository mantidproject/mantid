
#ifndef LOADMCSTASTEST_H_
#define LOADMCSTASTEST_H_

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMcStas.h"
// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this
// test case.
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidDataHandling/LoadInstrument.h"
#include <Poco/Path.h>
#include <Poco/TemporaryFile.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

//
// Test checks if number  of workspace equals one
// Test checks if number getNumberHistograms = 2x4096. (64x64= 4096 pixels in
// one detector)
//
class LoadMcStasTest : public CxxTest::TestSuite {
public:
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void testExec() {
    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    outputSpace = "LoadMcStasTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), std::runtime_error);

    // Now set it...
    // specify name of file to load workspace from
    inputFile = "mcstas_event_hist.h5";
    algToBeTested.setPropertyValue("Filename", inputFile);

    // mark the temp file to be deleted upon end of execution
    { // limit variable scope
      std::string tempFile = algToBeTested.getPropertyValue("Filename");
      tempFile = tempFile.substr(0, tempFile.size() - 2) + "vtp";
      Poco::TemporaryFile::registerForDeletion(tempFile);
    }

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    std::string postfix = "_" + outputSpace;
    //
    //  test workspace created by LoadMcStas
    WorkspaceGroup_sptr output =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace);
    TS_ASSERT_EQUALS(output->getNumberOfEntries(), 7); // 5 NXdata groups
    //
    //
    MatrixWorkspace_sptr outputItem1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "EventData" + postfix);
    TS_ASSERT_EQUALS(outputItem1->getNumberHistograms(), 8192);
    auto sum_total = 0.0;
    for (size_t i = 0; i < outputItem1->getNumberHistograms(); i++)
		sum_total += outputItem1->y(i)[0];
	sum_total *= 1.0e22;
    TS_ASSERT_DELTA(sum_total, 107163.7851, 0.0001);
	//
	//
	MatrixWorkspace_sptr outputItem2 =
		AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EventData" +
			postfix + "_1");
	TS_ASSERT_EQUALS(outputItem2->getNumberHistograms(), 8192);
	auto sum_single = 0.0;
	for (size_t i = 0; i < outputItem2->getNumberHistograms(); i++)
		sum_single += outputItem2->y(i)[0];
	sum_single *= 1.0e22;
	TS_ASSERT_DELTA(sum_single, 107141.3295, 0.0001);
	//
	//
	MatrixWorkspace_sptr outputItem3 =
		AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EventData" +
			postfix + "_2");
	TS_ASSERT_EQUALS(outputItem3->getNumberHistograms(), 8192);
	auto sum_multiple = 0.0;
	for (size_t i = 0; i < outputItem3->getNumberHistograms(); i++)
		sum_multiple += outputItem3->y(i)[0];
	sum_multiple *= 1.0e22;
	TS_ASSERT_DELTA(sum_multiple, 22.4558, 0.0001);

	TS_ASSERT_DELTA(sum_total, (sum_single + sum_multiple), 0.0001);
    //
    //
    MatrixWorkspace_sptr outputItem4 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Edet.dat" +
                                                                    postfix);
    TS_ASSERT_EQUALS(outputItem4->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputItem4->getNPoints(), 1000);
    //
    //
    MatrixWorkspace_sptr outputItem5 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("PSD.dat" +
                                                                    postfix);
    TS_ASSERT_EQUALS(outputItem5->getNumberHistograms(), 128);
    //
    //
    MatrixWorkspace_sptr outputItem6 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "psd2_av.dat" + postfix);
    TS_ASSERT_EQUALS(outputItem6->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputItem6->getNPoints(), 100);
    //
    //
    MatrixWorkspace_sptr outputItem7 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("psd2.dat" +
                                                                    postfix);
    TS_ASSERT_EQUALS(outputItem7->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputItem7->getNPoints(), 100);
  } // testExec()

private:
  LoadMcStas algToBeTested;
  std::string inputFile;
  std::string outputSpace;
};

class LoadMcStasTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadMcStasTestPerformance *createSuite() {
    return new LoadMcStasTestPerformance();
  }

  static void destroySuite(LoadMcStasTestPerformance *suite) { delete suite; }

  void setUp() override {
    loadFile.initialize();
    loadFile.setProperty("Filename", "mcstas_event_hist.h5");
    loadFile.setProperty("OutputWorkspace", "outputWS");
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("outputWS");
  }

  void testDefaultLoad() { loadFile.execute(); }

private:
  LoadMcStas loadFile;
};

#endif /*LoadMcStasTEST_H_*/
