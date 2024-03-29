// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidMDAlgorithms/BinMD.h"
#include "MantidMDAlgorithms/LoadMD.h"
#include "MantidMDAlgorithms/SaveMD2.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

/** Note: See the LoadMDTest class
 * for a more thorough test that does
 * a round-trip.
 */
class SaveMD2Test : public CxxTest::TestSuite {
public:
  void test_Init() {
    SaveMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() { do_test_exec(23, "SaveMD2Test.nxs"); }

  void test_exec_noEvents() { do_test_exec(0, "SaveMD2Test_noEvents.nxs"); }

  void test_MakeFileBacked() { do_test_exec(23, "SaveMD2Test.nxs", true); }

  void test_MakeFileBacked_then_UpdateFileBackEnd() { do_test_exec(23, "SaveMD2Test_updating.nxs", true, true); }

  void do_test_exec(size_t numPerBox, const std::string &filename, bool MakeFileBacked = false,
                    bool UpdateFileBackEnd = false) {

    // Make a 1D MDEventWorkspace
    MDEventWorkspace1Lean::sptr ws = MDEventsTestHelper::makeMDEW<1>(10, 0.0, 10.0, numPerBox);
    // Make sure it is split
    ws->splitBox();

    AnalysisDataService::Instance().addOrReplace("SaveMD2Test_ws", ws);

    ws->refreshCache();

    // There are this many boxes, so this is the max ID.
    TS_ASSERT_EQUALS(ws->getBoxController()->getMaxId(), 11);

    IMDEventWorkspace_sptr iws = ws;

    SaveMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "SaveMD2Test_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MakeFileBacked", MakeFileBacked));

    // clean up possible rubbish from the previous runs
    std::string fullName = alg.getPropertyValue("Filename");
    if (fullName != "")
      if (Poco::File(fullName).exists())
        Poco::File(fullName).remove();

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    std::string this_filename = alg.getProperty("Filename");
    TSM_ASSERT("File was indeed created", Poco::File(this_filename).exists());

    if (MakeFileBacked) {
      TSM_ASSERT("Workspace was made file-backed", ws->isFileBacked());
      TSM_ASSERT("File back-end no longer needs updating.", !ws->fileNeedsUpdating());
    }

