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

#include <cxxtest/TestSuite.h>

#include <hdf5.h>

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class LoadMDTest : public CxxTest::TestSuite {
public:
  void test_simple_file() {
    // std::string filename("/home/wfg/data/mantid/SrMnSb2_T2K_H0T_Ei50meV_dmb_5runs.nxs"); // 6s
    // std::string filename("/home/wfg/data/mantid/SrMnSb2_T2K_H0T_Ei50meV_dmb_10runs.nxs"); //15s
    std::string filename("/home/wfg/data/mantid/SrMnSb2_T2K_H0T_Ei50meV_dmb_20runs.nxs"); // 50s
    // std::string filename("/home/wfg/data/mantid/SrMnSb2_T2K_H0T_Ei50meV_dmb_40runs.nxs"); // 212s
    // std::string filename("/home/wfg/data/mantid/SrMnSb2_T2K_H0T_Ei50meV_dmb_80runs.nxs"); // 1320s
    std::string outWSName("outWS");
    bool fileBackEnd(false);
    // double memory(0.0); // don't allocate in-cache memory for the file backend
    LoadMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FileBackEnd", fileBackEnd));
    // TS_ASSERT_THROWS_NOTHING(alg.setProperty("Memory", memory));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MetadataOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BoxStructureOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
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
      }   // is mdbox1
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
};
