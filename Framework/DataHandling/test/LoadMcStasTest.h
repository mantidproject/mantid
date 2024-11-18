// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMcStas.h"
// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this
// test case.
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
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
  void testLoadHistPlusEvent() {
    outputSpace = "LoadMcStasTestLoadHistPlusEvent";

    load_test("mcstas_event_hist.h5", outputSpace, true);

    std::string postfix = "_" + outputSpace;

    //  test if expected number of workspaces returned
    WorkspaceGroup_sptr output = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace);
    TS_ASSERT_EQUALS(output->getNumberOfEntries(), 5);

    // check if event data was loaded
    MatrixWorkspace_sptr outputItemEvent =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EventData" + postfix);
    extractSumAndTest(outputItemEvent, 107163.7852);

    // check if the 4 histogram workspaced were loaded
    MatrixWorkspace_sptr outputItemHist1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("Edet.dat" + postfix);
    TS_ASSERT_EQUALS(outputItemHist1->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputItemHist1->getNPoints(), 1000);

    MatrixWorkspace_sptr outputItemHist2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("PSD.dat" + postfix);
    TS_ASSERT_EQUALS(outputItemHist2->getNumberHistograms(), 128);

    MatrixWorkspace_sptr outputItemHist3 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("psd2_av.dat" + postfix);
    TS_ASSERT_EQUALS(outputItemHist3->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputItemHist3->getNPoints(), 100);

    MatrixWorkspace_sptr outputItemHist4 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("psd2.dat" + postfix);
    TS_ASSERT_EQUALS(outputItemHist4->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outputItemHist4->getNPoints(), 100);
  }

  // Same as above but with OutputOnlySummedEventWorkspace = false
  // The mcstas_event_hist.h5 dataset contains two mcstas event data
  // components, hence where two additional event datasets are returned
  void testLoadHistPlusEvent2() {
    outputSpace = "LoadMcStasTestLoadHistPlusEvent2";

    load_test("mcstas_event_hist.h5", outputSpace);

    std::string postfix = "_" + outputSpace;

    // test if expected number of workspaces returned
    WorkspaceGroup_sptr output = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace);
    TS_ASSERT_EQUALS(output->getNumberOfEntries(), 7);

    // load the summed eventworkspace
    MatrixWorkspace_sptr outputItemEvent =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("EventData" + postfix);
    const auto sumTotal = extractSumAndTest(outputItemEvent, 107163.7852);

    MatrixWorkspace_sptr outputItemEvent_k01 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("k01_events_dat_list_p_x_y_n_id_t" + postfix);
    const auto sum_k01 = extractSumAndTest(outputItemEvent_k01, 107141.3295);

    MatrixWorkspace_sptr outputItemEvent_k02 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("k02_events_dat_list_p_x_y_n_id_t" + postfix);
    const auto sum_k02 = extractSumAndTest(outputItemEvent_k02, 22.4558);

    TS_ASSERT_DELTA(sumTotal, (sum_k01 + sum_k02), 0.0001);
  }

  void testLoadMultipleDatasets() {
    outputSpace = "LoadMcStasTestLoadMultipleDatasets";
    // load one dataset
    auto outputGroup = load_test("mccode_contains_one_bank.h5", outputSpace);
    TS_ASSERT_EQUALS(outputGroup->getNumberOfEntries(), 6);
    // load another dataset
    outputGroup = load_test("mccode_multiple_scattering.h5", outputSpace);
    TS_ASSERT_EQUALS(outputGroup->getNumberOfEntries(), 3);
  }

  void testLoadSameDataTwice() {
    outputSpace = "LoadMcStasTestLoadSameDataTwice";
    // load the same dataset twice
    load_test("mccode_contains_one_bank.h5", outputSpace, true);
    auto outputGroup = load_test("mccode_contains_one_bank.h5", outputSpace);
    TS_ASSERT_EQUALS(outputGroup->getNumberOfEntries(), 6);
  }

  // same as above but for a different dataset and different
  // values of OutputOnlySummedEventWorkspace
  void testLoadSameDataTwice2() {
    outputSpace = "LoadMcStasTestLoadSameDataTwice2";
    auto outputGroup = load_test("mccode_multiple_scattering.h5", outputSpace, true);
    TS_ASSERT_EQUALS(outputGroup->getNumberOfEntries(), 1);

    outputGroup = load_test("mccode_multiple_scattering.h5", outputSpace);
    TS_ASSERT_EQUALS(outputGroup->getNumberOfEntries(), 3);
  }

private:
  double extractSumAndTest(const MatrixWorkspace_sptr &workspace, const double &expectedSum) {
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 8192);
    auto sum = 0.0;
    for (auto i = 0u; i < workspace->getNumberHistograms(); ++i)
      sum += workspace->y(i)[0];
    sum *= 1.0e22;
    TS_ASSERT_DELTA(sum, expectedSum, 0.0001);
    return sum;
  }

  std::shared_ptr<WorkspaceGroup> load_test(const std::string &fileName, const std::string &outputName,
                                            bool summed = false) {
    LoadMcStas algToBeTested;
    algToBeTested.initialize();
    algToBeTested.setProperty("OutputWorkspace", outputName);
    algToBeTested.setPropertyValue("OutputOnlySummedEventWorkspace", boost::lexical_cast<std::string>(summed));
    algToBeTested.setProperty("Filename", fileName);

    // mark the temp file to be deleted upon end of execution
    { // limit variable scope
      std::string tempFile = algToBeTested.getPropertyValue("Filename");
      tempFile = tempFile.substr(0, tempFile.size() - 2) + "vtp";
      Poco::TemporaryFile::registerForDeletion(tempFile);
    }
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    auto outputGroup(AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputName));
    return outputGroup;
  }

  std::string inputFile;
  std::string outputSpace;
};

class LoadMcStasTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadMcStasTestPerformance *createSuite() { return new LoadMcStasTestPerformance(); }

  static void destroySuite(LoadMcStasTestPerformance *suite) { delete suite; }

  void setUp() override {
    loadFile.initialize();
    loadFile.setProperty("Filename", "mcstas_event_hist.h5");
    loadFile.setProperty("OutputWorkspace", "outputWS");
  }

  void tearDown() override { AnalysisDataService::Instance().remove("outputWS"); }

  void testDefaultLoad() { loadFile.execute(); }

private:
  LoadMcStas loadFile;
};
