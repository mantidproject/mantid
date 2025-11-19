// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <filesystem>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataHandling/LoadDiffCal.h"
// reuse what another test has for creating dummy workspaces
#include "MantidDataHandling/SaveDetectorsGrouping.h"
#include "SaveDiffCalTest.h"

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
    auto calWSIn = saveDiffCal.createCalibration(5 * 9); // nine components per bank
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
    TS_ASSERT_THROWS_NOTHING(loadAlg.setPropertyValue("WorkspaceName", outWSName));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("MakeGroupingWorkspace", false));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("MakeMaskWorkspace", false));
    TS_ASSERT_THROWS_NOTHING(loadAlg.execute(););
    TS_ASSERT(loadAlg.isExecuted());

    ITableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outWSName + "_cal"));
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
    if (std::filesystem::exists(filename))
      std::filesystem::remove(filename);
  }

  // Override a grouping definition specified by LoadDiffCal "Filename" property.
  // Use a grouping definition from an XML-formatted file specified by LoadDiffCal "GroupFilename" property.
  void test_alternate_grouping_definition_xml_format() {
    std::string outWSName("LoadDiffCalTest");
    std::string filename("LoadDiffCalTest.h5");
    std::string groupingfile("LoadDiffCalTest_grp.xml");

    // save a test file
    SaveDiffCalTest saveDiffCal;
    auto inst = saveDiffCal.createInstrument();
    auto groupWSIn = saveDiffCal.createGrouping(inst, false);
    auto maskWSIn = saveDiffCal.createMasking(inst);
    auto calWSIn = saveDiffCal.createCalibration(5 * 9); // nine components per bank

    SaveDiffCal saveAlg;
    saveAlg.initialize();
    saveAlg.setProperty("GroupingWorkspace", groupWSIn);
    saveAlg.setProperty("MaskWorkspace", maskWSIn);
    saveAlg.setProperty("Filename", filename);
    saveAlg.setProperty("CalibrationWorkspace", calWSIn);
    TS_ASSERT_THROWS_NOTHING(saveAlg.execute();); // make sure it runs
    filename = saveAlg.getPropertyValue("Filename");

    // create the overriding grouping workspace
    groupWSIn = saveDiffCal.createGrouping(inst, true);

    // create a file from the grouping workspace
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
    TS_ASSERT_THROWS_NOTHING(loadAlg.setPropertyValue("GroupFilename", groupingfile));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setPropertyValue("WorkspaceName", outWSName));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("MakeGroupingWorkspace", true));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("MakeMaskWorkspace", false));
    TS_ASSERT_THROWS_NOTHING(loadAlg.execute(););
    TS_ASSERT(loadAlg.isExecuted());

    // Verify that the loaded calibration workspace is the same as the input calibration workspace.
    ITableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outWSName + "_cal"));
    TS_ASSERT(ws);

    if (ws) {
      auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
      checkAlg->setProperty("Workspace1", calWSIn);
      checkAlg->setProperty("Workspace2", ws);
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));

      AnalysisDataService::Instance().remove(outWSName + "_cal");
    }

    // Verify that the loaded grouping workspace is the same as the overriding grouping workspace.
    GroupingWorkspace_sptr groupWSOut;
    TS_ASSERT_THROWS_NOTHING(groupWSOut =
                                 AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(outWSName + "_group"));
    TS_ASSERT(groupWSOut);

    if (groupWSOut) {
      auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
      checkAlg->setProperty("Workspace1", groupWSIn);
      checkAlg->setProperty("Workspace2", groupWSOut);
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));

      AnalysisDataService::Instance().remove(outWSName + "_group");
    }

    // cleanup
    if (std::filesystem::exists(filename))
      std::filesystem::remove(filename);
    if (std::filesystem::exists(groupingfile))
      std::filesystem::remove(groupingfile);
  }

  // Create a zero calibration workspace consistent with an input grouping workspace.
  TableWorkspace_sptr createZeroCalibration(GroupingWorkspace_const_sptr groupingWS) {
    auto calibrationWS = std::make_shared<Mantid::DataObjects::TableWorkspace>();
    calibrationWS->addColumn("int", "detid");
    calibrationWS->addColumn("double", "difc");
    calibrationWS->addColumn("double", "difa");
    calibrationWS->addColumn("double", "tzero");
    calibrationWS->addColumn("double", "tofmin");

    std::vector<int> groupIDs = groupingWS->getGroupIDs();
    for (size_t i = 0; i < groupIDs.size(); ++i) {
      std::vector<int> detIDs = groupingWS->getDetectorIDsOfGroup(groupIDs[i]);
      for (size_t j = 0; j < detIDs.size(); j++) {
        Mantid::API::TableRow row = calibrationWS->appendRow();
        row << detIDs[j] // detid
            << 0.        // difc
            << 0.        // difa
            << 0.        // tzero
            << 0.;       // tofmin
      }
    }

    return calibrationWS;
  }

  // Override a grouping definition specified by LoadDiffCal "Filename" property.
  // Use a grouping definition from an HDF-formatted file specified by LoadDiffCal "GroupFilename" property.
  void test_alternate_grouping_definition_hdf_format() {
    std::string outWSName("LoadDiffCalTest");
    std::string filename("LoadDiffCalTest.h5");
    // intentionally giving groupingfile a mixed-case file name extension to test the robustness of LoadDiffCal file
    // name validation
    std::string groupingfile("LoadDiffCalTest_grp.HdF");

    // Create ingredients for a test calibration file.
    SaveDiffCalTest saveDiffCal;
    auto inst = saveDiffCal.createInstrument();
    auto groupWSIn = saveDiffCal.createGrouping(inst, false);
    auto maskWSIn = saveDiffCal.createMasking(inst);
    auto calWSIn = saveDiffCal.createCalibration(5 * 9); // nine components per bank

    // Save a test calibration file
    SaveDiffCal saveAlg;
    saveAlg.initialize();
    saveAlg.setProperty("CalibrationWorkspace", calWSIn);
    saveAlg.setProperty("GroupingWorkspace", groupWSIn);
    saveAlg.setProperty("MaskWorkspace", maskWSIn);
    saveAlg.setProperty("Filename", filename);    // path to the file to be created by SaveDiffCal
    TS_ASSERT_THROWS_NOTHING(saveAlg.execute();); // make sure it runs
    filename = saveAlg.getPropertyValue("Filename");

    // Now create a new grouping definition which is supposed to override the previous one.
    groupWSIn = saveDiffCal.createGrouping(inst, true);

    // Save the new grouping definition in HDF format. Since SaveDiffCal requires an input calibration
    // workspace, create a zero calibration workspace to serve as a place holder consistent with the grouping workspace.
    auto zeroCalWSIn = createZeroCalibration(groupWSIn);

    saveAlg.initialize();
    saveAlg.setProperty("CalibrationWorkspace", zeroCalWSIn);
    saveAlg.setProperty("GroupingWorkspace", groupWSIn);
    saveAlg.setProperty("Filename", groupingfile); // path to the file to be created by SaveDiffCal
    TS_ASSERT_THROWS_NOTHING(saveAlg.execute(););

    // Run the algorithm of interest
    LoadDiffCal loadAlg;
    TS_ASSERT_THROWS_NOTHING(loadAlg.initialize());
    TS_ASSERT(loadAlg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("InputWorkspace", groupWSIn)); // workspace to take instrument from
    TS_ASSERT_THROWS_NOTHING(
        loadAlg.setPropertyValue("Filename", filename)); // file with original calibration and grouping workspaces
    TS_ASSERT_THROWS_NOTHING(loadAlg.setPropertyValue("GroupFilename", groupingfile)); // overriding grouping definition
    TS_ASSERT_THROWS_NOTHING(
        loadAlg.setPropertyValue("WorkspaceName", outWSName)); // prefix for the output calibration workspace names
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("MakeGroupingWorkspace", true));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("MakeMaskWorkspace", false));
    TS_ASSERT_THROWS_NOTHING(loadAlg.execute(););
    TS_ASSERT(loadAlg.isExecuted());

    // Verify that the loaded calibration workspace is the same as the input calibration workspace.
    ITableWorkspace_sptr calWSOut;
    TS_ASSERT_THROWS_NOTHING(calWSOut =
                                 AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outWSName + "_cal"));
    TS_ASSERT(calWSOut);

    if (calWSOut) {
      auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
      checkAlg->setProperty("Workspace1", calWSIn);
      checkAlg->setProperty("Workspace2", calWSOut);
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));

      AnalysisDataService::Instance().remove(outWSName + "_cal");
    }

    // Verify that the loaded grouping workspace is the same as the overriding grouping workspace.
    GroupingWorkspace_sptr groupWSOut;
    TS_ASSERT_THROWS_NOTHING(groupWSOut =
                                 AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>(outWSName + "_group"));
    TS_ASSERT(groupWSOut);

    if (groupWSOut) {
      auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
      checkAlg->setProperty("Workspace1", groupWSIn);
      checkAlg->setProperty("Workspace2", groupWSOut);
      checkAlg->execute();
      TS_ASSERT(checkAlg->getProperty("Result"));

      AnalysisDataService::Instance().remove(outWSName + "_group");
    }

    // cleanup
    if (std::filesystem::exists(filename))
      std::filesystem::remove(filename);
    if (std::filesystem::exists(groupingfile))
      std::filesystem::remove(groupingfile);
  }
};
