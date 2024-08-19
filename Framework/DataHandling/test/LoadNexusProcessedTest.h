// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataHandling/SaveNexusProcessed.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidHistogramData/Histogram.h"

#include "SaveNexusProcessedTest.h"

#include <cxxtest/TestSuite.h>

#include <hdf5.h>

#include <Poco/File.h>

#include <string>

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::HistogramData;
using Mantid::detid_t;

// Note that this suite tests an old version of Nexus processed files that we
// continue to support.
// LoadRawSaveNxsLoadNxs tests the current version of Nexus processed by loading
// a newly created Nexus processed file.
//
// LoadRawSaveNxsLoadNxs should be run when making any changes to
// LoadNexusProcessed
// in addition to this test.

class LoadNexusProcessedTest : public CxxTest::TestSuite {
public:
  static LoadNexusProcessedTest *createSuite() { return new LoadNexusProcessedTest(); }
  static void destroySuite(LoadNexusProcessedTest *suite) { delete suite; }

  LoadNexusProcessedTest() : testFile("GEM38370_Focussed_Legacy.nxs"), output_ws("nxstest"), m_savedTmpEventFile("") {}

  ~LoadNexusProcessedTest() override {
    AnalysisDataService::Instance().clear();
    clearTmpEventNexus();
  }

  void testFastMultiPeriodDefault() {
    LoadNexusProcessed alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
    const bool bFastMultiPeriod = alg.getProperty("FastMultiPeriod");
    TSM_ASSERT("Should default to offering fast multiperiod loading", bFastMultiPeriod);
  }

  void testProcessedFile() {

    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve(output_ws));
    TS_ASSERT(workspace.get());

    MatrixWorkspace_sptr matrix_ws = std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT(matrix_ws.get());

    // Test proton charge from the sample block
    TS_ASSERT_DELTA(matrix_ws->run().getProtonCharge(), 30.14816, 1e-5);

    doHistoryTest(matrix_ws);

