
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
    outputSpace = "LoadMcStasTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), std::runtime_error);

    load_test("mcstas_event_hist.h5", outputSpace);

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
    const auto sum_total = extractSumAndTest(outputItem1, 107163.7851);
    //
    //
    MatrixWorkspace_sptr outputItem2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Edet.dat" +
                                                                    postfix);
    TS_ASSERT_EQUALS(outputItem2->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputItem2->getNPoints(), 1000);
    //
    //
    MatrixWorkspace_sptr outputItem3 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("PSD.dat" +
                                                                    postfix);
    TS_ASSERT_EQUALS(outputItem3->getNumberHistograms(), 128);
    //
    //
    MatrixWorkspace_sptr outputItem4 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "psd2_av.dat" + postfix);
    TS_ASSERT_EQUALS(outputItem4->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputItem4->getNPoints(), 100);
    //
    //
    MatrixWorkspace_sptr outputItem5 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("psd2.dat" +
                                                                    postfix);
    TS_ASSERT_EQUALS(outputItem5->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputItem5->getNPoints(), 100);
    //
    //
    MatrixWorkspace_sptr outputItem6 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "k01_events_dat_list_p_x_y_n_id_t" + postfix);
    const auto sum_single = extractSumAndTest(outputItem6, 107141.3295);
    //
    //
    MatrixWorkspace_sptr outputItem7 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "k02_events_dat_list_p_x_y_n_id_t" + postfix);
    const auto sum_multiple = extractSumAndTest(outputItem7, 22.4558);

    TS_ASSERT_DELTA(sum_total, (sum_single + sum_multiple), 0.0001);
  }

  void testLoadMultiple() {
    outputSpace = "LoadMcStasTest";
    algToBeTested.setProperty("OutputWorkspace", outputSpace);
    auto outputGroup = load_test("mccode_contains_one_bank.h5", outputSpace);
    TS_ASSERT_EQUALS(outputGroup->getNumberOfEntries(), 6);
    outputGroup = load_test("mccode_multiple_scattering.h5", outputSpace);
    TS_ASSERT_EQUALS(outputGroup->getNumberOfEntries(), 3);
  }

  void testLoadTwice() {
    outputSpace = "LoadMcStasTest";
    algToBeTested.setProperty("OutputWorkspace", outputSpace);
    load_test("mccode_contains_one_bank.h5", outputSpace);
    auto outputGroup = load_test("mccode_contains_one_bank.h5", outputSpace);
    TS_ASSERT_EQUALS(outputGroup->getNumberOfEntries(), 6);
  }

private:
  double extractSumAndTest(MatrixWorkspace_sptr workspace,
                           const double &expectedSum) {
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 8192);
    auto sum = 0.0;
    for (auto i = 0u; i < workspace->getNumberHistograms(); ++i)
      sum += workspace->y(i)[0];
    sum *= 1.0e22;
    TS_ASSERT_DELTA(sum, expectedSum, 0.0001);
    return sum;
  }
  boost::shared_ptr<WorkspaceGroup> load_test(std::string fileName,
                                              std::string outputName) {

    // specify name of file to load workspace from
    algToBeTested.setProperty("Filename", fileName);

    // mark the temp file to be deleted upon end of execution
    { // limit variable scope
      std::string tempFile = algToBeTested.getPropertyValue("Filename");
      tempFile = tempFile.substr(0, tempFile.size() - 2) + "vtp";
      Poco::TemporaryFile::registerForDeletion(tempFile);
    }
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    auto outputGroup(
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputName));
    return outputGroup;
  }

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
