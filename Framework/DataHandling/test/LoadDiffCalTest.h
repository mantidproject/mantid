// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADDIFFCALTEST_H_
#define MANTID_DATAHANDLING_LOADDIFFCALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataHandling/LoadDiffCal.h"
// reuse what another test has for creating dummy workspaces
#include "MantidDataHandling/SaveDetectorsGrouping.h"
#include "SaveDiffCalTest.h"

#include <Poco/File.h>

using Mantid::DataHandling::LoadDiffCal;
using Mantid::DataHandling::SaveDiffCal;
using namespace Mantid::API;

class LoadDiffCalTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadDiffCalTest *createSuite() { return new LoadDiffCalTest(); }
  static void destroySuite(LoadDiffCalTest *suite) { delete suite; }

  void test_Init() {
    LoadDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_exec() {
    // this is a round-trip test
    std::string outWSName("LoadDiffCalTest");
    std::string filename("LoadDiffCalTest.h5");

    // save a test file
    SaveDiffCalTest saveDiffCal;
    auto inst = saveDiffCal.createInstrument();
    auto groupWSIn = saveDiffCal.createGrouping(inst);
    auto maskWSIn = saveDiffCal.createMasking(inst);
    auto calWSIn =
        saveDiffCal.createCalibration(5 * 9); // nine components per bank
    SaveDiffCal saveAlg;
    saveAlg.initialize();
    saveAlg.setProperty("GroupingWorkspace", groupWSIn);
    saveAlg.setProperty("MaskWorkspace", maskWSIn);
    saveAlg.setProperty("Filename", filename);
    saveAlg.setProperty("CalibrationWorkspace", calWSIn);
    TS_ASSERT_THROWS_NOTHING(saveAlg.execute();); // make sure it runs
    filename = saveAlg.getPropertyValue("Filename");

    // run the algorithm of interest
    LoadDiffCal loadAlg;
    TS_ASSERT_THROWS_NOTHING(loadAlg.initialize());
    TS_ASSERT(loadAlg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loadAlg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loadAlg.setPropertyValue("WorkspaceName", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        loadAlg.setProperty("MakeGroupingWorkspace", false));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("MakeMaskWorkspace", false));
    TS_ASSERT_THROWS_NOTHING(loadAlg.execute(););
    TS_ASSERT(loadAlg.isExecuted());

    ITableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            outWSName + "_cal"));
    TS_ASSERT(ws);

    if (ws) {
      auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
      checkAlg->setProperty("Workspace1", calWSIn);
      checkAlg->setProperty("Workspace2", ws);
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));

      AnalysisDataService::Instance().remove(outWSName + "_cal");
    }

    // cleanup
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void test_override_grouping() {
    // this is a round-trip test
    std::string outWSName("LoadDiffCalTest");
    std::string filename("LoadDiffCalTest.h5");
    std::string groupingfile("LoadDiffCalTest_grp.xml");

    // save a test file
    SaveDiffCalTest saveDiffCal;
    auto inst = saveDiffCal.createInstrument();
    auto groupWSIn = saveDiffCal.createGrouping(inst, false);
    auto maskWSIn = saveDiffCal.createMasking(inst);
    auto calWSIn =
        saveDiffCal.createCalibration(5 * 9); // nine components per bank
    SaveDiffCal saveAlg;
    saveAlg.initialize();
    saveAlg.setProperty("GroupingWorkspace", groupWSIn);
    saveAlg.setProperty("MaskWorkspace", maskWSIn);
    saveAlg.setProperty("Filename", filename);
    saveAlg.setProperty("CalibrationWorkspace", calWSIn);
    TS_ASSERT_THROWS_NOTHING(saveAlg.execute();); // make sure it runs
    filename = saveAlg.getPropertyValue("Filename");

    // create the overriding grouping file
    groupWSIn = saveDiffCal.createGrouping(inst, true);
    SaveDetectorsGrouping saveGrouping;
    saveGrouping.initialize();
    saveGrouping.setProperty("InputWorkspace", groupWSIn);
    saveGrouping.setProperty("OutputFile", groupingfile);
    TS_ASSERT_THROWS_NOTHING(saveGrouping.execute();); // make sure it runs
    groupingfile = saveGrouping.getPropertyValue("OutputFile");

    // run the algorithm of interest
    LoadDiffCal loadAlg;
    TS_ASSERT_THROWS_NOTHING(loadAlg.initialize());
    TS_ASSERT(loadAlg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("InputWorkspace", groupWSIn));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(
        loadAlg.setPropertyValue("GroupFilename", groupingfile));
    TS_ASSERT_THROWS_NOTHING(
        loadAlg.setPropertyValue("WorkspaceName", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        loadAlg.setProperty("MakeGroupingWorkspace", true));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("MakeMaskWorkspace", false));
    TS_ASSERT_THROWS_NOTHING(loadAlg.execute(););
    TS_ASSERT(loadAlg.isExecuted());

    ITableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            outWSName + "_cal"));
    TS_ASSERT(ws);

    if (ws) {
      auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
      checkAlg->setProperty("Workspace1", calWSIn);
      checkAlg->setProperty("Workspace2", ws);
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));

      AnalysisDataService::Instance().remove(outWSName + "_cal");
    }

    // cleanup
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
    if (Poco::File(groupingfile).exists())
      Poco::File(groupingfile).remove();
  }
};

#endif /* MANTID_DATAHANDLING_LOADDIFFCALTEST_H_ */
