// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "SaveMD2Test.h"
#include "SaveMDTest.h"

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataObjects/BoxControllerNeXusIO.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDGridBox.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidKernel/Strings.h"
#include "MantidMDAlgorithms/LoadMD.h"
#include "MantidNexus/H5Util.h"
#include <H5Cpp.h>

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
using namespace H5;

class LoadMDTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  /// Compare two box controllers and assert each part of them.
  static void compareBoxControllers(BoxController &a, BoxController &b) {
    TS_ASSERT_EQUALS(a.getNDims(), b.getNDims());
    TS_ASSERT_EQUALS(a.getMaxDepth(), b.getMaxDepth());
    TS_ASSERT_EQUALS(a.getMaxId(), b.getMaxId());
    TS_ASSERT_EQUALS(a.getSplitThreshold(), b.getSplitThreshold());
    TS_ASSERT_EQUALS(a.getNumMDBoxes(), b.getNumMDBoxes());
    TS_ASSERT_EQUALS(a.getNumSplit(), b.getNumSplit());
    TS_ASSERT_EQUALS(a.getMaxNumMDBoxes(), b.getMaxNumMDBoxes());
    for (size_t d = 0; d < a.getNDims(); d++) {
      TS_ASSERT_EQUALS(a.getSplitInto(d), b.getSplitInto(d));
    }
  }

  //=================================================================================================================
  /** Compare two MDEventWorkspaces
   *
   * @param ws :: workspace to check
   * @param ws1 :: reference workspace
   * @param BoxStructureOnly :: true if you only compare the box structure and
   *ignore differences in event lists
   */
  template <typename MDE, size_t nd>
  static void do_compare_MDEW(std::shared_ptr<MDEventWorkspace<MDE, nd>> ws1,
                              std::shared_ptr<MDEventWorkspace<MDE, nd>> ws2, bool BoxStructureOnly = false) {
    TS_ASSERT(ws1->getBox());

    // Compare the initial to the final workspace
    TS_ASSERT_EQUALS(ws1->getBox()->getNumChildren(), ws2->getBox()->getNumChildren());
    if (!BoxStructureOnly) {
      TS_ASSERT_EQUALS(ws1->getNPoints(), ws2->getNPoints());
    }

    TS_ASSERT_EQUALS(ws1->getBoxController()->getMaxId(), ws2->getBoxController()->getMaxId());
    // Compare all the details of the box1 controllers
    compareBoxControllers(*ws1->getBoxController(), *ws2->getBoxController());

    // Compare every box1
    std::vector<IMDNode *> boxes;
    std::vector<IMDNode *> boxes1;

    ws1->getBox()->getBoxes(boxes, 1000, false);
    ws2->getBox()->getBoxes(boxes1, 1000, false);

    TS_ASSERT_EQUALS(boxes.size(), boxes1.size());
    if (boxes.size() != boxes1.size())
      return;

    for (size_t j = 0; j < boxes.size(); j++) {
      IMDNode *box1 = boxes[j];
      IMDNode *box2 = boxes1[j];

      // std::cout << "ID: " << box1->getId() << '\n';
      TS_ASSERT_EQUALS(box1->getID(), box2->getID());
      TS_ASSERT_EQUALS(box1->getDepth(), box2->getDepth());
      TS_ASSERT_EQUALS(box1->getNumChildren(), box2->getNumChildren());
      for (size_t i = 0; i < box1->getNumChildren(); i++) {
        TS_ASSERT_EQUALS(box1->getChild(i)->getID(), box2->getChild(i)->getID());
      }
      for (size_t d = 0; d < nd; d++) {
        TS_ASSERT_DELTA(box1->getExtents(d).getMin(), box2->getExtents(d).getMin(), 1e-5);
        TS_ASSERT_DELTA(box1->getExtents(d).getMax(), box2->getExtents(d).getMax(), 1e-5);
      }
      double vol = box1->getInverseVolume();
      if (vol == 0)
        vol = 1;
      TS_ASSERT(std::fabs(vol - box2->getInverseVolume()) / vol < 1e-3);

      if (!BoxStructureOnly) {
        TS_ASSERT_DELTA(box1->getSignal(), box2->getSignal(), 1e-3);
        TS_ASSERT_DELTA(box1->getErrorSquared(), box2->getErrorSquared(), 1e-3);
        TS_ASSERT_EQUALS(box1->getNPoints(), box2->getNPoints());
      }
      TS_ASSERT(box1->getBoxController());
      TS_ASSERT(box1->getBoxController() == ws1->getBoxController().get());

      // Are both MDGridBoxes ?
      MDGridBox<MDE, nd> *gridbox1 = dynamic_cast<MDGridBox<MDE, nd> *>(box1);
      MDGridBox<MDE, nd> *gridbox2 = dynamic_cast<MDGridBox<MDE, nd> *>(box2);
      if (gridbox1) {
        for (size_t d = 0; d < nd; d++) {
          TS_ASSERT_DELTA(gridbox1->getBoxSize(d), gridbox2->getBoxSize(d), 1e-4);
        }
      }

      // Are both MDBoxes (with events)
      MDBox<MDE, nd> *mdbox1 = dynamic_cast<MDBox<MDE, nd> *>(box1);
      MDBox<MDE, nd> *mdbox2 = dynamic_cast<MDBox<MDE, nd> *>(box2);
      if (mdbox1) {
        TS_ASSERT(mdbox2);
        if (!BoxStructureOnly) {
          const std::vector<MDE> &events1 = mdbox1->getConstEvents();
          const std::vector<MDE> &events2 = mdbox2->getConstEvents();
          TS_ASSERT_EQUALS(events1.size(), events2.size());
          if (events1.size() == events2.size() && events1.size() > 2) {
            // Check first and last event
            for (size_t i = 0; i < events1.size(); i += events1.size() - 1) {
              for (size_t d = 0; d < nd; d++) {
                TS_ASSERT_DELTA(events1[i].getCenter(d), events2[i].getCenter(d), 1e-4);
              }
              TS_ASSERT_DELTA(events1[i].getSignal(), events2[i].getSignal(), 1e-4);
              TS_ASSERT_DELTA(events1[i].getErrorSquared(), events2[i].getErrorSquared(), 1e-4);
            }
          }
          mdbox1->releaseEvents();
          mdbox2->releaseEvents();
        } // Don't compare if BoxStructureOnly
      } // is mdbox1
    }

    TS_ASSERT_EQUALS(ws1->getNumExperimentInfo(), ws2->getNumExperimentInfo());
    if (ws1->getNumExperimentInfo() == ws2->getNumExperimentInfo()) {
      for (uint16_t i = 0; i < ws1->getNumExperimentInfo(); i++) {
        ExperimentInfo_sptr ei1 = ws1->getExperimentInfo(i);
        ExperimentInfo_sptr ei2 = ws2->getExperimentInfo(i);
        // TS_ASSERT_EQUALS(ei1->getInstrument()->getName(),
        // ei2->getInstrument()->getName());
      }
    }
  }

  //=================================================================================================================
  template <size_t nd>
  void do_test_exec(bool FileBackEnd, bool deleteWorkspace = true, double memory = 0, bool BoxStructureOnly = false) {
    using MDE = MDLeanEvent<nd>;

    //------ Start by creating the file
    //----------------------------------------------
    // Make a 1D MDEventWorkspace
    std::shared_ptr<MDEventWorkspace<MDE, nd>> ws1 = MDEventsTestHelper::makeMDEW<nd>(10, 0.0, 10.0, 0);
    ws1->getBoxController()->setSplitThreshold(100);
    // Put in ADS so we can use fake data
    AnalysisDataService::Instance().addOrReplace("LoadMDTest_ws", std::dynamic_pointer_cast<IMDEventWorkspace>(ws1));
    FrameworkManager::Instance().exec("FakeMDEventData", 6, "InputWorkspace", "LoadMDTest_ws", "UniformParams", "10000",
                                      "RandomizeSignal", "1");
    //    FrameworkManager::Instance().exec("FakeMDEventData", 6,
    //        "InputWorkspace", "LoadMDTest_ws", "PeakParams", "30000, 5.0,
    //        0.01", "RandomizeSignal", "1");

    // ------ Make a ExperimentInfo entry ------------
    ExperimentInfo_sptr ei(new ExperimentInfo());
    ei->mutableRun().setProtonCharge(1.234);
    // TODO: Add instrument
    ws1->addExperimentInfo(ei);

    // -------- Save it ---------------
    SaveMD2 saver;
    TS_ASSERT_THROWS_NOTHING(saver.initialize())
    TS_ASSERT(saver.isInitialized())
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("InputWorkspace", "LoadMDTest_ws"));
    TS_ASSERT_THROWS_NOTHING(saver.setPropertyValue("Filename", "LoadMDTest" + Strings::toString(nd) + ".nxs"));

    // Retrieve the full path; delete any pre-existing file
    std::string filename = saver.getPropertyValue("Filename");
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();

    TS_ASSERT_THROWS_NOTHING(saver.execute(););
    TS_ASSERT(saver.isExecuted());

    //------ Now the loading -------------------------------------
    // Name of the output workspace.
    std::string outWSName("LoadMDTest_OutputWS");

    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileBackEnd", FileBackEnd));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Memory", memory));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MetadataOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BoxStructureOnly", BoxStructureOnly));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(outWSName));
    TS_ASSERT(iws);
    if (!iws)
      return;

    // Perform the full comparison
    auto ws = std::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<nd>, nd>>(iws);
    do_compare_MDEW(ws, ws1, BoxStructureOnly);

    // Look for the not-disk-cached-cause-they-are-too-small
    if (memory > 0) {
      // Force a flush of the read-write cache
      BoxController_sptr bc = ws->getBoxController();
      DiskBuffer *dbuf = bc->getFileIO();
      dbuf->flushCache();

      typename std::vector<API::IMDNode *> boxes;
      ws->getBox()->getBoxes(boxes, 1000, false);
      for (auto &boxIt : boxes) {
        MDBox<MDE, nd> *box = dynamic_cast<MDBox<MDE, nd> *>(boxIt);
        if (box) {
          TSM_ASSERT("Large box should not be in memory", box->getISaveable()->getDataMemorySize() == 0);
          TSM_ASSERT("Large box should be cached to disk", box->getISaveable()->wasSaved());
        }
      }
    }

    // Remove workspace from the data service.
    if (deleteWorkspace) {
      ws->clearFileBacked(false);
      AnalysisDataService::Instance().remove(outWSName);
      if (Poco::File(filename).exists())
        Poco::File(filename).remove();
    }
  }

  /** Follow up test that:
   *  - Modifies the data in a couple of ways
   *  - Saves AGAIN to update a file back end
   *  - Re-loads to a brand new workspace and compares everything. */
  template <size_t nd> void do_test_UpdateFileBackEnd() {
    std::string outWSName("LoadMDTest_OutputWS");
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(outWSName));
    TS_ASSERT(iws);
    if (!iws)
      return;
    std::shared_ptr<MDEventWorkspace<MDLeanEvent<nd>, nd>> ws2 =
        std::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<nd>, nd>>(iws);

    // Modify that by adding some boxes
    MDGridBox<MDLeanEvent<nd>, nd> *box = dynamic_cast<MDGridBox<MDLeanEvent<nd>, nd> *>(ws2->getBox());
    // Now there are 1000+1000 boxes (the box 12 was split into 10x10x10)
    box->splitContents(12);

    // And add an ExperimentInfo thingie
    ExperimentInfo_sptr ei(new ExperimentInfo());
    ei->mutableRun().setProtonCharge(2.345);
    iws->addExperimentInfo(ei);

    // Add one event using addEvent(). The event will need to be written out to
    // disk too.
    MDLeanEvent<nd> ev(1.0, 1.0);
    for (size_t d = 0; d < nd; d++)
      ev.setCenter(d, 0.5);
    box->addEvent(ev);
    // CHANGE from AB: 20/01/2013: you have to split to identify changes!
    box->splitAllIfNeeded(nullptr);

    // Modify a different box by accessing the events
    MDBox<MDLeanEvent<nd>, nd> *box8 = dynamic_cast<MDBox<MDLeanEvent<nd>, nd> *>(box->getChild(8));
    std::vector<MDLeanEvent<nd>> &events = box8->getEvents();
    // Add 10 to this signal
    signal_t newSignal = events[0].getSignal() + 10.0;
    events[0].setSignal(float(newSignal));
    box8->releaseEvents();

    //    // Modify a third box by adding an event
    //    MDBox<MDLeanEvent<nd>,nd> * box17 =
    //    dynamic_cast<MDBox<MDLeanEvent<nd>,nd>*>(box->getChild(17));
    //    std::vector<MDLeanEvent<nd> > & events17 = box17->getEvents();
    //    MDLeanEvent<nd> ev_new(1.0, 1.0);
    //    box->addEvent( ev_new );
    //    box17->releaseEvents();

    ws2->refreshCache();

    // There are now 2 more events
    TS_ASSERT_EQUALS(ws2->getNPoints(), 10001);

    // There are some new boxes that are not cached to disk at this point.
    // Save it again.
    SaveMD2 saver;
    TS_ASSERT_THROWS_NOTHING(saver.initialize())
    TS_ASSERT(saver.isInitialized())
    TS_ASSERT_THROWS_NOTHING(saver.setPropertyValue("InputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(saver.setPropertyValue("Filename", ""));
    TS_ASSERT_THROWS_NOTHING(saver.setPropertyValue("UpdateFileBackEnd", "1"));
    TS_ASSERT_THROWS_NOTHING(saver.execute(););
    TS_ASSERT(saver.isExecuted());

    // Now we look at the file that's currently open
    auto loader = dynamic_cast<BoxControllerNeXusIO *>(ws2->getBoxController()->getFileIO());
    TS_ASSERT(loader);
    if (!loader)
      return;

    ::NeXus::File *file = loader->getFile();
    TSM_ASSERT_LESS_THAN("The event_data field in the file must be at least 10002 long.", 10002,
                         file->getInfo().dims[0]);

    // The file should have been modified but that's tricky to check directly.
    std::string filename = ws2->getBoxController()->getFileIO()->getFileName();
    // Now we re-re-load it!
    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileBackEnd", false));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "reloaded_again"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("reloaded_again"));
    std::shared_ptr<MDEventWorkspace<MDLeanEvent<nd>, nd>> ws3 =
        std::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<nd>, nd>>(iws);
    TS_ASSERT(ws3);
    if (!ws3)
      return;
    ws3->refreshCache();

    // Perform the full comparison of the second and 3rd loaded workspaces
    do_compare_MDEW(ws2, ws3);

    // break the connection between the workspace and the file, ws2 is file
    // backed
    ws2->clearFileBacked(false);
    AnalysisDataService::Instance().remove(outWSName);
  }

  //=================================================================================================================

  /// Load directly to memory
  void test_exec_1D() { do_test_exec<1>(false); }

  /// Run the loading but keep the events on file and load on demand
  void test_exec_1D_with_file_backEnd() { do_test_exec<1>(true); }

  /// Load directly to memory
  void test_exec_3D() { do_test_exec<3>(false); }

  /// Run the loading but keep the events on file and load on demand
  void test_exec_3D_with_FileBackEnd() { do_test_exec<3>(true); }

  /// Run the loading but keep the events on file and load on demand
  void test_exec_3D_with_FileBackEnd_andSmallBuffer() { do_test_exec<3>(true, true, 1.0); }

  /** Use the file back end,
   * then change it and save to update the file at the back end.
   */
  void test_exec_3D_with_FileBackEnd_then_update_SaveMDEW() {
    std::cout << "Starting the first step\n";
    do_test_exec<3>(true, false);
    std::cout << "\nStarting the update step\n";
    do_test_UpdateFileBackEnd<3>();
  }

  /// Only load the box structure, no events
  void test_exec_3D_BoxStructureOnly() { do_test_exec<3>(false, true, 0.0, true); }

  //=================================================================================================================

  void testMetaDataOnly() {
    //------ Start by creating the file
    //----------------------------------------------
    // Make a 1D MDEventWorkspace
    std::shared_ptr<MDEventWorkspace<MDLeanEvent<2>, 2>> ws1 = MDEventsTestHelper::makeMDEW<2>(10, 0.0, 10.0, 0);
    ws1->getBoxController()->setSplitThreshold(100);
    AnalysisDataService::Instance().addOrReplace("LoadMDTest_ws", std::dynamic_pointer_cast<IMDEventWorkspace>(ws1));

    // Save it
    SaveMD2 saver;
    TS_ASSERT_THROWS_NOTHING(saver.initialize())
    TS_ASSERT(saver.isInitialized())
    TS_ASSERT_THROWS_NOTHING(saver.setProperty("InputWorkspace", "LoadMDTest_ws"));
    TS_ASSERT_THROWS_NOTHING(saver.setPropertyValue("Filename", "LoadMDTest2.nxs"));
    // clean up possible rubbish from the previous runs
    std::string fullName = saver.getPropertyValue("Filename");
    if (Poco::File(fullName).exists())
      Poco::File(fullName).remove();

    TS_ASSERT_THROWS_NOTHING(saver.execute(););
    TS_ASSERT(saver.isExecuted());

    // Retrieve the full path
    std::string filename = saver.getPropertyValue("Filename");

    //------ Now the loading -------------------------------------
    // Name of the output workspace.
    std::string outWSName("LoadMDTest_OutputWS");
    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileBackEnd", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Memory", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MetadataOnly", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    std::shared_ptr<MDEventWorkspace<MDLeanEvent<2>, 2>> ws =
        AnalysisDataService::Instance().retrieveWS<MDEventWorkspace<MDLeanEvent<2>, 2>>(outWSName);

    TSM_ASSERT_EQUALS("Should have no events!", 0, ws->getNPoints());
    TSM_ASSERT_EQUALS("Wrong number of dimensions", 2, ws->getNumDims());

    AnalysisDataService::Instance().remove(outWSName);
    try {
      if (Poco::File(filename).exists())
        Poco::File(filename).remove();
    } catch (...) { /* ignore windows error */
    }
  }

  /** Run SaveMD v1 with the MDHistoWorkspace */
  void doTestHistoV1(const MDHistoWorkspace_sptr &ws) {
    std::string filename = "SaveMDTestHisto.nxs";

    SaveMD alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize())
    TS_ASSERT(alg1.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg1.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("Filename", filename));
    alg1.execute();
    TS_ASSERT(alg1.isExecuted());
    filename = alg1.getPropertyValue("Filename");

    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "loaded"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MDHistoWorkspace_sptr newWS;
    TS_ASSERT_THROWS_NOTHING(newWS = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>("loaded"));
    TS_ASSERT(newWS);
    if (!newWS)
      return;

    TS_ASSERT_EQUALS(ws->getNPoints(), newWS->getNPoints());
    TS_ASSERT_EQUALS(ws->getNumDims(), newWS->getNumDims());
    TS_ASSERT_EQUALS(ws->displayNormalization(), newWS->displayNormalization());
    for (size_t i = 0; i < ws->getNPoints(); i++) {
      TS_ASSERT_DELTA(ws->getSignalAt(i), newWS->getSignalAt(i), 1e-6);
      TS_ASSERT_DELTA(ws->getErrorAt(i), newWS->getErrorAt(i), 1e-6);
      TS_ASSERT_DELTA(ws->getNumEventsAt(i), newWS->getNumEventsAt(i), 1e-6);
      TS_ASSERT_EQUALS(ws->getIsMaskedAt(i), newWS->getIsMaskedAt(i));
    }

    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  /** Run SaveMD2 with the MDHistoWorkspace */
  void doTestHisto(const MDHistoWorkspace_sptr &ws) {
    std::string filename = "SaveMD2TestHisto.nxs";

    SaveMD2 alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize())
    TS_ASSERT(alg1.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg1.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("Filename", filename));
    alg1.execute();
    TS_ASSERT(alg1.isExecuted());
    filename = alg1.getPropertyValue("Filename");

    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "loaded"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MDHistoWorkspace_sptr newWS;
    TS_ASSERT_THROWS_NOTHING(newWS = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>("loaded"));
    TS_ASSERT(newWS);
    if (!newWS)
      return;

    TS_ASSERT_EQUALS(ws->getNPoints(), newWS->getNPoints());
    TS_ASSERT_EQUALS(ws->getNumDims(), newWS->getNumDims());
    TS_ASSERT_EQUALS(ws->displayNormalization(), newWS->displayNormalization());
    for (size_t i = 0; i < ws->getNPoints(); i++) {
      TS_ASSERT_DELTA(ws->getSignalAt(i), newWS->getSignalAt(i), 1e-6);
      TS_ASSERT_DELTA(ws->getErrorAt(i), newWS->getErrorAt(i), 1e-6);
      TS_ASSERT_DELTA(ws->getNumEventsAt(i), newWS->getNumEventsAt(i), 1e-6);
      TS_ASSERT_EQUALS(ws->getIsMaskedAt(i), newWS->getIsMaskedAt(i));
    }

    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void test_histo1D() {
    Mantid::coord_t min(-10.0);
    Mantid::coord_t max(10.0);
    Mantid::Geometry::GeneralFrame frame(Mantid::Geometry::GeneralFrame::GeneralFrameDistance, "m");
    std::vector<Geometry::IMDDimension_sptr> dims(
        1, std::make_shared<Geometry::MDHistoDimension>("X", "x", frame, min, max, 5));
    MDHistoWorkspace_sptr ws = std::make_shared<MDHistoWorkspace>(dims, API::VolumeNormalization);
    ws->setTo(1.0, 1.0, 1);
    doTestHistoV1(ws);
    doTestHisto(ws);
  }

  void test_histo2D() {
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.5, 2, 10, 10.0, 3.5, "histo2", 4.5);
    doTestHistoV1(ws);
    doTestHisto(ws);
  }

  void test_histo3D() {
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.5, 3, 4, 10.0, 3.5, "histo3", 4.5);
    doTestHistoV1(ws);
    doTestHisto(ws);
  }

  /// More of an integration test as it uses both load and save.
  void test_save_and_load_special_coordinates_MDEventWorkspace() {
    Mantid::Geometry::QSample frame;
    MDEventWorkspace1Lean::sptr mdeventWS = MDEventsTestHelper::makeMDEWWithFrames<1>(10, 0.0, 10.0, frame, 2);
    const Mantid::Kernel::SpecialCoordinateSystem appliedCoordinateSystem =
        Mantid::Kernel::SpecialCoordinateSystem::QSample;
    mdeventWS->setCoordinateSystem(appliedCoordinateSystem);

    auto loadedWS = testSaveAndLoadWorkspace(mdeventWS, "MDEventWorkspace");
    // Check that the special coordinate system is the same before the save-load
    // cycle.
    TS_ASSERT_EQUALS(appliedCoordinateSystem, loadedWS->getSpecialCoordinateSystem());
  }

  // backwards-compatability check for coordinate in log
  void test_load_coordinate_system_MDEventWorkspace_from_experiment_info() {
    Mantid::Geometry::QSample frame;
    MDEventWorkspace1Lean::sptr mdeventWS = MDEventsTestHelper::makeMDEWWithFrames<1>(10, 0.0, 10.0, frame, 2);
    const Mantid::Kernel::SpecialCoordinateSystem appliedCoordinateSystem =
        Mantid::Kernel::SpecialCoordinateSystem::QSample;
    mdeventWS->setCoordinateSystem(appliedCoordinateSystem);

    // Create a log in the first experiment info to simulated an old version of
    // the file
    auto expt0 = mdeventWS->getExperimentInfo(0);
    expt0->mutableRun().addProperty("CoordinateSystem", static_cast<int>(appliedCoordinateSystem));

    const bool rmCoordField(true);
    auto loadedWS = testSaveAndLoadWorkspace(mdeventWS, "MDEventWorkspace", rmCoordField);
    // Check that the special coordinate system is the same before the save-load
    // cycle.
    TS_ASSERT_EQUALS(appliedCoordinateSystem, loadedWS->getSpecialCoordinateSystem());
  }

  void test_save_and_load_special_coordinates_MDHistoWorkspace() {
    Mantid::Geometry::QSample frame;
    auto mdhistoWS = MDEventsTestHelper::makeFakeMDHistoWorkspaceWithMDFrame(2.5, 2, frame, 10, 10.0, 3.5, "", 4.5);
    const Mantid::Kernel::SpecialCoordinateSystem appliedCoordinateSystem =
        Mantid::Kernel::SpecialCoordinateSystem::QSample;
    mdhistoWS->setCoordinateSystem(appliedCoordinateSystem);

    auto loadedWS = testSaveAndLoadWorkspace(mdhistoWS, "MDHistoWorkspace");
    // Check that the special coordinate system is the same before the save-load
    // cycle.
    TS_ASSERT_EQUALS(appliedCoordinateSystem, loadedWS->getSpecialCoordinateSystem());
  }

  // backwards-compatability check for coordinate in log
  void test_load_coordinate_system_MDHistoWorkspace_from_experiment_info() {
    Mantid::Geometry::QSample frame;
    auto mdhistoWS = MDEventsTestHelper::makeFakeMDHistoWorkspaceWithMDFrame(2.5, 2, frame, 10, 10.0, 3.5, "", 4.5);
    const Mantid::Kernel::SpecialCoordinateSystem appliedCoordinateSystem =
        Mantid::Kernel::SpecialCoordinateSystem::QSample;
    mdhistoWS->setCoordinateSystem(appliedCoordinateSystem);

    // Create a log in the first experiment info to simulated an old version of
    // the file
    auto expt0 = mdhistoWS->getExperimentInfo(0);
    expt0->mutableRun().addProperty("CoordinateSystem", static_cast<int>(appliedCoordinateSystem));

    const bool rmCoordField(true);
    auto loadedWS = testSaveAndLoadWorkspace(mdhistoWS, "MDHistoWorkspace", rmCoordField);
    // Check that the special coordinate system is the same before the save-load
    // cycle.
    TS_ASSERT_EQUALS(appliedCoordinateSystem, loadedWS->getSpecialCoordinateSystem());
  }

  Mantid::API::IMDWorkspace_sptr testSaveAndLoadWorkspace(const Mantid::API::IMDWorkspace_sptr &inputWS,
                                                          std::string rootGroup, const bool rmCoordField = false) {
    const std::string fileName = "SaveMDSpecialCoordinatesTest.nxs";
    SaveMD2 saveAlg;
    saveAlg.setChild(true);
    saveAlg.initialize();
    saveAlg.setProperty("InputWorkspace", inputWS);
    saveAlg.setPropertyValue("Filename", fileName);
    saveAlg.execute();
    TS_ASSERT(saveAlg.isExecuted());
    std::string this_fileName = saveAlg.getProperty("Filename");

    if (rmCoordField) {
      H5File h5file(this_fileName.c_str(), H5F_ACC_RDWR);
      H5Util::deleteObjectLink(h5file, rootGroup + "/coordinate_system");
    }

    LoadMD loadAlg;
    loadAlg.setChild(true);
    loadAlg.initialize();
    loadAlg.isInitialized();
    loadAlg.setPropertyValue("Filename", fileName);
    loadAlg.setProperty("FileBackEnd", false);
    loadAlg.setPropertyValue("OutputWorkspace", "_unused_for_child");
    loadAlg.execute();
    TS_ASSERT(loadAlg.isExecuted());

    if (Poco::File(this_fileName).exists()) {
      Poco::File(this_fileName).remove();
    }

    return loadAlg.getProperty("OutputWorkspace");
  }

  void test_loadAffine() {
    std::string filename("SaveMDAffineTest.nxs");
    // Make a 4D MDEventWorkspace
    MDEventWorkspace4Lean::sptr ws = MDEventsTestHelper::makeMDEW<4>(10, 0.0, 10.0, 2);
    AnalysisDataService::Instance().addOrReplace("SaveMDAffineTest_ws", ws);

    // Bin data to get affine matrix
    BinMD balg;
    balg.initialize();
    balg.setProperty("InputWorkspace", "SaveMDAffineTest_ws");
    balg.setProperty("OutputWorkspace", "SaveMDAffineTestHisto_ws");
    balg.setProperty("AlignedDim0", "Axis2,0,10,10");
    balg.setProperty("AlignedDim1", "Axis0,0,10,5");
    balg.setProperty("AlignedDim2", "Axis1,0,10,5");
    balg.setProperty("AlignedDim3", "Axis3,0,10,2");
    balg.execute();

    SaveMD2 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "SaveMDAffineTestHisto_ws");
    alg.setPropertyValue("Filename", filename);
    alg.setProperty("MakeFileBacked", "0");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    std::string this_filename = alg.getProperty("Filename");

    LoadMD loadAlg;
    loadAlg.initialize();
    loadAlg.isInitialized();
    loadAlg.setPropertyValue("Filename", filename);
    loadAlg.setProperty("FileBackEnd", false);
    loadAlg.setPropertyValue("OutputWorkspace", "reloaded_affine");
    loadAlg.execute();
    TS_ASSERT(loadAlg.isExecuted());

    // Check the affine matrix over at a couple of locations
    MDHistoWorkspace_sptr newWS = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>("reloaded_affine");
    Matrix<coord_t> affMat = newWS->getTransformToOriginal()->makeAffineMatrix();
    TS_ASSERT_EQUALS(affMat[0][1], 1.0);
    TS_ASSERT_EQUALS(affMat[2][0], 1.0);

    if (Poco::File(this_filename).exists()) {
      Poco::File(this_filename).remove();
    }

    AnalysisDataService::Instance().remove("SaveMDAffineTest_ws");
    AnalysisDataService::Instance().remove("SaveMDAffineTestHisto_ws");
    AnalysisDataService::Instance().remove("reloaded_affine");
  }

  void test_loadHistory() {
    std::string filename("History.nxs");
    // Make a 4D MDEventWorkspace
    MDEventWorkspace4Lean::sptr ws = MDEventsTestHelper::makeMDEW<4>(10, 0.0, 10.0, 2);
    AnalysisDataService::Instance().addOrReplace("HistoryEvTest_ws", ws);

    // Bin data
    BinMD balg;
    balg.initialize();
    balg.setProperty("InputWorkspace", "HistoryEvTest_ws");
    balg.setProperty("OutputWorkspace", "HistoryHiTest_ws");
    balg.setProperty("AlignedDim0", "Axis2,0,10,10");
    balg.setProperty("AlignedDim1", "Axis0,0,10,5");
    balg.setProperty("AlignedDim2", "Axis1,0,10,5");
    balg.setProperty("AlignedDim3", "Axis3,0,10,2");
    balg.execute();

    SaveMD2 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "HistoryHiTest_ws");
    alg.setPropertyValue("Filename", filename);
    alg.setProperty("MakeFileBacked", "0");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    std::string this_filename = alg.getProperty("Filename");

    LoadMD loadAlg;
    loadAlg.initialize();
    loadAlg.isInitialized();
    loadAlg.setPropertyValue("Filename", this_filename);
    loadAlg.setProperty("FileBackEnd", false);
    loadAlg.setPropertyValue("OutputWorkspace", "withHisto");
    loadAlg.execute();
    TS_ASSERT(loadAlg.isExecuted());

    // Check the affine matrix over at a couple of locations
    MDHistoWorkspace_sptr newWSh = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>("withHisto");

    // BinMD and LoadMD should be in the history
    TS_ASSERT_EQUALS(newWSh->getHistory().size(), 2);

    loadAlg.initialize();
    loadAlg.isInitialized();
    loadAlg.setPropertyValue("Filename", this_filename);
    loadAlg.setProperty("FileBackEnd", false);
    loadAlg.setPropertyValue("OutputWorkspace", "noHisto");
    loadAlg.setProperty("LoadHistory", false);
    loadAlg.execute();
    TS_ASSERT(loadAlg.isExecuted());

    // Check the affine matrix over at a couple of locations
    MDHistoWorkspace_sptr newWSnh = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>("noHisto");

    // Only LoadMD should be in the history
    TS_ASSERT_EQUALS(newWSnh->getHistory().size(), 1);

    if (Poco::File(this_filename).exists()) {
      Poco::File(this_filename).remove();
    }

    AnalysisDataService::Instance().remove("HistoryEvTest_ws");
    AnalysisDataService::Instance().remove("HistoryHiTest_ws");
    AnalysisDataService::Instance().remove("withHisto");
    AnalysisDataService::Instance().remove("noHisto");
  }

  void test_MDEventWorkspace_contains_normalization_flags() {
    // Arrange
    std::string filename("SaveMDEventNormalizationFlagTest.nxs");
    MDEventWorkspace4Lean::sptr ws = MDEventsTestHelper::makeMDEW<4>(10, 0.0, 10.0, 2);
    AnalysisDataService::Instance().addOrReplace("SaveMDEventNormalizationFlagTest_ws", ws);

    auto eventNormalization = Mantid::API::NoNormalization;
    auto histoNormalization = Mantid::API::NumEventsNormalization;
    ws->setDisplayNormalization(eventNormalization);
    ws->setDisplayNormalizationHisto(histoNormalization);

    // Act
    SaveMD2 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "SaveMDEventNormalizationFlagTest_ws");
    alg.setPropertyValue("Filename", filename);
    alg.setProperty("MakeFileBacked", "0");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    std::string this_filename = alg.getProperty("Filename");

    LoadMD loadAlg;
    loadAlg.initialize();
    loadAlg.isInitialized();
    loadAlg.setPropertyValue("Filename", filename);
    loadAlg.setProperty("FileBackEnd", false);
    loadAlg.setPropertyValue("OutputWorkspace", "reloaded_MDEventNormalization");
    loadAlg.execute();
    TS_ASSERT(loadAlg.isExecuted());
    auto newWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("reloaded_MDEventNormalization");

    // Assert
    TSM_ASSERT_EQUALS("Should have no normalization set", newWS->displayNormalization(), eventNormalization);
    TSM_ASSERT_EQUALS("Should have number events normalization set", newWS->displayNormalizationHisto(),
                      histoNormalization);

    // Clean up
    if (Poco::File(this_filename).exists()) {
      Poco::File(this_filename).remove();
    }

    AnalysisDataService::Instance().remove("SaveMDEventNormalizationFlagTest_ws");
    AnalysisDataService::Instance().remove("reloaded_MDEventNormalization");
  }

  void test_loading_legacy_QSample_file_with_MDEvent_is_being_corrected() {
    // Arrange
    std::string filename("MDEvent_wo_MDFrames_w_QSample_flag.nxs");
    std::string outWSName("LoadMD_legacy_test_file");

    // Act
    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileBackEnd", false));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MetadataOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(outWSName));
    TS_ASSERT(iws);

    if (!iws) {
      return;
    }

    // Assert
    // We expect the first three dimensions to be QSample and the fourth to be a
    // GeneralFrame
    for (int index = 0; index < 3; ++index) {
      TSM_ASSERT_EQUALS("The first three dimension should contain a QSample Frame",
                        iws->getDimension(index)->getMDFrame().name(), Mantid::Geometry::QSample::QSampleName);
    }

    TSM_ASSERT_EQUALS("The fourth dimension should contain a General Frame", iws->getDimension(3)->getMDFrame().name(),
                      Mantid::Geometry::GeneralFrame::GeneralFrameName);
    // Clean up
    if (iws) {
      AnalysisDataService::Instance().remove(outWSName);
    }
  }

  void test_loading_legacy_HKL_file_with_MDHisto_is_being_corrected() {
    // Arrange
    std::string filename("MDHisto_wo_MDFrames_w_HKL_flag.nxs");
    std::string outWSName("LoadMD_legacy_test_file");

    // Act
    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileBackEnd", false));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MetadataOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDHistoWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(outWSName));
    TS_ASSERT(iws);

    if (!iws) {
      return;
    }

    // Assert
    // We expect the first three dimensions to be QSample and the fourth to be a
    // GeneralFrame
    for (int index = 0; index < 3; ++index) {
      TSM_ASSERT_EQUALS("The first three dimension should contain an HKL Frame",
                        iws->getDimension(index)->getMDFrame().name(), Mantid::Geometry::HKL::HKLName);
    }

    TSM_ASSERT_EQUALS("The fourth dimension should contain a General Frame", iws->getDimension(3)->getMDFrame().name(),
                      Mantid::Geometry::GeneralFrame::GeneralFrameName);
    // Clean up
    if (iws) {
      AnalysisDataService::Instance().remove(outWSName);
    }
  }

  void test_loading_legacy_HKL_file_with_MDHisto_with_incorrect_HKLUnits_doesnt_make_changes() {
    // Arrange
    std::string filename("MDHisto_wo_MDFrames_w_HKL_flag_w_invalid_HKLUnits.nxs");
    std::string outWSName("LoadMD_legacy_test_file");

    // Act
    LoadMD alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileBackEnd", false));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MetadataOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDHistoWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(outWSName));
    TS_ASSERT(iws);

    if (!iws) {
      return;
    }

    // Assert
    // We expect the first three dimensions to be QSample and the fourth to be a
    // GeneralFrame
    for (int index = 0; index < 3; ++index) {
      TSM_ASSERT_EQUALS("The first three dimension should contain an Unkown frame",
                        iws->getDimension(index)->getMDFrame().name(),
                        Mantid::Geometry::UnknownFrame::UnknownFrameName);
    }

    TSM_ASSERT_EQUALS("The fourth dimension should contain an Unkown frame", iws->getDimension(3)->getMDFrame().name(),
                      Mantid::Geometry::UnknownFrame::UnknownFrameName);
    // Clean up
    if (iws) {
      AnalysisDataService::Instance().remove(outWSName);
    }
  }

  void test_loading_MD_with_missing_parameter_map() {
    // Arrange
    std::string filename("md_missing_paramater_map.nxs");
    std::string outWSName("LoadMD_md_missing_paramater_map");

    // Act
    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileBackEnd", false));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MetadataOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDHistoWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(outWSName));
    TS_ASSERT(iws);

    if (!iws) {
      return;
    }

    // Assert
    // If the missing parameter map is not correctly recreated then these values
    // will be wrong
    auto &detectorInfo = iws->getExperimentInfo(0)->detectorInfo();
    TS_ASSERT_DELTA(detectorInfo.twoTheta(0), 0.8859675, 1e-7);
    TS_ASSERT_DELTA(detectorInfo.azimuthal(0), -3.0639441, 1e-7);

    // Clean up
    if (iws) {
      AnalysisDataService::Instance().remove(outWSName);
    }
  }

  void test_simple_file() {
    return; // uncomment for debugging
    std::string filename("/tmp/LoadMDTest1.nxs");
    std::string outWSName("outWS");
    bool fileBackEnd(false);
    double memory(0.0); // don't allocate in-cache memory for the file backend
    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileBackEnd", fileBackEnd));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Memory", memory));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MetadataOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BoxStructureOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(outWSName));
    TS_ASSERT(iws);
  }
};