    // Continue the test
    if (UpdateFileBackEnd)
      do_test_UpdateFileBackEnd(ws, filename);
    else {

      ws->clearFileBacked(false);
      if (Poco::File(this_filename).exists())
        Poco::File(this_filename).remove();
    }
  }

  /// Add some data and update the back-end
  void do_test_UpdateFileBackEnd(const MDEventWorkspace1Lean::sptr &ws, const std::string &filename) {
    size_t initial_numEvents = ws->getNPoints();
    TSM_ASSERT_EQUALS("Starting off with 230 events.", initial_numEvents, 230);

    // Add 100 events
    for (size_t i = 0; i < 100; i++) {
      MDLeanEvent<1> ev(1.0, 1.0);
      ev.setCenter(0, double(i) * 0.01 + 0.4);
      ws->addEvent(ev);
    }
    ws->splitAllIfNeeded(nullptr);
    ws->refreshCache();
    // Manually set the flag that the algo would set
    ws->setFileNeedsUpdating(true);

    TSM_ASSERT_EQUALS("Correctly added 100 events to original 230.", ws->getNPoints(), 230 + 100);

    SaveMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "SaveMD2Test_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UpdateFileBackEnd", true));
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Since there are 330 events, the file needs to be that big (or bigger).
    TS_ASSERT_LESS_THAN(330, ws->getBoxController()->getFileIO()->getFileLength());

    TSM_ASSERT("File back-end no longer needs updating.", !ws->fileNeedsUpdating());
    // Clean up file
    ws->clearFileBacked(false);
    std::string fullPath = alg.getPropertyValue("Filename");
    if (Poco::File(fullPath).exists())
      Poco::File(fullPath).remove();
  }

  void test_saveExpInfo() {
    std::string filename("MultiExperSaveMD2Test.nxs");
    // Make a 1D MDEventWorkspace
    MDEventWorkspace1Lean::sptr ws = MDEventsTestHelper::makeMDEW<1>(10, 0.0, 10.0, 2);
    // Make sure it is split
    ws->splitBox();

    Mantid::Geometry::Goniometer gon;
    gon.pushAxis("Psi", 0, 1, 0);
    // add experiment infos
    for (int i = 0; i < 80; i++) {
      ExperimentInfo_sptr ei = std::make_shared<ExperimentInfo>();
      ei->mutableRun().addProperty("Psi", double(i));
      ei->mutableRun().addProperty("Ei", 400.);
      ei->mutableRun().setGoniometer(gon, true);
      ws->addExperimentInfo(ei);
    }

    AnalysisDataService::Instance().addOrReplace("SaveMD2Test_ws", ws);

    ws->refreshCache();

    // There are this many boxes, so this is the max ID.
    TS_ASSERT_EQUALS(ws->getBoxController()->getMaxId(), 11);

    IMDEventWorkspace_sptr iws = ws;

    SaveMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "SaveMD2Test_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MakeFileBacked", "0"));
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    std::string this_filename = alg.getProperty("Filename");
    ws->clearFileBacked(false);
    if (Poco::File(this_filename).exists())
      Poco::File(this_filename).remove();
  }

  void test_saveOptions() {
    std::string filename("OptionsSaveMD2Test.nxs");
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.5, 2, 10, 10.0, 3.5, "histo2", 4.5);

    Mantid::Geometry::Goniometer gon;
    gon.pushAxis("Psi", 0, 1, 0);
    // add experiment infos
    for (int i = 0; i < 80; i++) {
      ExperimentInfo_sptr ei = std::make_shared<ExperimentInfo>();
      ei->mutableRun().addProperty("Psi", double(i));
      ei->mutableRun().addProperty("Ei", 400.);
      ei->mutableRun().setGoniometer(gon, true);
      ws->addExperimentInfo(ei);
    }

    AnalysisDataService::Instance().addOrReplace("SaveMD2Test_ws", ws);

    // Save everything
    SaveMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "SaveMD2Test_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    std::string this_filename = alg.getProperty("Filename");
    const auto fileSize = Poco::File(this_filename).getSize();
    if (Poco::File(this_filename).exists())
      Poco::File(this_filename).remove();

    // Only save data
    SaveMD2 alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("InputWorkspace", "SaveMD2Test_ws"));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("SaveHistory", "0"));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("SaveInstrument", "0"));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("SaveSample", "0"));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("SaveLogs", "0"));
    alg2.execute();
    TS_ASSERT(alg2.isExecuted());
    std::string this_filename2 = alg2.getProperty("Filename");
    const auto fileSize2 = Poco::File(this_filename2).getSize();
    if (Poco::File(this_filename2).exists())
      Poco::File(this_filename2).remove();

    // The second file should be small since less is saved
    TS_ASSERT_LESS_THAN(fileSize2, fileSize);
  }

  void test_saveAffine() {
    std::string filename("MDAffineSaveMD2Test.nxs");
    // Make a 4D MDEventWorkspace
    MDEventWorkspace4Lean::sptr ws = MDEventsTestHelper::makeMDEW<4>(10, 0.0, 10.0, 2);
    AnalysisDataService::Instance().addOrReplace("SaveMD2Test_ws", ws);

    // Bin data to get affine matrix
    BinMD balg;
    balg.initialize();
    balg.setProperty("InputWorkspace", "SaveMD2Test_ws");
    balg.setProperty("OutputWorkspace", "SaveMD2TestHisto_ws");
    balg.setProperty("AlignedDim0", "Axis2,0,10,10");
    balg.setProperty("AlignedDim1", "Axis0,0,10,5");
    balg.setProperty("AlignedDim2", "Axis1,0,10,5");
    balg.setProperty("AlignedDim3", "Axis3,0,10,2");
    balg.execute();

    SaveMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "SaveMD2TestHisto_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MakeFileBacked", "0"));
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    std::string this_filename = alg.getProperty("Filename");
    ws->clearFileBacked(false);
    if (Poco::File(this_filename).exists()) {
      Poco::File(this_filename).remove();
    }
  }

  void test_saveMaskedEventWorkspace() {
    // Create a masked workspace
    const std::string maskedWSName("SaveMDTest_maskedWS");
    MDEventsTestHelper::makeAnyMDEW<MDLeanEvent<2>, 2>(10, 0., 20., 1, maskedWSName);
    // Mask half of the workspace (and thus half of the events)
    FrameworkManager::Instance().exec("MaskMD", 6, "Workspace", "SaveMDTest_maskedWS", "Dimensions", "Axis0,Axis1",
                                      "Extents", "0,10,0,20");

    // Save the masked workspace
    const std::string saveFilename = "SaveMDTest_masked.nxs";
    SaveMD2 saveAlg;
    TS_ASSERT_THROWS_NOTHING(saveAlg.initialize())
    TS_ASSERT(saveAlg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(saveAlg.setPropertyValue("InputWorkspace", maskedWSName));
    TS_ASSERT_THROWS_NOTHING(saveAlg.setPropertyValue("Filename", saveFilename));
    TS_ASSERT_THROWS_NOTHING(saveAlg.setProperty("MakeFileBacked", "0"));
    saveAlg.execute();
    TS_ASSERT(saveAlg.isExecuted());

    // Load the masked workspace
    const std::string loadedWSName("SaveMDTest_loadedWS");
    LoadMD loadAlg;
    TS_ASSERT_THROWS_NOTHING(loadAlg.initialize())
    TS_ASSERT(loadAlg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(loadAlg.setPropertyValue("Filename", saveFilename));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("FileBackEnd", false));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setPropertyValue("OutputWorkspace", loadedWSName));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("MetadataOnly", false));
    TS_ASSERT_THROWS_NOTHING(loadAlg.setProperty("BoxStructureOnly", false));
    TS_ASSERT_THROWS_NOTHING(loadAlg.execute(););
    TS_ASSERT(loadAlg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(loadedWSName));
    TS_ASSERT(iws);
    if (!iws)
      return;

    // Test that number of events in the workspace is original events minus the
    // masked ones
    TS_ASSERT_EQUALS(iws->getNPoints(), 100 - 50);

    // Clean up
    const std::string this_filename = saveAlg.getProperty("Filename");
    if (Poco::File(this_filename).exists()) {
      Poco::File(this_filename).remove();
    }
  }

  /** Run SaveMD with the MDHistoWorkspace */
  void doTestHisto(const MDHistoWorkspace_sptr &ws) {
    std::string filename = "SaveMD2TestHisto.nxs";

    SaveMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    filename = alg.getPropertyValue("Filename");
    TSM_ASSERT("File was indeed created", Poco::File(filename).exists());
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void test_histo2() {
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.5, 2, 10, 10.0, 3.5, "histo2", 4.5);
    doTestHisto(ws);
  }
};

class SaveMD2TestPerformance : public CxxTest::TestSuite {
public:
  MDEventWorkspace3Lean::sptr ws;
  void setUp() override {
    // Make a 1D MDEventWorkspace
    ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
    ws->getBoxController()->setSplitInto(5);
    ws->getBoxController()->setSplitThreshold(2000);

    AnalysisDataService::Instance().addOrReplace("SaveMD2TestPerformance_ws", ws);

    FrameworkManager::Instance().exec("FakeMDEventData", 4, "InputWorkspace", "SaveMD2TestPerformance_ws",
                                      "UniformParams", "10000000");

    ws->refreshCache();
  }

  void test_exec_3D() {
    SaveMD2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "SaveMD2TestPerformance_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "SaveMD2TestPerformance.nxs"));
    alg.execute();
    TS_ASSERT(alg.isExecuted());
  }
};