    std::shared_ptr<const Mantid::Geometry::Instrument> inst = matrix_ws->getInstrument();
    TS_ASSERT_EQUALS(inst->getName(), "GEM");
    TS_ASSERT_EQUALS(inst->getSource()->getPos().Z(), -17);
  }

  void testNexusProcessed_Min_Max() {

    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    testFile = "focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumMin", "2");
    alg.setPropertyValue("SpectrumMax", "4");

    const std::vector<int> expectedSpectra = {3, 4, 5};
    doSpectrumListTests(alg, expectedSpectra);
  }

  void testNexusProcessed_List() {
    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    testFile = "focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumList", "1,2,3,4");

    const std::vector<int> expectedSpectra = {2, 3, 4, 5};
    doSpectrumListTests(alg, expectedSpectra);
  }

  void testNexusProcessed_Min_Max_List() {
    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    testFile = "focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumMin", "1");
    alg.setPropertyValue("SpectrumMax", "3");
    alg.setPropertyValue("SpectrumList", "4,5");

    const std::vector<int> expectedSpectra = {2, 3, 4, 5, 6};
    doSpectrumListTests(alg, expectedSpectra);
  }

  void testNexusProcessed_Min() {
    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    testFile = "focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumMin", "4");

    doSpectrumMinOrMaxTest(alg, 3);
  }

  void testNexusProcessed_Max() {
    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    testFile = "focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumMax", "3");

    doSpectrumMinOrMaxTest(alg, 3);
  }

  // Saving and reading masking correctly
  void testMasked() {
    LoadNexusProcessed alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    testFile = "focussed.nxs";
    alg.setPropertyValue("Filename", testFile);
    testFile = alg.getPropertyValue("Filename");

    alg.setPropertyValue("OutputWorkspace", output_ws);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Test some aspects of the file
    MatrixWorkspace_sptr workspace;
    workspace = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(output_ws));
    TS_ASSERT(workspace.get());

    for (size_t si = 0; si < workspace->getNumberHistograms(); ++si) {
      workspace->maskBin(si, 0, 1.0);
      workspace->maskBin(si, 1, 1.0);
      workspace->maskBin(si, 2, 1.0);
    }

    SaveNexusProcessed save;
    save.initialize();
    save.setPropertyValue("InputWorkspace", output_ws);
    std::string filename = "LoadNexusProcessed_tmp.nxs";
    save.setPropertyValue("Filename", filename);
    filename = save.getPropertyValue("Filename");
    save.execute();
    LoadNexusProcessed load;
    load.initialize();
    load.setPropertyValue("Filename", filename);
    load.setPropertyValue("OutputWorkspace", output_ws);
    load.execute();

    workspace = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(output_ws));
    TS_ASSERT(workspace.get());

    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 6);

    TS_ASSERT(workspace->hasMaskedBins(0));
    TS_ASSERT(workspace->hasMaskedBins(1));
    TS_ASSERT(workspace->hasMaskedBins(2));
    TS_ASSERT(workspace->hasMaskedBins(3));
    TS_ASSERT(workspace->hasMaskedBins(4));
    TS_ASSERT(workspace->hasMaskedBins(5));

    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void dotest_LoadAnEventFile(EventType type) {
    std::string filename_root = "LoadNexusProcessed_ExecEvent_";

    // Call a function that writes out the file
    std::string outputFile;
    EventWorkspace_sptr origWS =
        SaveNexusProcessedTest::do_testExec_EventWorkspaces(filename_root, type, outputFile, false, false);

    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("Filename", outputFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve(output_ws));
    TS_ASSERT(workspace.get());

    EventWorkspace_sptr ws = std::dynamic_pointer_cast<EventWorkspace>(workspace);
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Testing the number of histograms
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 5);

    TS_ASSERT_EQUALS(ws->x(0).size(), 100);

    for (size_t wi = 0; wi < 5; wi++) {
      const EventList &el = ws->getSpectrum(wi);
      TS_ASSERT_EQUALS(el.getEventType(), type);
      TS_ASSERT(el.hasDetectorID(detid_t(wi + 1) * 10));
    }
    TS_ASSERT_EQUALS(ws->getSpectrum(0).getNumberEvents(), 300);
    TS_ASSERT_EQUALS(ws->getSpectrum(1).getNumberEvents(), 100);
    TS_ASSERT_EQUALS(ws->getSpectrum(2).getNumberEvents(), 200);
    TS_ASSERT_EQUALS(ws->getSpectrum(3).getNumberEvents(), 0);
    TS_ASSERT_EQUALS(ws->getSpectrum(4).getNumberEvents(), 100);

    // Do the comparison algo to check that they really are the same
    origWS->sortAll(TOF_SORT, nullptr);
    ws->sortAll(TOF_SORT, nullptr);

    auto alg2 = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    alg2->initialize();
    alg2->setProperty<MatrixWorkspace_sptr>("Workspace1", origWS);
    alg2->setProperty<MatrixWorkspace_sptr>("Workspace2", ws);
    alg2->setProperty<double>("Tolerance", 1e-5);
    alg2->setProperty<bool>("CheckAxes", false);
    alg2->execute();
    if (alg2->isExecuted()) {
      TS_ASSERT(alg2->getProperty("Result"));
    } else {
      TS_ASSERT(false);
    }

    // Clear old file
    if (Poco::File(outputFile).exists())
      Poco::File(outputFile).remove();
  }

  void test_LoadEventNexus_TOF() { dotest_LoadAnEventFile(TOF); }

  void test_LoadEventNexus_WEIGHTED() { dotest_LoadAnEventFile(WEIGHTED); }

  void test_LoadEventNexus_WEIGHTED_NOTIME() { dotest_LoadAnEventFile(WEIGHTED_NOTIME); }

  void test_loadEventNexus_Min() {
    writeTmpEventNexus();

    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("Filename", m_savedTmpEventFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumMin", "3");
    // this should imply 4==ws->getNumberHistograms()

    // expected number of spectra and length of the alg history
    doCommonEventLoadChecks(alg, 4, 2);
  }

  void test_loadEventNexus_Max() {
    writeTmpEventNexus();

    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("Filename", m_savedTmpEventFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumMax", "2");
    // this should imply 3==ws->getNumberHistograms()

    // expected number of spectra and length of the alg history
    doCommonEventLoadChecks(alg, 2, 2);
  }

  void test_loadEventNexus_Min_Max() {
    writeTmpEventNexus();

    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("Filename", m_savedTmpEventFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumMin", "2");
    alg.setPropertyValue("SpectrumMax", "4");
    // this should imply 3==ws->getNumberHistograms()

    // expected number of spectra and length of the alg history
    // in history, expect: load + LoadInst (child)
    doCommonEventLoadChecks(alg, 3, 2);
  }

  void test_loadEventNexus_Fail() {
    writeTmpEventNexus();

    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("Filename", m_savedTmpEventFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumList", "1,3,5,89");
    // the 89 should cause trouble, but gracefully...

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());
  }

  void test_loadEventNexus_List() {
    writeTmpEventNexus();

    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("Filename", m_savedTmpEventFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumList", "1,3,5");
    // this should imply 3==ws->getNumberHistograms()

    // expected number of spectra and length of the alg history
    doCommonEventLoadChecks(alg, 3, 2);
  }

  void test_loadEventNexus_Min_List() {
    writeTmpEventNexus();

    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("Filename", m_savedTmpEventFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumList", "5");
    alg.setPropertyValue("SpectrumMin", "4");
    // this should imply 2==ws->getNumberHistograms()

    // expected number of spectra and length of the alg history
    doCommonEventLoadChecks(alg, 3, 2);
  }

  void test_loadEventNexus_Max_List() {
    writeTmpEventNexus();

    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("Filename", m_savedTmpEventFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumMax", "2");
    alg.setPropertyValue("SpectrumList", "3,5");
    // this should imply 4==ws->getNumberHistograms()

    // expected number of spectra and length of the alg history
    doCommonEventLoadChecks(alg, 4, 2);
  }

  void test_loadEventNexus_Min_Max_List() {
    writeTmpEventNexus();

    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("Filename", m_savedTmpEventFile);
    alg.setPropertyValue("OutputWorkspace", output_ws);
    alg.setPropertyValue("SpectrumMin", "3");
    alg.setPropertyValue("SpectrumMax", "5");
    alg.setPropertyValue("SpectrumList", "1,2,3,5");
    // this should imply 5(all)==ws->getNumberHistograms()

    // expected number of spectra and length of the alg history
    doCommonEventLoadChecks(alg, 5, 2);
  }

  void test_load_saved_workspace_group() {
    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("Filename", "WorkspaceGroup.nxs");
    alg.setPropertyValue("OutputWorkspace", "group");

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve("group"));
    WorkspaceGroup_sptr group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    TS_ASSERT(group);
    int groupSize = group->getNumberOfEntries();
    TS_ASSERT_EQUALS(groupSize, 12);
    for (int i = 0; i < groupSize; ++i) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(i));
      TS_ASSERT(ws);
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 10);
      TS_ASSERT_EQUALS(ws->getName(), "group_" + std::to_string(i + 1));
    }
  }

  void test_load_workspace_group_unique_names() {
    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // Group two uses unique names for each workspace
    alg.setPropertyValue("Filename", "WorkspaceGroup2.nxs");
    alg.setPropertyValue("OutputWorkspace", "group");

    const char *suffix[] = {"eq2", "eq1", "elf"};
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve("group"));
    WorkspaceGroup_sptr group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    TS_ASSERT(group);
    int groupSize = group->getNumberOfEntries();
    TS_ASSERT_EQUALS(groupSize, 3);
    for (int i = 0; i < groupSize; ++i) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(i));
      TS_ASSERT(ws);
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 2);
      TS_ASSERT_EQUALS(ws->getName(), "irs55125_graphite002_to_55131_" + std::string(suffix[i]));
    }
  }

  void test_load_workspace_group_unique_names_two_workspaces() {
    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // Group two uses unique names for each workspace
    alg.setPropertyValue("Filename", "WorkspaceGroup2.nxs");
    alg.setPropertyValue("OutputWorkspace", "group");

    const char *suffix[] = {"eq2", "eq1", "elf"};
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve("group"));
    WorkspaceGroup_sptr group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    TS_ASSERT(group);
    int groupSize = group->getNumberOfEntries();
    TS_ASSERT_EQUALS(groupSize, 3);
    for (int i = 0; i < groupSize; ++i) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(i));
      TS_ASSERT(ws);
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 2);
      TS_ASSERT_EQUALS(ws->getName(), "irs55125_graphite002_to_55131_" + std::string(suffix[i]));
    }

    // load same file again, but to a different group
    // this checks that the names will be unique

    LoadNexusProcessed alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // Group two uses unique names for each workspace
    alg2.setPropertyValue("Filename", "WorkspaceGroup2.nxs");
    alg2.setPropertyValue("OutputWorkspace", "group2");

    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve("group2"));
    group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    TS_ASSERT(group);
    groupSize = group->getNumberOfEntries();
    TS_ASSERT_EQUALS(groupSize, 3);

    for (int i = 0; i < groupSize; ++i) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(i));
      TS_ASSERT(ws);
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 2);
      TS_ASSERT_EQUALS(ws->getName(), "irs55125_graphite002_to_55131_" + std::string(suffix[i]) + "_1");
    }
  }

  void test_load_fit_parameters() {
    LoadNexusProcessed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("Filename", "HRP38692a.nxs");
    alg.setPropertyValue("OutputWorkspace", "HRPDparameters");

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("HRPDparameters");

    // test to see if parameters are loaded
    std::vector<std::shared_ptr<const Mantid::Geometry::IComponent>> bankComp =
        ws->getInstrument()->getAllComponentsWithName("bank_bsk");

    TS_ASSERT(bankComp[0]->getParameterNames().size() == 3);
  }

  void testTableWorkspace() {
    Load alg;
    alg.initialize();
    alg.setPropertyValue("Filename", "SavedTableWorkspace.nxs");
    const std::string wsName("SavedTableWorkspace");
    alg.setPropertyValue("OutputWorkspace", wsName);
    TS_ASSERT(alg.execute());

    TableWorkspace_const_sptr ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(wsName);
    TS_ASSERT_EQUALS(ws->columnCount(), 10);
    TS_ASSERT_EQUALS(ws->rowCount(), 4);

    try {
      {
        ConstColumnVector<std::string> column = ws->getVector("Name");
        TS_ASSERT_EQUALS(column[0], "Height");
        TS_ASSERT_EQUALS(column[1], "PeakCentre");
        TS_ASSERT_EQUALS(column[2], "Sigma");
        TS_ASSERT_EQUALS(column[3], "Cost function value");
      }
      {
        ConstColumnVector<double> column = ws->getVector("Value");
        TS_ASSERT_DELTA(column[0], 79.2315, 0.0001);
        TS_ASSERT_DELTA(column[1], 2.3979, 0.0001);
        TS_ASSERT_DELTA(column[2], 0.3495, 0.0001);
        TS_ASSERT_DELTA(column[3], 35.6841, 0.0001);
      }
      {
        ConstColumnVector<double> column = ws->getVector("Error");
        TS_ASSERT_DELTA(column[0], 0.814478, 0.0001);
        TS_ASSERT_DELTA(column[1], 0.00348291, 0.000001);
        TS_ASSERT_DELTA(column[2], 0.00342847, 0.000001);
        TS_ASSERT_EQUALS(column[3], 0);
      }
      {
        TS_ASSERT_THROWS(ConstColumnVector<int64_t> column = ws->getVector("Integer"), const std::runtime_error &);
        ConstColumnVector<int> column = ws->getVector("Integer");
        TS_ASSERT_EQUALS(column[0], 5);
        TS_ASSERT_EQUALS(column[1], 3);
        TS_ASSERT_EQUALS(column[2], 2);
        TS_ASSERT_EQUALS(column[3], 4);
      }
      {
        ConstColumnVector<uint32_t> column = ws->getVector("UInteger");
        TS_ASSERT_EQUALS(column[0], 35);
        TS_ASSERT_EQUALS(column[1], 33);
        TS_ASSERT_EQUALS(column[2], 32);
        TS_ASSERT_EQUALS(column[3], 34);
      }
      {
        ConstColumnVector<int64_t> column = ws->getVector("Integer64");
        TS_ASSERT_EQUALS(column[0], 15);
        TS_ASSERT_EQUALS(column[1], 13);
        TS_ASSERT_EQUALS(column[2], 12);
        TS_ASSERT_EQUALS(column[3], 14);
      }
      {
        ConstColumnVector<float> column = ws->getVector("Float");
        TS_ASSERT_DELTA(column[0], 0.5, 0.000001);
        TS_ASSERT_DELTA(column[1], 0.3, 0.000001);
        TS_ASSERT_DELTA(column[2], 0.2, 0.000001);
        TS_ASSERT_DELTA(column[3], 0.4, 0.000001);
      }
      {
        ConstColumnVector<size_t> column = ws->getVector("Size");
        TS_ASSERT_EQUALS(column[0], 25);
        TS_ASSERT_EQUALS(column[1], 23);
        TS_ASSERT_EQUALS(column[2], 22);
        TS_ASSERT_EQUALS(column[3], 24);
      }
      {
        ConstColumnVector<Boolean> column = ws->getVector("Bool");
        TS_ASSERT(column[0]);
        TS_ASSERT(column[1]);
        TS_ASSERT(!column[2]);
        TS_ASSERT(column[3]);
      }
      {
        ConstColumnVector<Mantid::Kernel::V3D> column = ws->getVector("3DVector");
        TS_ASSERT_EQUALS(column[0], Mantid::Kernel::V3D(1, 2, 3));
        TS_ASSERT_EQUALS(column[1], Mantid::Kernel::V3D(4, 5, 6));
        TS_ASSERT_EQUALS(column[2], Mantid::Kernel::V3D(7, 8, 9));
        TS_ASSERT_EQUALS(column[3], Mantid::Kernel::V3D(11, 12, 13));
      }
    } catch (std::exception &e) {
      TS_FAIL(e.what());
    }

    AnalysisDataService::Instance().remove(wsName);
  }

  void test_peaks_workspace_with_shape_format() {
    LoadNexusProcessed loadAlg;
    loadAlg.setChild(true);
    loadAlg.initialize();
    loadAlg.setPropertyValue("Filename", "SingleCrystalPeakTable.nxs");
    loadAlg.setPropertyValue("OutputWorkspace", "dummy");
    loadAlg.execute();

    Workspace_sptr ws = loadAlg.getProperty("OutputWorkspace");
    auto peakWS = std::dynamic_pointer_cast<Mantid::DataObjects::PeaksWorkspace>(ws);
    TS_ASSERT(peakWS);

    TS_ASSERT_EQUALS(3, peakWS->getNumberPeaks());
    // In this peaks workspace one of the peaks has been marked as spherically
    // integrated.
    TS_ASSERT_EQUALS("spherical", peakWS->getPeak(0).getPeakShape().shapeName());
    TS_ASSERT_EQUALS("none", peakWS->getPeak(1).getPeakShape().shapeName());
    TS_ASSERT_EQUALS("none", peakWS->getPeak(2).getPeakShape().shapeName());
  }

  void test_lean_peaks_workspace_with_shape_format() {
    LoadNexusProcessed loadAlg;
    loadAlg.setChild(true);
    loadAlg.initialize();
    loadAlg.setPropertyValue("Filename", "SingleCrystalLeanElasticPeakTableNew.nxs");
    loadAlg.setPropertyValue("OutputWorkspace", "dummy");
    loadAlg.execute();

    Workspace_sptr ws = loadAlg.getProperty("OutputWorkspace");
    auto peakWS = std::dynamic_pointer_cast<Mantid::DataObjects::LeanElasticPeaksWorkspace>(ws);
    TS_ASSERT(peakWS);

    TS_ASSERT_EQUALS(3, peakWS->getNumberPeaks());
    // In this peaks workspace one of the peaks has been marked as spherically
    // integrated.
    TS_ASSERT_EQUALS("none", peakWS->getPeak(0).getPeakShape().shapeName());
    TS_ASSERT_EQUALS("spherical", peakWS->getPeak(1).getPeakShape().shapeName());
    TS_ASSERT_EQUALS("spherical", peakWS->getPeak(2).getPeakShape().shapeName());
  }

  /* The nexus format for this type of workspace has a legacy format with no
   * shape information
   * We should still be able to load that */
  void test_peaks_workspace_no_shape_format() {
    LoadNexusProcessed loadAlg;
    loadAlg.setChild(true);
    loadAlg.initialize();
    loadAlg.setPropertyValue("Filename", "SingleCrystalPeakTableLegacy.nxs");
    loadAlg.setPropertyValue("OutputWorkspace", "dummy");
    loadAlg.execute();

    Workspace_sptr ws = loadAlg.getProperty("OutputWorkspace");
    auto peakWS = std::dynamic_pointer_cast<Mantid::DataObjects::PeaksWorkspace>(ws);
    TS_ASSERT(peakWS);
  }

  void test_coordinates_saved_and_loaded_on_peaks_workspace() {
    auto peaksTestWS = WorkspaceCreationHelper::createPeaksWorkspace(2);
    // Loading a peaks workspace without a instrument from an IDF doesn't work
    // ...
    const std::string filename = FileFinder::Instance().getFullPath("unit_testing/MINITOPAZ_Definition.xml");
    InstrumentDefinitionParser parser(filename, "MINITOPAZ", Strings::loadFile(filename));
    auto instrument = parser.parseXML(nullptr);
    peaksTestWS->populateInstrumentParameters();
    peaksTestWS->setInstrument(instrument);

    const SpecialCoordinateSystem appliedCoordinateSystem = QSample;
    peaksTestWS->setCoordinateSystem(appliedCoordinateSystem);

    SaveNexusProcessed saveAlg;
    saveAlg.setChild(true);
    saveAlg.initialize();
    saveAlg.setProperty("InputWorkspace", peaksTestWS);
    saveAlg.setPropertyValue("Filename", "LoadAndSaveNexusProcessedCoordinateSystem.nxs");
    saveAlg.execute();
    std::string filePath = saveAlg.getPropertyValue("Filename");

    LoadNexusProcessed loadAlg;
    loadAlg.setChild(true);
    loadAlg.initialize();
    loadAlg.setPropertyValue("Filename", filePath);
    loadAlg.setPropertyValue("OutputWorkspace", "__unused");
    loadAlg.execute();

    Mantid::API::Workspace_sptr loadedWS = loadAlg.getProperty("OutputWorkspace");
    auto loadedPeaksWS = std::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(loadedWS);
    Poco::File testFile(filePath);
    if (testFile.exists()) {
      testFile.remove();
    }

    TS_ASSERT_EQUALS(appliedCoordinateSystem, loadedPeaksWS->getSpecialCoordinateSystem());
  }

  // backwards compatability check
  void test_coordinates_saved_and_loaded_on_peaks_workspace_from_expt_info() {
    auto peaksTestWS = WorkspaceCreationHelper::createPeaksWorkspace(2);
    // Loading a peaks workspace without a instrument from an IDF doesn't work
    // ...
    const std::string filename = FileFinder::Instance().getFullPath("unit_testing/MINITOPAZ_Definition.xml");
    InstrumentDefinitionParser parser(filename, "MINITOPAZ", Strings::loadFile(filename));
    auto instrument = parser.parseXML(nullptr);
    peaksTestWS->populateInstrumentParameters();
    peaksTestWS->setInstrument(instrument);

    // simulate old-style file with "CoordinateSystem" log
    const SpecialCoordinateSystem appliedCoordinateSystem = QSample;
    peaksTestWS->logs()->addProperty("CoordinateSystem", static_cast<int>(appliedCoordinateSystem));

    SaveNexusProcessed saveAlg;
    saveAlg.setChild(true);
    saveAlg.initialize();
    saveAlg.setProperty("InputWorkspace", peaksTestWS);
    saveAlg.setPropertyValue("Filename", "LoadAndSaveNexusProcessedCoordinateSystemOldFormat.nxs");
    saveAlg.execute();
    std::string filePath = saveAlg.getPropertyValue("Filename");

    // Remove the coordinate_system entry so it falls back on the log. NeXus
    // can't do this so use the HDF5 API directly
    auto fid = H5Fopen(filePath.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    auto mantid_id = H5Gopen(fid, "mantid_workspace_1", H5P_DEFAULT);
    auto peaks_id = H5Gopen(mantid_id, "peaks_workspace", H5P_DEFAULT);
    if (peaks_id > 0) {
      H5Ldelete(peaks_id, "coordinate_system", H5P_DEFAULT);
      H5Gclose(peaks_id);
      H5Gclose(mantid_id);
    } else {
      TS_FAIL("Cannot unlink coordinate_system group. Test file has unexpected "
              "structure.");
    }
    H5Fclose(fid);

    LoadNexusProcessed loadAlg;
    loadAlg.setChild(true);
    loadAlg.initialize();
    loadAlg.setPropertyValue("Filename", filePath);
    loadAlg.setPropertyValue("OutputWorkspace", "__unused");
    loadAlg.execute();

    Mantid::API::Workspace_sptr loadedWS = loadAlg.getProperty("OutputWorkspace");
    auto loadedPeaksWS = std::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(loadedWS);
    Poco::File testFile(filePath);
    if (testFile.exists()) {
      testFile.remove();
    }

    TS_ASSERT_EQUALS(appliedCoordinateSystem, loadedPeaksWS->getSpecialCoordinateSystem());
  }

  void testTableWorkspace_vectorColumn() {
    // Create a table we will save
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
    table->addColumn("vector_int", "IntVectorColumn");
    table->addColumn("vector_double", "DoubleVectorColumn");

    std::vector<double> d1, d2, d3;
    d1.emplace_back(0.5);
    d2.emplace_back(1.0);
    d2.emplace_back(2.5);
    d3.emplace_back(4.0);

    std::vector<int> i1(Strings::parseRange("1"));
    std::vector<int> i2(Strings::parseRange("2,3,"));
    std::vector<int> i3(Strings::parseRange("4,5,6,7"));

    // Add some rows of different sizes
    TableRow row1 = table->appendRow();
    row1 << i1 << d1;
    TableRow row2 = table->appendRow();
    row2 << i2 << d2;
    TableRow row3 = table->appendRow();
    row3 << i3 << d3;

    ScopedWorkspace inTableEntry(table);
    std::string savedFileName("LoadNexusProcessedTest_testTableWorkspace_vectorColumn.nxs");

    SaveNexusProcessed saveAlg;
    saveAlg.initialize();
    saveAlg.setPropertyValue("InputWorkspace", inTableEntry.name());
    saveAlg.setPropertyValue("Filename", savedFileName);

    TS_ASSERT_THROWS_NOTHING(saveAlg.execute());
    TS_ASSERT(saveAlg.isExecuted());

    if (!saveAlg.isExecuted())
      return; // Nothing to check

    // Get absolute path to the saved file
    savedFileName = saveAlg.getPropertyValue("Filename");

    ScopedWorkspace outTableEntry;

    LoadNexusProcessed loadAlg;
    loadAlg.initialize();
    loadAlg.setPropertyValue("Filename", savedFileName);
    loadAlg.setPropertyValue("OutputWorkspace", outTableEntry.name());

    TS_ASSERT_THROWS_NOTHING(loadAlg.execute());
    TS_ASSERT(loadAlg.isExecuted());

    // The file is not needed anymore
    Poco::File(savedFileName).remove();

    if (!loadAlg.isExecuted())
      return; // Nothing to check

    auto outTable = std::dynamic_pointer_cast<const TableWorkspace>(outTableEntry.retrieve());
    TS_ASSERT(outTable);

    if (!outTable)
      return; // Nothing to check

    TS_ASSERT_EQUALS(outTable->columnCount(), 2);
    TS_ASSERT_EQUALS(outTable->rowCount(), 3);

    Column_const_sptr column; // Column we are currently checking

    TS_ASSERT_THROWS_NOTHING(column = outTable->getColumn("IntVectorColumn"));
    TS_ASSERT(column->isType<std::vector<int>>());

    if (column->isType<std::vector<int>>()) {
      TS_ASSERT_EQUALS(column->cell<std::vector<int>>(0), i1);
      TS_ASSERT_EQUALS(column->cell<std::vector<int>>(1), i2);
      TS_ASSERT_EQUALS(column->cell<std::vector<int>>(2), i3);
    }

    TS_ASSERT_THROWS_NOTHING(column = outTable->getColumn("DoubleVectorColumn"));
    TS_ASSERT(column->isType<std::vector<double>>());

    if (column->isType<std::vector<double>>()) {
      TS_ASSERT_EQUALS(column->cell<std::vector<double>>(0), d1);
      TS_ASSERT_EQUALS(column->cell<std::vector<double>>(1), d2);
      TS_ASSERT_EQUALS(column->cell<std::vector<double>>(2), d3);
    }
  }

  void test_SaveAndLoadOnHistogramWS() { doTestLoadAndSaveHistogramWS(false); }

  void test_SaveAndLoadOnHistogramWSWithNumericAxis() { doTestLoadAndSaveHistogramWS(false, true); }

  void test_SaveAndLoadOnHistogramWSwithXErrors() { doTestLoadAndSaveHistogramWS(true); }

  void test_SaveAndLoadOnHistogramWSwithLegacyXErrors() { doTestLoadAndSaveHistogramWS(true, false, true); }

  void test_SaveAndLoadOnPointLikeWS() { doTestLoadAndSavePointWS(false); }

  void test_SaveAndLoadOnPointLikeWSWithXErrors() { doTestLoadAndSavePointWS(true); }

  void test_that_workspace_name_is_loaded() {
    // Arrange
    LoadNexusProcessed loader;
    loader.setChild(false);
    loader.initialize();
    loader.setPropertyValue("Filename", "POLREF00004699_nexus.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    loader.setProperty("FastMultiPeriod", true);
    // Act
    TS_ASSERT(loader.execute());
    // Assert
    TSM_ASSERT("Can access workspace via name which was set in file",
               AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("y_1"));
    TSM_ASSERT("Can access workspace via name which was set in file",
               AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("y_2"));
    // Clean up
    AnalysisDataService::Instance().remove("y_1");
    AnalysisDataService::Instance().remove("y_2");
  }

  void test_that_workspace_name_is_not_loaded_when_is_duplicate() {
    // Arrange
    SaveNexusProcessed alg;
    alg.initialize();
    std::string tempFile = "LoadNexusProcessed_TmpTestWorkspace.nxs";
    alg.setPropertyValue("Filename", tempFile);

    std::string workspaceName = "test_workspace_name";
    for (size_t index = 0; index < 2; ++index) {
      // Create a sample workspace and add it to the ADS, so it gets a name.
      auto ws = WorkspaceCreationHelper::create1DWorkspaceConstant(3, static_cast<double>(index),
                                                                   static_cast<double>(index), true);
      AnalysisDataService::Instance().addOrReplace(workspaceName, ws);
      alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(ws));
      if (index == 0) {
        alg.setProperty("Append", false);
      } else {
        alg.setProperty("Append", true);
      }
      alg.execute();
    }
    // Delete the workspace
    AnalysisDataService::Instance().remove(workspaceName);

    tempFile = alg.getPropertyValue("Filename");

    // Load the data
    LoadNexusProcessed loader;
    loader.setChild(false);
    loader.initialize();
    loader.setPropertyValue("Filename", tempFile);
    loader.setPropertyValue("OutputWorkspace", "ws_loaded");
    // Act
    loader.execute();

    // Assert
    TSM_ASSERT("Can access workspace via name which is the name of the file "
               "with an index",
               AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("ws_loaded_1"));
    TSM_ASSERT("Can access workspace via name which is the name of the file "
               "with an index",
               AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("ws_loaded_2"));
    // Clean up
    AnalysisDataService::Instance().remove("ws_loaded_1");
    AnalysisDataService::Instance().remove("ws_loaded_2");
    if (!tempFile.empty() && Poco::File(tempFile).exists()) {
      Poco::File(tempFile).remove();
    }
  }

  void do_load_multiperiod_workspace(bool fast) {
    LoadNexusProcessed loader;
    loader.setChild(true);
    loader.initialize();
    loader.setPropertyValue("Filename", "POLREF00004699_nexus.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    loader.setProperty("FastMultiPeriod", fast);

    TS_ASSERT(loader.execute());

    Workspace_sptr outWS = loader.getProperty("OutputWorkspace");
    WorkspaceGroup_sptr asGroupWS = std::dynamic_pointer_cast<WorkspaceGroup>(outWS);
    TSM_ASSERT("We expect a group workspace back", asGroupWS);
    TSM_ASSERT_EQUALS("We expect the size to be 2", 2, asGroupWS->size());
    MatrixWorkspace_sptr period1 = std::dynamic_pointer_cast<MatrixWorkspace>(asGroupWS->getItem(0));
    MatrixWorkspace_sptr period2 = std::dynamic_pointer_cast<MatrixWorkspace>(asGroupWS->getItem(1));

    TSM_ASSERT("We expect the group workspace is multiperiod", asGroupWS->isMultiperiod());
    TSM_ASSERT_EQUALS("X-data should be identical", period1->x(0), period2->x(0));
    TSM_ASSERT_DIFFERS("Y-data should be different", period1->y(0), period2->y(0));
    TSM_ASSERT_DIFFERS("E-data should be different", period1->e(0), period2->e(0));

    TS_ASSERT(period1->getInstrument());
    TS_ASSERT(period2->getInstrument());

    auto period1Logs = period1->run().getLogData();
    auto period2Logs = period2->run().getLogData();

    TSM_ASSERT_EQUALS("We expect to have the same number of log entries", period1Logs.size(), period2Logs.size());

    TSM_ASSERT_THROWS("Should only have a period 1 entry", period1->run().getLogData("period 2"),
                      Exception::NotFoundError &);
    TSM_ASSERT_THROWS("Should only have a period 2 entry", period2->run().getLogData("period 1"),
                      Exception::NotFoundError &);
  }

  void test_load_multiperiod_workspace_fast() { do_load_multiperiod_workspace(true /*Use optimised route*/); }

  void test_load_multiperiod_workspace_old() { do_load_multiperiod_workspace(false /*Use old route*/); }

  void test_load_workspace_empty_textaxis() {
    // filename workspaceEmptyTextAxis
    LoadNexusProcessed loader;
    loader.setChild(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "workspaceEmptyTextAxis.nxs"));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", "ws"));

    TS_ASSERT(loader.execute());
    TS_ASSERT(loader.isExecuted());

    Workspace_const_sptr ws = loader.getProperty("OutputWorkspace");
    const auto outWS = std::dynamic_pointer_cast<const MatrixWorkspace>(ws);

    const size_t numBins = outWS->blocksize();
    for (size_t i = 0; i < numBins; ++i) {
      TS_ASSERT_EQUALS(outWS->x(0)[i], i);
      TS_ASSERT_EQUALS(outWS->y(0)[i], i);
    }
  }

  void test_load_workspace_with_textaxis() {
    // filename workspaceWithTextAxis
    LoadNexusProcessed loader;
    loader.setChild(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "workspaceWithTextAxis.nxs"));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", "ws"));

    TS_ASSERT(loader.execute());
    TS_ASSERT(loader.isExecuted());

    Workspace_const_sptr ws = loader.getProperty("OutputWorkspace");
    const auto outWS = std::dynamic_pointer_cast<const MatrixWorkspace>(ws);

    const size_t numBins = outWS->blocksize();
    for (size_t i = 0; i < numBins; ++i) {
      TS_ASSERT_EQUALS(outWS->x(0)[i], i);
      TS_ASSERT_EQUALS(outWS->y(0)[i], i);
    }
  }

  void test_Log_invalid_value_filtering_survives_save_and_load() {
    // this test checks that this log is the same from both original and processed nexus
    const std::string LOG_TO_CHECK("cryo_temp1");
    constexpr int LOG_SIZE{1};
    constexpr int LOG_SECONDS{15};
    constexpr double LOG_VALUE{7.};

    LoadNexus alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    testFile = "ENGINX00228061_log_alarm_data.nxs";
    alg.setPropertyValue("Filename", testFile);
    testFile = alg.getPropertyValue("Filename");

    alg.setPropertyValue("OutputWorkspace", output_ws);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Test some aspects of the file
    MatrixWorkspace_sptr workspace;
    workspace = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(output_ws));

    check_log(workspace, LOG_TO_CHECK, LOG_SIZE, LOG_SECONDS, LOG_VALUE);

    SaveNexusProcessed save;
    save.initialize();
    save.setPropertyValue("InputWorkspace", output_ws);
    std::string filename = "LoadNexusProcessed_test_Log_invalid_value_"
                           "filtering_survives_save_and_load.nxs";
    save.setPropertyValue("Filename", filename);
    filename = save.getPropertyValue("Filename");
    save.execute();

    LoadNexusProcessed load;
    load.initialize();
    load.setPropertyValue("Filename", filename);
    load.setPropertyValue("OutputWorkspace", output_ws);
    load.execute();

    workspace = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(output_ws));
    TS_ASSERT(workspace.get());
    check_log(workspace, LOG_TO_CHECK, LOG_SIZE, LOG_SECONDS, LOG_VALUE);
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void test_log_filtering_survives_save_and_load() {
    LoadNexus alg;
    std::string group_ws = "test_log_filtering_survives_save_and_load";
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    testFile = "POLREF00014966.nxs";
    alg.setPropertyValue("Filename", testFile);
    testFile = alg.getPropertyValue("Filename");

    alg.setPropertyValue("OutputWorkspace", group_ws);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Test some aspects of the file
    MatrixWorkspace_sptr workspace;
    workspace = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(group_ws + "_1"));
    // should be filtered
    check_log(workspace, "raw_uah_log", 429, 17, 99.4740982879);
    // should not be filtered
    check_log(workspace, "periods", 37, 1, 1);
    check_log(workspace, "period 1", 36, 505, true);
    check_log(workspace, "running", 72, 501, true);

    SaveNexusProcessed save;
    save.initialize();
    save.setProperty("InputWorkspace", workspace);
    std::string filename = "LoadNexusProcessed_test_log_filtering_survives_save_and_load.nxs";
    save.setPropertyValue("Filename", filename);
    filename = save.getPropertyValue("Filename");
    save.execute();
    LoadNexusProcessed load;
    load.initialize();
    load.setPropertyValue("Filename", filename);
    load.setPropertyValue("OutputWorkspace", output_ws);
    load.execute();

    auto reloadedWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(output_ws));
    // should not change as should be filtered as before
    check_log(reloadedWorkspace, "raw_uah_log", 429, 17, 99.4740982879);
    // should not change as should not be filtered as before
    check_log(reloadedWorkspace, "periods", 37, 1, 1);
    check_log(reloadedWorkspace, "period 1", 36, 505, true);
    check_log(reloadedWorkspace, "running", 72, 501, true);

    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void test_load_leanElasticPeakWorkspace() {
    // generate a lean elastic peak workspace with two peaks
    auto lpws = std::make_shared<LeanElasticPeaksWorkspace>();
    // add peaks
    LeanElasticPeak pk1(V3D(0.0, 0.0, 6.28319), 2.0);     // (100)
    LeanElasticPeak pk2(V3D(6.28319, 0.0, 6.28319), 1.0); // (110)
    lpws->addPeak(pk1);
    lpws->addPeak(pk2);

    // save the lean elastic peak workspace to temp
    std::string filename = "testLoadLeanElasticPeaksWorkspace.nxs";
    SaveNexusProcessed save;
    save.initialize();
    save.setProperty("InputWorkspace", lpws);
    save.setPropertyValue("Filename", filename);
    save.execute();

    // load it back with the loader
    LoadNexusProcessed load;
    load.initialize();
    load.setProperty("Filename", filename);
    load.setProperty("OutputWorkspace", "outws");
    load.execute();

    auto lpws_loaded = AnalysisDataService::Instance().retrieveWS<LeanElasticPeaksWorkspace>("outws");

    // confirm that the peaks are the same in original
    // and the loaded lean elastic peak workspace
    TS_ASSERT_EQUALS(lpws->getNumberPeaks(), lpws_loaded->getNumberPeaks());
    // --pk1
    TS_ASSERT_DELTA(lpws->getPeak(0).getWavelength(), pk1.getWavelength(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(0).getFinalEnergy(), pk1.getFinalEnergy(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(0).getH(), pk1.getH(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(0).getK(), pk1.getK(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(0).getL(), pk1.getL(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(0).getQLabFrame().X(), pk1.getQLabFrame().X(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(0).getQLabFrame().Y(), pk1.getQLabFrame().Y(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(0).getQLabFrame().Z(), pk1.getQLabFrame().Z(), 1e-5);
    // --pk2
    TS_ASSERT_DELTA(lpws->getPeak(1).getWavelength(), pk2.getWavelength(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(1).getFinalEnergy(), pk2.getFinalEnergy(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(1).getH(), pk2.getH(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(1).getK(), pk2.getK(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(1).getL(), pk2.getL(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(1).getQLabFrame().X(), pk2.getQLabFrame().X(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(1).getQLabFrame().Y(), pk2.getQLabFrame().Y(), 1e-5);
    TS_ASSERT_DELTA(lpws->getPeak(1).getQLabFrame().Z(), pk2.getQLabFrame().Z(), 1e-5);

    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void test_ws_run_title_failover_to_title() {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    const std::string testTitle = "TestTitle";
    const std::string filename = "TestTitleFailoverNexusProcessed.nxs";
    std::vector<double> dataEYX{0.0, 1.234, 2.468};

    // Set no workspace title
    TS_ASSERT_THROWS_NOTHING(alg->setProperty<std::vector<double>>("DataX", dataEYX));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty<std::vector<double>>("DataY", dataEYX));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", "test_CreateWorkspace"));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    TS_ASSERT(alg->isExecuted());
    MatrixWorkspace_const_sptr ws;

    TS_ASSERT_THROWS_NOTHING(ws = std::dynamic_pointer_cast<MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve("test_CreateWorkspace")));

    // Save workspace
    IAlgorithm_sptr save = AlgorithmManager::Instance().create("SaveNexusProcessed");
    save->initialize();
    TS_ASSERT(save->isInitialized());
    TS_ASSERT_THROWS_NOTHING(save->setProperty("InputWorkspace", "test_CreateWorkspace"));
    TS_ASSERT_THROWS_NOTHING(save->setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(save->setPropertyValue("Title", testTitle));
    TS_ASSERT_THROWS_NOTHING(save->execute());

    // Load workspace
    IAlgorithm_sptr load = AlgorithmManager::Instance().create("LoadNexusProcessed");
    load->initialize();
    TS_ASSERT(load->isInitialized());
    TS_ASSERT_THROWS_NOTHING(load->setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(load->setPropertyValue("OutputWorkspace", "test_failoverOutput"));
    TS_ASSERT_THROWS_NOTHING(load->execute());

    // Assert it failsover to the title supplied at save
    MatrixWorkspace_sptr outputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_failoverOutput");
    TS_ASSERT_EQUALS(testTitle, outputWs->getTitle());

    // Remove workspace and saved nexus file
    AnalysisDataService::Instance().remove("test_failoverOutput");
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove("test_CreateWorkspace"));
  }

  void test_ragged_Workspace2D() {
    // create ragged workspace
    MatrixWorkspace_sptr raggedWS = WorkspaceCreationHelper::create2DWorkspace(2, 1);

    // create and replace histograms with ragged ones
    raggedWS->setHistogram(0, Histogram(BinEdges{100., 200., 300., 400.}, Counts{1., 2., 3.}));
    raggedWS->setHistogram(1, Histogram(BinEdges{200., 400., 600.}, Counts{4., 5.}));

    // quick check of the input workspace
    TS_ASSERT(raggedWS->isRaggedWorkspace());
    TS_ASSERT_EQUALS(raggedWS->getNumberHistograms(), 2);

    TS_ASSERT_EQUALS(raggedWS->histogram(0).xMode(), Histogram::XMode::BinEdges);
    TS_ASSERT_EQUALS(raggedWS->histogram(1).xMode(), Histogram::XMode::BinEdges);
    TS_ASSERT_EQUALS(raggedWS->x(0).size(), 4);
    TS_ASSERT_EQUALS(raggedWS->x(1).size(), 3);
    TS_ASSERT_EQUALS(raggedWS->y(0).size(), 3);
    TS_ASSERT_EQUALS(raggedWS->y(1).size(), 2);

    doRaggedWorkspaceTest(raggedWS);
  }

  void test_ragged_Workspace2D_pointdata() {
    // create ragged workspace
    MatrixWorkspace_sptr raggedWS = WorkspaceCreationHelper::create2DWorkspacePoints(2, 1);

    // create and replace histograms with ragged ones
    raggedWS->setHistogram(0, Histogram(Points{100., 200., 300.}, Counts{1., 2., 3.}));
    raggedWS->setHistogram(1, Histogram(Points{200., 400.}, Counts{4., 5.}));

    // quick check of the input workspace
    TS_ASSERT(raggedWS->isRaggedWorkspace());
    TS_ASSERT_EQUALS(raggedWS->getNumberHistograms(), 2);

    TS_ASSERT_EQUALS(raggedWS->histogram(0).xMode(), Histogram::XMode::Points);
    TS_ASSERT_EQUALS(raggedWS->histogram(1).xMode(), Histogram::XMode::Points);
    TS_ASSERT_EQUALS(raggedWS->x(0).size(), 3);
    TS_ASSERT_EQUALS(raggedWS->x(1).size(), 2);
    TS_ASSERT_EQUALS(raggedWS->y(0).size(), 3);
    TS_ASSERT_EQUALS(raggedWS->y(1).size(), 2);

    doRaggedWorkspaceTest(raggedWS);
  }

  void test_ragged_EventWorkspace() {
    // create ragged workspace
    MatrixWorkspace_sptr raggedWS = WorkspaceCreationHelper::createEventWorkspace2(2, 1);

    // create and replace histograms with ragged ones
    raggedWS->setHistogram(0, BinEdges{100., 200., 300., 400.});
    raggedWS->setHistogram(1, BinEdges{200., 400., 600.});

    // quick check of the input workspace
    TS_ASSERT(raggedWS->isRaggedWorkspace());
    TS_ASSERT_EQUALS(raggedWS->getNumberHistograms(), 2);

    TS_ASSERT_EQUALS(raggedWS->x(0).size(), 4);
    TS_ASSERT_EQUALS(raggedWS->x(1).size(), 3);

    doRaggedWorkspaceTest(raggedWS);
  }

private:
  template <typename TYPE>
  void check_log(Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &logName, const int noOfEntries,
                 const int firstInterval, const TYPE firstValue) {
    TS_ASSERT(workspace.get());
    auto run = workspace->run();

    auto prop = run.getLogData(logName);
    TSM_ASSERT(logName + " Log was not found", prop);
    if (prop) {
      auto log = dynamic_cast<TimeSeriesProperty<TYPE> *>(prop);
      TSM_ASSERT(logName + " Log was not the expected type", log);
      if (log) {
        // middle value is invalid and is filtered out
        TSM_ASSERT_EQUALS(logName + " Log size not as expected", log->size(), noOfEntries);
        TSM_ASSERT_EQUALS(logName + " Log first interval not as expected", log->nthInterval(0).length().total_seconds(),
                          firstInterval);
        templated_equality_check(logName + " Log first value not as expected", log->nthValue(0), firstValue);
      }
    }
  }

  // There is also an explicit instantiation of this for doubles
  template <typename TYPE>
  void templated_equality_check(const std::string &message, const TYPE value, const TYPE refValue) {
    TSM_ASSERT_EQUALS(message, value, refValue);
  }

  void doHistoryTest(const MatrixWorkspace_sptr &matrix_ws) {
    const WorkspaceHistory history = matrix_ws->getHistory();
    int nalgs = static_cast<int>(history.size());
    TS_ASSERT_EQUALS(nalgs, 4);

    if (nalgs == 4) {
      TS_ASSERT_EQUALS(history[0]->name(), "LoadRaw");
      TS_ASSERT_EQUALS(history[1]->name(), "AlignDetectors");
      TS_ASSERT_EQUALS(history[2]->name(), "DiffractionFocussing");
      TS_ASSERT_EQUALS(history[3]->name(), "LoadNexusProcessed");
    }
  }

  /**
   * Do a few standard checks that are repeated in multiple tests of
   * partial event data loading
   *
   * @param alg initialized and parameterized load algorithm
   * @param nSpectra expected number of spectra
   * @param nHistory expected number of entries in the algorithm
   * history
   **/
  void doCommonEventLoadChecks(LoadNexusProcessed &alg, size_t nSpectra, size_t nHistory) {
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Test basic props of the ws
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve(output_ws));
    TS_ASSERT(workspace);
    if (!workspace)
      return;
    TS_ASSERT(workspace.get());

    EventWorkspace_sptr ews = std::dynamic_pointer_cast<EventWorkspace>(workspace);
    TS_ASSERT(ews);
    if (!ews)
      return;
    TS_ASSERT(ews.get());
    TS_ASSERT_EQUALS(ews->getNumberHistograms(), nSpectra);

    TS_ASSERT_EQUALS(ews->getHistory().size(), nHistory);
  }

  /*
   * Does a few common checks for using a single spectra property
   * such as spectrumMin or spectrumMax. Expects the algorithm
   * passed in to be configured for the test
   *
   * @param alg The configured algorithm to be executed
   * @param expectedSize The number of spectra which should be present
   */
  void doSpectrumMinOrMaxTest(LoadNexusProcessed &alg, const size_t expectedSize) {
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve(output_ws));
    TS_ASSERT(workspace.get());

    MatrixWorkspace_sptr matrix_ws = std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT(matrix_ws.get());

    // Testing the number of histograms
    TS_ASSERT_EQUALS(matrix_ws->getNumberHistograms(), expectedSize);

    // Test history
    doHistoryTest(matrix_ws);

    std::shared_ptr<const Mantid::Geometry::Instrument> inst = matrix_ws->getInstrument();
    TS_ASSERT_EQUALS(inst->getName(), "GEM");
    TS_ASSERT_EQUALS(inst->getSource()->getPos().Z(), -17);
  }

  /**
   * Does a few common checks for using spectra lists with/without
   * spectrum min and/or max being set. Expects the algorithm
   * passed in to be configured for this test.
   *
   * @param alg The configured algorithm to executed
   * @param expectedSpectra The IDs of the spectrum loaded which should
   * be present
   */
  void doSpectrumListTests(LoadNexusProcessed &alg, const std::vector<int> &expectedSpectra) {
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Test some aspects of the file
    Workspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(workspace = AnalysisDataService::Instance().retrieve(output_ws));
    TS_ASSERT(workspace.get());

    MatrixWorkspace_sptr matrix_ws = std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
    TS_ASSERT(matrix_ws.get());

    // Test spectrum numbers are as expected
    size_t index(0);
    int seenSpectra(0);
    for (const auto spectrum : expectedSpectra) {
      TS_ASSERT_EQUALS(matrix_ws->getSpectrum(index).getSpectrumNo(), spectrum);
      ++index;
      ++seenSpectra;
    }

    TS_ASSERT_EQUALS(seenSpectra, expectedSpectra.size());

    doHistoryTest(matrix_ws);

    std::shared_ptr<const Mantid::Geometry::Instrument> inst = matrix_ws->getInstrument();
    TS_ASSERT_EQUALS(inst->getName(), "GEM");
    TS_ASSERT_EQUALS(inst->getSource()->getPos().Z(), -17);
  }

  void writeTmpEventNexus() {
    // return;
    if (!m_savedTmpEventFile.empty() && Poco::File(m_savedTmpEventFile).exists())
      return;

    std::vector<std::vector<int>> groups(6);
    groups[0].emplace_back(9);
    groups[0].emplace_back(12);
    groups[1].emplace_back(5);
    groups[1].emplace_back(10);
    groups[2].emplace_back(20);
    groups[2].emplace_back(21);
    groups[3].emplace_back(10);
    groups[4].emplace_back(50);
    groups[5].emplace_back(15);
    groups[5].emplace_back(20);

    EventWorkspace_sptr ws = WorkspaceCreationHelper::createGroupedEventWorkspace(groups, 30, 1.0);
    ws->getSpectrum(4).clear();

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), groups.size());

    SaveNexusProcessed alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", std::dynamic_pointer_cast<Workspace>(ws));
    m_savedTmpEventFile = "LoadNexusProcessed_TmpEvent.nxs";
    alg.setPropertyValue("Filename", m_savedTmpEventFile);
    alg.setPropertyValue("Title", "Tmp test event workspace as NexusProcessed file");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get absolute path to the saved file
    m_savedTmpEventFile = alg.getPropertyValue("Filename");
  }

  void clearTmpEventNexus() {
    // remove saved/re-loaded test event data file
    if (!m_savedTmpEventFile.empty() && Poco::File(m_savedTmpEventFile).exists())
      Poco::File(m_savedTmpEventFile).remove();
  }

  void doTestLoadAndSaveHistogramWS(bool useXErrors = false, bool numericAxis = false, bool legacyXErrors = false) {
    // Test SaveNexusProcessed/LoadNexusProcessed on a histogram workspace with
    // x errors
    const std::string filename = "TestSaveAndLoadNexusProcessed.nxs";
    // Create histogram workspace with two spectra and 4 points
    std::vector<double> x1{1, 2, 3};
    std::vector<double> dx1{3, 2};
    std::vector<double> y1{1, 2};
    std::vector<double> x2{1, 2, 3};
    std::vector<double> dx2{3, 2};
    std::vector<double> y2{1, 2};
    MatrixWorkspace_sptr inputWs = WorkspaceFactory::Instance().create("Workspace2D", 2, x1.size(), y1.size());
    inputWs->mutableX(0) = x1;
    inputWs->mutableX(1) = x2;
    inputWs->mutableY(0) = y1;
    inputWs->mutableY(1) = y2;
    if (useXErrors) {
      inputWs->setPointStandardDeviations(0, dx1);
      inputWs->setPointStandardDeviations(1, dx2);
      if (legacyXErrors) {
        inputWs->dataDx(0).emplace_back(1);
        inputWs->dataDx(1).emplace_back(1);
      }
    }
    if (numericAxis) {
      auto numericAxis = std::make_unique<NumericAxis>(2);
      numericAxis->setValue(0, 10.0);
      numericAxis->setValue(1, 20.0);
      inputWs->replaceAxis(1, std::move(numericAxis));
    }

    // Save workspace
    auto save = AlgorithmManager::Instance().create("SaveNexusProcessed");
    save->initialize();
    TS_ASSERT(save->isInitialized());
    TS_ASSERT_THROWS_NOTHING(save->setProperty("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(save->setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(save->execute());

    // Load workspace
    auto load = AlgorithmManager::Instance().create("LoadNexusProcessed");
    load->initialize();
    TS_ASSERT(load->isInitialized());
    TS_ASSERT_THROWS_NOTHING(load->setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(load->setPropertyValue("OutputWorkspace", "output"));
    TS_ASSERT_THROWS_NOTHING(load->execute());

    // Check spectra in loaded workspace
    MatrixWorkspace_sptr outputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output");
    TS_ASSERT_EQUALS(1, outputWs->getSpectrum(0).getSpectrumNo());
    TS_ASSERT_EQUALS(2, outputWs->getSpectrum(1).getSpectrumNo());
    TS_ASSERT_EQUALS(inputWs->x(0), outputWs->x(0));
    TS_ASSERT_EQUALS(inputWs->x(1), outputWs->x(1));
    TS_ASSERT_EQUALS(inputWs->y(0), outputWs->y(0));
    TS_ASSERT_EQUALS(inputWs->y(1), outputWs->y(1));
    TS_ASSERT_EQUALS(inputWs->e(0), outputWs->e(0));
    TS_ASSERT_EQUALS(inputWs->e(1), outputWs->e(1));
    if (useXErrors) {
      TSM_ASSERT("Should have an x error", outputWs->hasDx(0));
      TS_ASSERT_EQUALS(dx1, outputWs->dx(0).rawData());
      TS_ASSERT_EQUALS(dx2, outputWs->dx(1).rawData());
    }

    // Axes
    auto axis1 = outputWs->getAxis(1);
    if (numericAxis) {
      TS_ASSERT(axis1->isNumeric());
      TS_ASSERT_DELTA(10.0, axis1->getValue(0), 1e-10);
      TS_ASSERT_DELTA(20.0, axis1->getValue(1), 1e-10);
    } else {
      TS_ASSERT(axis1->isSpectra());
      TS_ASSERT_DELTA(1.0, axis1->getValue(0), 1e-10);
      TS_ASSERT_DELTA(2.0, axis1->getValue(1), 1e-10);
    }

    // Remove workspace and saved nexus file
    AnalysisDataService::Instance().remove("output");
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void doTestLoadAndSavePointWS(bool useXErrors = false) {
    // Test SaveNexusProcessed/LoadNexusProcessed on a point-like workspace
    const std::string filename = "TestSaveAndLoadNexusProcessed.nxs";
    // Create histogram workspace with two spectra and 4 points
    std::vector<double> x1{1, 2, 3};
    std::vector<double> dx1{3, 2, 1};
    std::vector<double> y1{1, 2, 3};
    std::vector<double> x2{10, 20, 30};
    std::vector<double> dx2{30, 22, 10};
    std::vector<double> y2{10, 20, 30};
    MatrixWorkspace_sptr inputWs = WorkspaceFactory::Instance().create("Workspace2D", 2, x1.size(), y1.size());
    inputWs->mutableX(0) = x1;
    inputWs->mutableX(1) = x2;
    inputWs->mutableY(0) = y1;
    inputWs->mutableY(1) = y2;
    if (useXErrors) {
      inputWs->setPointStandardDeviations(0, dx1);
      inputWs->setPointStandardDeviations(1, dx2);
    }

    // Save workspace
    auto save = AlgorithmManager::Instance().create("SaveNexusProcessed");
    save->initialize();
    TS_ASSERT(save->isInitialized());
    TS_ASSERT_THROWS_NOTHING(save->setProperty("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(save->setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(save->execute());

    // Load workspace
    auto load = AlgorithmManager::Instance().create("LoadNexusProcessed");
    load->initialize();
    TS_ASSERT(load->isInitialized());
    TS_ASSERT_THROWS_NOTHING(load->setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(load->setPropertyValue("OutputWorkspace", "output"));
    TS_ASSERT_THROWS_NOTHING(load->execute());

    // Check spectra in loaded workspace
    MatrixWorkspace_sptr outputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output");
    TS_ASSERT_EQUALS(inputWs->x(0), outputWs->x(0));
    TS_ASSERT_EQUALS(inputWs->x(1), outputWs->x(1));
    TS_ASSERT_EQUALS(inputWs->y(0), outputWs->y(0));
    TS_ASSERT_EQUALS(inputWs->y(1), outputWs->y(1));
    TS_ASSERT_EQUALS(inputWs->e(0), outputWs->e(0));
    TS_ASSERT_EQUALS(inputWs->e(1), outputWs->e(1));
    if (useXErrors) {
      TSM_ASSERT("Should have an x error", outputWs->hasDx(0));
      TS_ASSERT_EQUALS(inputWs->dx(0).rawData(), outputWs->dx(0).rawData());
      TS_ASSERT_EQUALS(inputWs->dx(1).rawData(), outputWs->dx(1).rawData());
    }

    // Remove workspace and saved nexus file
    AnalysisDataService::Instance().remove("output");
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void doRaggedWorkspaceTest(MatrixWorkspace_sptr raggedWS) {
    // save the ragged workspace
    std::string filename = "testRaggedWorkspace.nxs";
    SaveNexusProcessed save;
    save.initialize();
    save.setProperty("InputWorkspace", raggedWS);
    save.setPropertyValue("Filename", filename);
    save.execute();

    // load it back with the loader
    LoadNexusProcessed load;
    load.setChild(true);
    load.initialize();
    load.setProperty("Filename", filename);
    load.setProperty("OutputWorkspace", "dummy");
    load.execute();

    Workspace_sptr loadedWS = load.getProperty("OutputWorkspace");

    // compare original to loaded workspace
    auto compare = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    compare->initialize();
    compare->setProperty<MatrixWorkspace_sptr>("Workspace1", raggedWS);
    compare->setProperty<Workspace_sptr>("Workspace2", loadedWS);
    ;
    compare->execute();
    if (compare->isExecuted()) {
      TS_ASSERT(compare->getProperty("Result"));
    } else {
      TS_ASSERT(false);
    }

    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  std::string testFile, output_ws;
  /// Saved using SaveNexusProcessed and re-used in several load event tests
  std::string m_savedTmpEventFile;
  static const EventType m_savedTmpType = TOF;
};

template <>
void LoadNexusProcessedTest::templated_equality_check(const std::string &message, const double value,
                                                      const double refValue) {
  TSM_ASSERT_DELTA(message, value, refValue, 1e-5);
}

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadNexusProcessedTestPerformance : public CxxTest::TestSuite {
public:
  void testHistogramWorkspace() {
    LoadNexusProcessed loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "PG3_733_focussed.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }

  void testPeaksWorkspace() {
    LoadNexusProcessed loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "24954_allpeaksbyhand.nxs");
    loader.setPropertyValue("OutputWorkspace", "peaks");
    TS_ASSERT(loader.execute());
  }
};
