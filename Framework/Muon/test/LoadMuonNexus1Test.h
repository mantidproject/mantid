// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this
// test case.
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidMuon/LoadMuonNexus1.h"

#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class LoadMuonNexus1Test : public CxxTest::TestSuite {
public:
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(nxLoad.initialize());
    TS_ASSERT(nxLoad.isInitialized());
  }

  void testExec() {
    if (!nxLoad.isInitialized())
      nxLoad.initialize();
    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(nxLoad.execute(), const std::runtime_error &);

    // Now set required filename and output workspace name
    inputFile = "emu00006473.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());

    // Test additional output parameters
    std::string field = nxLoad.getProperty("MainFieldDirection");
    TS_ASSERT(field == "Longitudinal");

    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 32 for file inputFile =
    // "../../../../Test/Nexus/emu00006473.nxs";
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 32);
    // Check two X vectors are the same
    TS_ASSERT((output2D->x(3)) == (output2D->x(31)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->y(5).size(), output2D->y(17).size());
    // Check one particular value
    TS_ASSERT_EQUALS(output2D->y(11)[686], 81);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->e(11)[686], 9);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA(output2D->x(11)[687], 10.738, 0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output->isDistribution());

    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check Child Algorithm is running
    // properly
    //----------------------------------------------------------------------
    Property *l_property = output->run().getLogData(std::string("beamlog_current"));
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double> *>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 27), "2006-Nov-21 07:03:08  182.8");
    // check that sample name has been set correctly
    TS_ASSERT_EQUALS(output->sample().getName(), "Cr2.7Co0.3Si");

    // check that the main field direction has been added as a log
    Property *fieldDirection = output->run().getLogData("main_field_direction");
    TS_ASSERT(fieldDirection);
    TS_ASSERT_EQUALS(fieldDirection->value(), "Longitudinal");
  }

  void testTransverseDataset() {
    LoadMuonNexus1 nxL;
    if (!nxL.isInitialized())
      nxL.initialize();

    // Now set required filename and output workspace name
    std::string inputFile_musr00022725 = "MUSR00022725.nxs";
    nxL.setPropertyValue("FileName", inputFile_musr00022725);

    outputSpace = "outermusr00022725";
    nxL.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(nxL.execute());
    TS_ASSERT(nxL.isExecuted());

    // Test additional output parameters
    std::string field = nxL.getProperty("MainFieldDirection");
    TS_ASSERT(field == "Transverse");
    double timeZero = nxL.getProperty("TimeZero");
    TS_ASSERT_DELTA(timeZero, 0.55, 0.001);
    double firstGood = nxL.getProperty("FirstGoodData");
    TS_ASSERT_DELTA(firstGood, 0.656, 0.001);
    double lastGood = nxL.getProperty("LastGoodData");
    TS_ASSERT_DELTA(lastGood, 32.0, 0.001);

    // Test that the output workspace knows the field direction
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    Property *fieldDirection = output->run().getLogData("main_field_direction");
    TS_ASSERT(fieldDirection);
    TS_ASSERT_EQUALS(fieldDirection->value(), "Transverse");
  }

  void testExec2() {
    // test for multi period
    // Now set required filename and output workspace name
    inputFile2 = "emu00006475.nxs";
    nxLoad.setPropertyValue("FileName", inputFile2);

    outputSpace = "outer2";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("EntryNumber", "1");
    int64_t entryNumber = nxLoad.getProperty("EntryNumber");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());
    //
    // Test workspace data - should be 4 separate workspaces for this 4 period
    // file
    //
    if (entryNumber == 0) {
      WorkspaceGroup_sptr outGrp;
      TS_ASSERT_THROWS_NOTHING(outGrp = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace));
    }
    // if entry number is given
    if (entryNumber == 1) {
      MatrixWorkspace_sptr output;
      output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);

      Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
      // Workspace2D_sptr output2D2 =
      // std::dynamic_pointer_cast<Workspace2D>(output2);
      // Should be 32 for file inputFile =
      // "../../../../Test/Nexus/emu00006475.nxs";
      TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 32);
      // Check two X vectors are the same
      TS_ASSERT((output2D->x(3)) == (output2D->x(31)));
      // Check two Y arrays have the same number of elements
      TS_ASSERT_EQUALS(output2D->y(5).size(), output2D->y(17).size());
      // Check that the time is as expected from bin boundary update
      TS_ASSERT_DELTA(output2D->x(11)[687], 10.738, 0.001);

      // Check the unit has been set correctly
      TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
      TS_ASSERT(!output->isDistribution());

      // check that sample name has been set correctly
      TS_ASSERT_EQUALS(output->sample().getName(), "ptfe test");
    }
    MatrixWorkspace_sptr output, output2, output3, output4;
    WorkspaceGroup_sptr outGrp;
    // if no entry number load the group workspace
    if (entryNumber == 0) {
      TS_ASSERT_THROWS_NOTHING(outGrp = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace));

      (output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_1"));
      (output2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_2"));
      (output3 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_3"));
      (output4 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_4"));

      Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
      Workspace2D_sptr output2D2 = std::dynamic_pointer_cast<Workspace2D>(output2);
      // Should be 32 for file inputFile =
      // "../../../../Test/Nexus/emu00006475.nxs";
      TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 32);
      // Check two X vectors are the same
      TS_ASSERT((output2D->x(3)) == (output2D->x(31)));
      // Check two Y arrays have the same number of elements
      TS_ASSERT_EQUALS(output2D->y(5).size(), output2D->y(17).size());
      // Check one particular value
      TS_ASSERT_EQUALS(output2D2->y(8)[502], 121);
      // Check that the error on that value is correct
      TS_ASSERT_EQUALS(output2D2->e(8)[502], 11);
      // Check that the time is as expected from bin boundary update
      TS_ASSERT_DELTA(output2D->x(11)[687], 10.738, 0.001);

      // Check the unit has been set correctly
      TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
      TS_ASSERT(!output->isDistribution());

      // check that sample name has been set correctly
      TS_ASSERT_EQUALS(output->sample().getName(), output2->sample().getName());
      TS_ASSERT_EQUALS(output->sample().getName(), "ptfe test");
    }
  }
  void testExec2withZeroEntryNumber() {
    // test for multi period
    // Now set required filename and output workspace name
    inputFile2 = "emu00006475.nxs";
    nxLoad.setPropertyValue("FileName", inputFile2);

    outputSpace = "outer2";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("EntryNumber", "0");
    int64_t entryNumber = nxLoad.getProperty("EntryNumber");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());
    //
    // Test workspace data - should be 4 separate workspaces for this 4 period
    // file
    //
    WorkspaceGroup_sptr outGrp;
    TS_ASSERT_THROWS_NOTHING(outGrp = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace));

    MatrixWorkspace_sptr output, output2, output3, output4;
    // WorkspaceGroup_sptr outGrp;
    // if no entry number load the group workspace
    if (entryNumber == 0) {
      (output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_1"));
      (output2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_2"));
      (output3 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_3"));
      (output4 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_4"));

      Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
      Workspace2D_sptr output2D2 = std::dynamic_pointer_cast<Workspace2D>(output2);
      // Should be 32 for file inputFile =
      // "../../../../Test/Nexus/emu00006475.nxs";
      TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 32);
      // Check two X vectors are the same
      TS_ASSERT((output2D->x(3)) == (output2D->x(31)));
      // Check two Y arrays have the same number of elements
      TS_ASSERT_EQUALS(output2D->y(5).size(), output2D->y(17).size());
      // Check one particular value
      TS_ASSERT_EQUALS(output2D2->y(8)[502], 121);
      // Check that the error on that value is correct
      TS_ASSERT_EQUALS(output2D2->e(8)[502], 11);
      // Check that the time is as expected from bin boundary update
      TS_ASSERT_DELTA(output2D->x(11)[687], 10.738, 0.001);

      // Check the unit has been set correctly
      TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
      TS_ASSERT(!output->isDistribution());

      // check that sample name has been set correctly
      TS_ASSERT_EQUALS(output->sample().getName(), output2->sample().getName());
      TS_ASSERT_EQUALS(output->sample().getName(), "ptfe test");
    }
  }

  void testarrayin() {
    if (!nxload3.isInitialized())
      nxload3.initialize();

    nxload3.setPropertyValue("Filename", inputFile);
    nxload3.setPropertyValue("OutputWorkspace", "outWS");
    nxload3.setPropertyValue("SpectrumList", "29,30,32");
    nxload3.setPropertyValue("SpectrumMin", "5");
    nxload3.setPropertyValue("SpectrumMax", "10");

    TS_ASSERT_THROWS_NOTHING(nxload3.execute());
    TS_ASSERT(nxload3.isExecuted());

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    (output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    // Should be 6 for selected input
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 9);

    // Check two X vectors are the same
    TS_ASSERT((output2D->x(1)) == (output2D->x(5)));

    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->y(2).size(), output2D->y(7).size());

    // Check one particular value
    TS_ASSERT_EQUALS(output2D->y(8)[479], 144);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->e(8)[479], 12);
    // Check that the error on that value is correct
    TS_ASSERT_DELTA(output2D->x(8)[479], 7.410, 0.0001);
  }

  void testPartialSpectraLoading() {
    LoadMuonNexus1 alg1;
    LoadMuonNexus1 alg2;
    inputFile = "emu00006473.nxs";

    const std::string deadTimeWSName = "LoadMuonNexus1Test_DeadTimes";
    const std::string groupingWSName = "LoadMuonNexus1Test_Grouping";

    // Execute alg1
    // It will only load some spectra
    TS_ASSERT_THROWS_NOTHING(alg1.initialize());
    TS_ASSERT(alg1.isInitialized());
    alg1.setPropertyValue("Filename", inputFile);
    alg1.setPropertyValue("OutputWorkspace", "outWS1");
    alg1.setPropertyValue("SpectrumList", "29,31");
    alg1.setPropertyValue("SpectrumMin", "5");
    alg1.setPropertyValue("SpectrumMax", "10");
    alg1.setPropertyValue("DeadTimeTable", deadTimeWSName);
    alg1.setPropertyValue("DetectorGroupingTable", groupingWSName);
    TS_ASSERT_THROWS_NOTHING(alg1.execute());
    TS_ASSERT(alg1.isExecuted());
    // Get back the saved workspace
    MatrixWorkspace_sptr output1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS1");
    Workspace2D_sptr out1 = std::dynamic_pointer_cast<Workspace2D>(output1);

    // Execute alg2
    // Load all the spectra
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());
    alg2.setPropertyValue("Filename", inputFile);
    alg2.setPropertyValue("OutputWorkspace", "outWS2");
    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg2.isExecuted());
    // Get back the saved workspace
    MatrixWorkspace_sptr output2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS2");
    Workspace2D_sptr out2 = std::dynamic_pointer_cast<Workspace2D>(output2);

    // Check common spectra
    // X values should match
    TS_ASSERT_EQUALS(out1->x(0), out2->x(0));
    TS_ASSERT_EQUALS(out1->x(4), out2->x(5));
    // Check some Y values
    TS_ASSERT_EQUALS(out1->y(0), out2->y(4));
    TS_ASSERT_EQUALS(out1->y(3), out2->y(7));
    TS_ASSERT_EQUALS(out1->y(5), out2->y(9));
    TS_ASSERT_EQUALS(out1->y(6), out2->y(28));
    TS_ASSERT_EQUALS(out1->y(7), out2->y(30));
    // Check some E values
    TS_ASSERT_EQUALS(out1->e(0), out2->e(4));
    TS_ASSERT_EQUALS(out1->e(3), out2->e(7));
    TS_ASSERT_EQUALS(out1->e(5), out2->e(9));
    TS_ASSERT_EQUALS(out1->e(6), out2->e(28));
    TS_ASSERT_EQUALS(out1->e(7), out2->e(30));

    AnalysisDataService::Instance().remove("outWS1");
    AnalysisDataService::Instance().remove("outWS2");

    // Check dead time table
    TableWorkspace_sptr deadTimeTable;
    TS_ASSERT_THROWS_NOTHING(deadTimeTable =
                                 AnalysisDataService::Instance().retrieveWS<TableWorkspace>(deadTimeWSName));
    TS_ASSERT(deadTimeTable);
    // Check number of rows and columns
    TS_ASSERT_EQUALS(deadTimeTable->columnCount(), 2);
    TS_ASSERT_EQUALS(deadTimeTable->rowCount(), 8);
    // Check spectrum numbers
    TS_ASSERT_EQUALS(deadTimeTable->Int(0, 0), 5);
    TS_ASSERT_EQUALS(deadTimeTable->Int(4, 0), 9);
    TS_ASSERT_EQUALS(deadTimeTable->Int(7, 0), 31);
    // Check dead time values
    TS_ASSERT_DELTA(deadTimeTable->Double(0, 1), 0.00161112, 0.00000001);
    TS_ASSERT_DELTA(deadTimeTable->Double(3, 1), 0.00431686, 0.00000001);
    TS_ASSERT_DELTA(deadTimeTable->Double(6, 1), 0.00254914, 0.00000001);
    AnalysisDataService::Instance().remove(deadTimeWSName);

    // Check detector grouping table
    TableWorkspace_sptr groupingTable;
    TS_ASSERT_THROWS_NOTHING(groupingTable =
                                 AnalysisDataService::Instance().retrieveWS<TableWorkspace>(groupingWSName));
    TS_ASSERT(groupingTable);
    // Check number of rows and columns
    TS_ASSERT_EQUALS(groupingTable->columnCount(), 1);
    TS_ASSERT_EQUALS(groupingTable->rowCount(), 2);
    // Check grouping
    std::vector<int> testVec;
    for (int i = 5; i < 11; i++)
      testVec.emplace_back(i);
    TS_ASSERT_EQUALS(groupingTable->cell<std::vector<int>>(0, 0), testVec);
    testVec.clear();
    testVec.emplace_back(29);
    testVec.emplace_back(31);
    TS_ASSERT_EQUALS(groupingTable->cell<std::vector<int>>(1, 0), testVec);
    AnalysisDataService::Instance().remove(groupingWSName);
  }

  void testPartialSpectraLoading_spectrumNumbers_detectorIDs() {
    LoadMuonNexus1 alg;

    // It will only load some spectra
    try {
      alg.initialize();
      alg.setChild(true);
      alg.setPropertyValue("Filename", "emu00006473.nxs");
      alg.setPropertyValue("OutputWorkspace", "__NotUsed");
      alg.setPropertyValue("SpectrumList", "29,31");
      alg.setPropertyValue("SpectrumMin", "5");
      alg.setPropertyValue("SpectrumMax", "10");
      alg.execute();
    } catch (const std::exception &ex) {
      TS_FAIL(std::string("Loading failed: ") + ex.what());
    }

    Workspace_sptr outWS = alg.getProperty("OutputWorkspace");
    const auto loadedWS = std::dynamic_pointer_cast<Workspace2D>(outWS);
    TS_ASSERT(loadedWS);

    // Check the right spectra have been loaded
    const std::vector<Mantid::specnum_t> expectedSpectra{5, 6, 7, 8, 9, 10, 29, 31};
    TS_ASSERT_EQUALS(loadedWS->getNumberHistograms(), expectedSpectra.size());
    for (size_t i = 0; i < loadedWS->getNumberHistograms(); ++i) {
      const auto spec = loadedWS->getSpectrum(i);
      TS_ASSERT_EQUALS(spec.getSpectrumNo(), expectedSpectra[i]);
      // detector ID = spectrum number for muon Nexus v1
      const auto detIDs = spec.getDetectorIDs();
      TS_ASSERT_EQUALS(detIDs.size(), 1);
      TS_ASSERT_EQUALS(*detIDs.begin(), static_cast<int>(spec.getSpectrumNo()));
    }
  }

  void test_loadingDeadTimes_singlePeriod() {
    const std::string outWSName = "LoadMuonNexus1Test_OutputWS";
    const std::string deadTimesWSName = "LoadMuonNexus1Test_DeadTimes";

    LoadMuonNexus1 alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "emu00006473.nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DeadTimeTable", deadTimesWSName));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr deadTimesTable;

    TS_ASSERT_THROWS_NOTHING(deadTimesTable =
                                 AnalysisDataService::Instance().retrieveWS<TableWorkspace>(deadTimesWSName));

    TS_ASSERT(deadTimesTable);

    if (deadTimesTable) {
      TS_ASSERT_EQUALS(deadTimesTable->columnCount(), 2);
      TS_ASSERT_EQUALS(deadTimesTable->rowCount(), 32);

      TS_ASSERT_EQUALS(deadTimesTable->Int(0, 0), 1);
      TS_ASSERT_EQUALS(deadTimesTable->Int(15, 0), 16);
      TS_ASSERT_EQUALS(deadTimesTable->Int(31, 0), 32);

      TS_ASSERT_DELTA(deadTimesTable->Double(0, 1), 0.00172168, 0.00000001);
      TS_ASSERT_DELTA(deadTimesTable->Double(15, 1), -0.00163397, 0.00000001);
      TS_ASSERT_DELTA(deadTimesTable->Double(31, 1), -0.03767336, 0.00000001);
    }

    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove(deadTimesWSName);
  }

  void test_loadingDeadTimes_multiPeriod() {

    const std::string outWSName = "LoadMuonNexus1Test_OutputWS";
    const std::string deadTimesWSName = "LoadMuonNexus1Test_DeadTimes";

    LoadMuonNexus1 alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "MUSR00015189.nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DeadTimeTable", deadTimesWSName));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WorkspaceGroup_sptr deadTimesGroup;

    TS_ASSERT_THROWS_NOTHING(deadTimesGroup =
                                 AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(deadTimesWSName));

    TS_ASSERT(deadTimesGroup);

    if (deadTimesGroup) {
      TS_ASSERT_EQUALS(deadTimesGroup->size(), 2);

      TableWorkspace_sptr table1 = std::dynamic_pointer_cast<TableWorkspace>(deadTimesGroup->getItem(0));
      TS_ASSERT(table1);

      if (table1) {
        TS_ASSERT_EQUALS(table1->columnCount(), 2);
        TS_ASSERT_EQUALS(table1->rowCount(), 64);

        TS_ASSERT_EQUALS(table1->Int(0, 0), 1);
        TS_ASSERT_EQUALS(table1->Int(31, 0), 32);
        TS_ASSERT_EQUALS(table1->Int(63, 0), 64);

        TS_ASSERT_DELTA(table1->Double(0, 1), 0.01285629, 0.00000001);
        TS_ASSERT_DELTA(table1->Double(31, 1), 0.01893649, 0.00000001);
        TS_ASSERT_DELTA(table1->Double(63, 1), 0.01245339, 0.00000001);
      }

      TableWorkspace_sptr table2 = std::dynamic_pointer_cast<TableWorkspace>(deadTimesGroup->getItem(1));
      TS_ASSERT(table2);

      if (table2) {
        TS_ASSERT_EQUALS(table2->columnCount(), 2);
        TS_ASSERT_EQUALS(table2->rowCount(), 64);

        TS_ASSERT_EQUALS(table2->Int(0, 0), 1);
        TS_ASSERT_EQUALS(table2->Int(31, 0), 32);
        TS_ASSERT_EQUALS(table2->Int(63, 0), 64);

        TS_ASSERT_DELTA(table2->Double(0, 1), 0.01285629, 0.00000001);
        TS_ASSERT_DELTA(table2->Double(31, 1), 0.01893649, 0.00000001);
        TS_ASSERT_DELTA(table2->Double(63, 1), 0.01245339, 0.00000001);
      }
    }

    AnalysisDataService::Instance().deepRemoveGroup(outWSName);
    AnalysisDataService::Instance().deepRemoveGroup(deadTimesWSName);
  }

  void test_loadingDetectorGrouping_singlePeriod() {
    const std::string outWSName = "LoadMuonNexus1Test_OutputWS";
    const std::string detectorGroupingWSName = "LoadMuonNexus1Test_DetectorGrouping";

    LoadMuonNexus1 alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "emu00006473.nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DetectorGroupingTable", detectorGroupingWSName));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TableWorkspace_sptr detectorGrouping;

    TS_ASSERT_THROWS_NOTHING(detectorGrouping =
                                 AnalysisDataService::Instance().retrieveWS<TableWorkspace>(detectorGroupingWSName));

    TS_ASSERT(detectorGrouping);

    if (detectorGrouping) {
      TS_ASSERT_EQUALS(detectorGrouping->columnCount(), 1);
      TS_ASSERT_EQUALS(detectorGrouping->rowCount(), 2);

      TS_ASSERT_EQUALS(detectorGrouping->getColumn(0)->type(), "vector_int");
      TS_ASSERT_EQUALS(detectorGrouping->getColumn(0)->name(), "Detectors");

      std::vector<int> e1, e2;
      TS_ASSERT_THROWS_NOTHING(e1 = detectorGrouping->cell<std::vector<int>>(0, 0));
      TS_ASSERT_THROWS_NOTHING(e2 = detectorGrouping->cell<std::vector<int>>(1, 0));

      TS_ASSERT_EQUALS(e1.size(), 16);
      TS_ASSERT_EQUALS(e2.size(), 16);

      TS_ASSERT_EQUALS(e1[0], 1);
      TS_ASSERT_EQUALS(e1[15], 16);

      TS_ASSERT_EQUALS(e2[0], 17);
      TS_ASSERT_EQUALS(e2[15], 32);
    }

    AnalysisDataService::Instance().remove(outWSName);
    AnalysisDataService::Instance().remove(detectorGroupingWSName);
  }

  void test_loadingDetectorGrouping_multiPeriod() {
    const std::string outWSName = "LoadMuonNexus1Test_OutputWS";
    const std::string detectorGroupingWSName = "LoadMuonNexus1Test_DetectorGrouping";

    LoadMuonNexus1 alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "MUSR00015189.nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DetectorGroupingTable", detectorGroupingWSName));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WorkspaceGroup_sptr detectorGrouping;

    TS_ASSERT_THROWS_NOTHING(detectorGrouping =
                                 AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(detectorGroupingWSName));

    TS_ASSERT(detectorGrouping);

    if (detectorGrouping) {
      TS_ASSERT_EQUALS(detectorGrouping->size(), 2);

      TableWorkspace_sptr table1 = std::dynamic_pointer_cast<TableWorkspace>(detectorGrouping->getItem(0));
      TS_ASSERT(table1);

      if (table1) {
        TS_ASSERT_EQUALS(table1->columnCount(), 1);
        TS_ASSERT_EQUALS(table1->rowCount(), 2);

        std::vector<int> e1, e2;
        TS_ASSERT_THROWS_NOTHING(e1 = table1->cell<std::vector<int>>(0, 0));
        TS_ASSERT_THROWS_NOTHING(e2 = table1->cell<std::vector<int>>(1, 0));

        TS_ASSERT_EQUALS(e1.size(), 32);
        TS_ASSERT_EQUALS(e2.size(), 32);

        TS_ASSERT_EQUALS(e1[0], 33);
        TS_ASSERT_EQUALS(e1[31], 64);

        TS_ASSERT_EQUALS(e2[0], 1);
        TS_ASSERT_EQUALS(e2[31], 32);
      }

      TableWorkspace_sptr table2 = std::dynamic_pointer_cast<TableWorkspace>(detectorGrouping->getItem(1));
      TS_ASSERT(table2);

      if (table2) {
        TS_ASSERT_EQUALS(table2->columnCount(), 1);
        TS_ASSERT_EQUALS(table2->rowCount(), 2);

        std::vector<int> e1, e2;
        TS_ASSERT_THROWS_NOTHING(e1 = table2->cell<std::vector<int>>(0, 0));
        TS_ASSERT_THROWS_NOTHING(e2 = table2->cell<std::vector<int>>(1, 0));

        TS_ASSERT_EQUALS(e1.size(), 32);
        TS_ASSERT_EQUALS(e2.size(), 32);

        TS_ASSERT_EQUALS(e1[0], 33);
        TS_ASSERT_EQUALS(e1[31], 64);

        TS_ASSERT_EQUALS(e2[0], 1);
        TS_ASSERT_EQUALS(e2[31], 32);
      }
    }

    AnalysisDataService::Instance().deepRemoveGroup(outWSName);
    AnalysisDataService::Instance().deepRemoveGroup(detectorGroupingWSName);
  }

  void test_autoGroup_singlePeriod() {
    ScopedWorkspace outWsEntry;

    try {
      LoadMuonNexus1 alg;
      alg.initialize();
      alg.setRethrows(true);
      alg.setPropertyValue("Filename", "emu00006473.nxs");
      alg.setProperty("AutoGroup", true);
      alg.setPropertyValue("OutputWorkspace", outWsEntry.name());
      alg.execute();
    } catch (std::exception &e) {
      TS_FAIL(e.what());
      return;
    }

    auto outWs = std::dynamic_pointer_cast<MatrixWorkspace>(outWsEntry.retrieve());
    TS_ASSERT(outWs);

    if (!outWs)
      return; // Nothing to check

    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2000);

    TS_ASSERT_EQUALS(outWs->y(0)[0], 461);
    TS_ASSERT_EQUALS(outWs->y(0)[1000], 192);
    TS_ASSERT_EQUALS(outWs->y(0)[1998], 1);

    TS_ASSERT_EQUALS(outWs->y(1)[0], 252);
    TS_ASSERT_EQUALS(outWs->y(1)[1000], 87);
    TS_ASSERT_EQUALS(outWs->y(1)[1998], 2);
  }

  void test_autoGroup_multiPeriod() {
    ScopedWorkspace outWsEntry;

    try {
      LoadMuonNexus1 alg;
      alg.initialize();
      alg.setRethrows(true);
      alg.setPropertyValue("Filename", "MUSR00015189.nxs");
      alg.setProperty("AutoGroup", true);
      alg.setPropertyValue("OutputWorkspace", outWsEntry.name());
      alg.execute();
    } catch (std::exception &e) {
      TS_FAIL(e.what());
      return;
    }

    auto outWs = std::dynamic_pointer_cast<WorkspaceGroup>(outWsEntry.retrieve());
    TS_ASSERT(outWs);

    if (!outWs)
      return; // Nothing to check

    TS_ASSERT_EQUALS(outWs->size(), 2);

    auto outWs1 = std::dynamic_pointer_cast<MatrixWorkspace>(outWs->getItem(0));
    TS_ASSERT(outWs1);

    if (outWs1) {
      TS_ASSERT_EQUALS(outWs1->getNumberHistograms(), 2);
      TS_ASSERT_EQUALS(outWs1->blocksize(), 2000);

      TS_ASSERT_EQUALS(outWs1->y(0)[0], 82);
      TS_ASSERT_EQUALS(outWs1->y(0)[458], 115);
      TS_ASSERT_EQUALS(outWs1->y(0)[1997], 1);

      TS_ASSERT_EQUALS(outWs1->y(1)[0], 6);
      TS_ASSERT_EQUALS(outWs1->y(1)[458], 91);
      TS_ASSERT_EQUALS(outWs1->y(1)[1997], 0);
    }

    auto outWs2 = std::dynamic_pointer_cast<MatrixWorkspace>(outWs->getItem(1));
    TS_ASSERT(outWs2);

    if (outWs2) {
      TS_ASSERT_EQUALS(outWs2->getNumberHistograms(), 2);
      TS_ASSERT_EQUALS(outWs2->blocksize(), 2000);

      TS_ASSERT_EQUALS(outWs2->y(0)[0], 16);
      TS_ASSERT_EQUALS(outWs2->y(0)[458], 132);
      TS_ASSERT_EQUALS(outWs2->y(0)[1930], 0);

      TS_ASSERT_EQUALS(outWs2->y(1)[0], 17);
      TS_ASSERT_EQUALS(outWs2->y(1)[458], 81);
      TS_ASSERT_EQUALS(outWs2->y(1)[1930], 1);
    }
  }

  void test_loadRunInformation() {
    ScopedWorkspace outWsEntry;

    LoadMuonNexus1 alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "emu00006475.nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWsEntry.name()));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    Workspace_sptr outWs = outWsEntry.retrieve();
    TS_ASSERT(outWs);

    if (!outWs)
      return;

    auto gws = std::dynamic_pointer_cast<WorkspaceGroup>(outWs);
    TS_ASSERT(gws);

    if (!gws)
      return;

    auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(gws->getItem(0));
    TS_ASSERT(ws);
    const Run &run = ws->run();

    // Check expected properties
    checkProperty(run, "run_number", std::string("6475"));
    checkProperty(run, "run_title", std::string("ptfe test T=280 F=20.0"));
    checkProperty(run, "run_start", std::string("2006-11-21T15:36:11"));
    checkProperty(run, "run_end", std::string("2006-11-21T17:10:18"));
    checkProperty(run, "dur_secs", std::string("5647"));
    checkProperty(run, "nspectra", 32);
    checkProperty(run, "goodfrm", 60800);

    checkProperty(run, "sample_temp", 280.0);
    checkProperty(run, "sample_magn_field", 20.0);
  }

  /**
   * CHRONUS0003422.nxs has no grouping entry in the file
   * Test loading grouping from this file
   */
  void test_loadingDetectorGrouping_missingGrouping() {
    LoadMuonNexus1 alg;
    try {
      alg.initialize();
      alg.setChild(true);
      alg.setPropertyValue("Filename", "CHRONUS00003422.nxs");
      alg.setPropertyValue("OutputWorkspace", "__NotUsed");
      alg.setPropertyValue("DetectorGroupingTable", "__Grouping");
      alg.execute();
    } catch (const std::exception &error) {
      TS_FAIL(error.what());
    }

    Workspace_sptr grouping;
    TS_ASSERT_THROWS_NOTHING(grouping = alg.getProperty("DetectorGroupingTable"));
    const auto detectorGrouping = std::dynamic_pointer_cast<TableWorkspace>(grouping);

    if (detectorGrouping) {
      TS_ASSERT_EQUALS(detectorGrouping->columnCount(), 1);
      TS_ASSERT_EQUALS(detectorGrouping->rowCount(), 8);

      TS_ASSERT_EQUALS(detectorGrouping->getColumn(0)->type(), "vector_int");
      TS_ASSERT_EQUALS(detectorGrouping->getColumn(0)->name(), "Detectors");

      std::vector<int> left, right, up, down;
      TS_ASSERT_THROWS_NOTHING(left = detectorGrouping->cell<std::vector<int>>(0, 0));
      TS_ASSERT_THROWS_NOTHING(right = detectorGrouping->cell<std::vector<int>>(1, 0));

      TS_ASSERT_THROWS_NOTHING(up = detectorGrouping->cell<std::vector<int>>(2, 0));
      TS_ASSERT_THROWS_NOTHING(down = detectorGrouping->cell<std::vector<int>>(3, 0));

      TS_ASSERT_EQUALS(left.size(), 76);
      TS_ASSERT_EQUALS(right.size(), 78);
      TS_ASSERT_EQUALS(up.size(), 76);
      TS_ASSERT_EQUALS(down.size(), 74);

    } else {
      TS_FAIL("Loaded grouping was null");
    }
  }

  /**
   * EMU00019489.nxs has a grouping entry in the file, but it is
   * filled with zeros.
   * Test loading grouping from this file
   */
  void test_loadingDetectorGrouping_zeroGrouping() {
    LoadMuonNexus1 alg;
    try {
      alg.initialize();
      alg.setChild(true);
      alg.setPropertyValue("Filename", "EMU00019489.nxs");
      alg.setPropertyValue("OutputWorkspace", "__NotUsed");
      alg.setPropertyValue("DetectorGroupingTable", "__Grouping");
      alg.execute();
    } catch (const std::exception &error) {
      TS_FAIL(error.what());
    }

    Workspace_sptr grouping;
    TS_ASSERT_THROWS_NOTHING(grouping = alg.getProperty("DetectorGroupingTable"));
    const auto detectorGrouping = std::dynamic_pointer_cast<TableWorkspace>(grouping);

    if (detectorGrouping) {
      TS_ASSERT_EQUALS(detectorGrouping->columnCount(), 1);
      TS_ASSERT_EQUALS(detectorGrouping->rowCount(), 2);

      TS_ASSERT_EQUALS(detectorGrouping->getColumn(0)->type(), "vector_int");
      TS_ASSERT_EQUALS(detectorGrouping->getColumn(0)->name(), "Detectors");

      std::vector<int> fwd, bwd;
      TS_ASSERT_THROWS_NOTHING(fwd = detectorGrouping->cell<std::vector<int>>(0, 0));
      TS_ASSERT_THROWS_NOTHING(bwd = detectorGrouping->cell<std::vector<int>>(1, 0));

      TS_ASSERT_EQUALS(fwd.size(), 48);
      TS_ASSERT_EQUALS(bwd.size(), 48);

      for (int i = 0; i < 48; i++) {
        TS_ASSERT_EQUALS(fwd[i], i + 1);
        TS_ASSERT_EQUALS(bwd[i], i + 49);
      }
    } else {
      TS_FAIL("Loaded grouping was null");
    }
  }

  /**
   * Some old data does not have run/instrument/beam/frames_good.
   * Test that we can use run/instrument/beam/frames in this case to get a
   * goodfrm value.
   * Example file: MUT53591
   */
  void test_loadingNumGoodFrames_notPresent() {
    LoadMuonNexus1 alg;
    ScopedWorkspace outWsEntry;
    try {
      alg.initialize();
      alg.setPropertyValue("Filename", "MUT00053591.NXS");
      alg.setPropertyValue("OutputWorkspace", outWsEntry.name());
      alg.execute();
    } catch (const std::exception &error) {
      TS_FAIL(error.what());
    }
    Workspace_sptr outWs = outWsEntry.retrieve();
    TS_ASSERT(outWs);
    const auto matrixWs = std::dynamic_pointer_cast<MatrixWorkspace>(outWs);
    TS_ASSERT(matrixWs);
    if (matrixWs) {
      checkProperty(matrixWs->run(), "goodfrm", 65500);
    }
  }

private:
  LoadMuonNexus1 nxLoad, nxload2, nxload3;
  std::string outputSpace;
  std::string entryName;
  std::string inputFile;
  std::string inputFile2;

  template <typename T> void checkProperty(const Run &run, const std::string &property, const T &expectedValue) {
    if (run.hasProperty(property)) {
      T propertyValue;

      try {
        propertyValue = run.getPropertyValueAsType<T>(property);
      } catch (...) {
        TS_FAIL("Unexpected property type: " + property);
        return;
      }

      TSM_ASSERT_EQUALS("Property value mismatch: " + property, propertyValue, expectedValue);
    } else {
      TS_FAIL("No property: " + property);
    }
  }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadMuonNexus1TestPerformance : public CxxTest::TestSuite {
  void setUp() override {
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006475.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
  }

  void tearDown() override { AnalysisDataService::Instance().remove("ws"); }

public:
  void testDefaultLoad() { loader.execute(); }

private:
  LoadMuonNexus1 loader;
};
