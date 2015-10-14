#ifndef DIFFRACTIONFOCUSSING2TEST_H_
#define DIFFRACTIONFOCUSSING2TEST_H_

#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAlgorithms/MaskBins.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidDataHandling/LoadEventPreNexus.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include <cxxtest/TestSuite.h>
#include "MantidKernel/cow_ptr.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class DiffractionFocussing2Test : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(focus.name(), "DiffractionFocussing"); }

  void testVersion() { TS_ASSERT_EQUALS(focus.version(), 2); }

  void testCategory() { TS_ASSERT_EQUALS(focus.category(), "Diffraction"); }

  void testInit() {
    focus.initialize();
    TS_ASSERT(focus.isInitialized());
  }

  void testExec() {
    Mantid::DataHandling::LoadNexus loader;
    loader.initialize();
    loader.setPropertyValue(
        "Filename", "HRP38692a.nxs"); // HRP38692.raw spectrum range 320 to 330

    std::string outputSpace = "tofocus";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // Have to align because diffraction focussing wants d-spacing
    Mantid::Algorithms::AlignDetectors align;
    align.initialize();
    align.setPropertyValue("InputWorkspace", outputSpace);
    align.setPropertyValue("OutputWorkspace", outputSpace);
    align.setPropertyValue("CalibrationFile", "hrpd_new_072_01.cal");
    TS_ASSERT_THROWS_NOTHING(align.execute());
    TS_ASSERT(align.isExecuted());

    focus.setPropertyValue("InputWorkspace", outputSpace);
    focus.setPropertyValue("OutputWorkspace", "focusedWS");
    focus.setPropertyValue("GroupingFileName", "hrpd_new_072_01.cal");

    TS_ASSERT_THROWS_NOTHING(focus.execute());
    TS_ASSERT(focus.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "focusedWS"));

    // only 1 group for this limited range of spectra
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 1);

    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("focusedWS");
  }

  void test_EventWorkspace_SameOutputWS() { dotestEventWorkspace(true, 2); }

  void test_EventWorkspace_DifferentOutputWS() {
    dotestEventWorkspace(false, 2);
  }
  void test_EventWorkspace_SameOutputWS_oneGroup() {
    dotestEventWorkspace(true, 1);
  }

  void test_EventWorkspace_DifferentOutputWS_oneGroup() {
    dotestEventWorkspace(false, 1);
  }

  void test_EventWorkspace_TwoGroups_dontPreserveEvents() {
    dotestEventWorkspace(false, 2, false);
  }

  void test_EventWorkspace_OneGroup_dontPreserveEvents() {
    dotestEventWorkspace(false, 1, false);
  }

  void dotestEventWorkspace(bool inplace, size_t numgroups,
                            bool preserveEvents = true,
                            int bankWidthInPixels = 16) {
    std::string nxsWSname("DiffractionFocussing2Test_ws");

    // Create the fake event workspace
    EventWorkspace_sptr inputW =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(
            3, bankWidthInPixels);
    AnalysisDataService::Instance().addOrReplace(nxsWSname, inputW);

    //    //----- Load some event data --------
    //    FrameworkManager::Instance().exec("LoadEventNexus", 4,
    //        "Filename", "CNCS_7860_event.nxs",
    //        "OutputWorkspace", nxsWSname.c_str());

    //-------- Check on the input workspace ---------------
    TS_ASSERT(inputW);
    if (!inputW)
      return;

    // Fake a d-spacing unit in the data.
    inputW->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");

    // Create a DIFFERENT x-axis for each pixel. Starting bin = the input
    // workspace index #
    for (size_t pix = 0; pix < inputW->getNumberHistograms(); pix++) {
      Kernel::cow_ptr<MantidVec> axis;
      MantidVec &xRef = axis.access();
      xRef.resize(5);
      for (int i = 0; i < 5; ++i)
        xRef[i] = static_cast<double>(1 + pix) + i * 1.0;
      xRef[4] = 1e6;
      // Set an X-axis
      inputW->setX(pix, axis);
      inputW->getEventList(pix).addEventQuickly(TofEvent(1000.0));
    }

    // ------------ Create a grouping workspace by name -------------
    std::string GroupNames = "bank2,bank3";
    if (numgroups == 1)
      GroupNames = "bank3";
    std::string groupWSName("DiffractionFocussing2Test_group");
    FrameworkManager::Instance().exec("CreateGroupingWorkspace", 6,
                                      "InputWorkspace", nxsWSname.c_str(),
                                      "GroupNames", GroupNames.c_str(),
                                      "OutputWorkspace", groupWSName.c_str());

    // ------------ Create a grouping workspace by name -------------
    DiffractionFocussing2 focus;
    focus.initialize();
    TS_ASSERT_THROWS_NOTHING(
        focus.setPropertyValue("InputWorkspace", nxsWSname));
    std::string outputws = nxsWSname + "_focussed";
    if (inplace)
      outputws = nxsWSname;
    TS_ASSERT_THROWS_NOTHING(
        focus.setPropertyValue("OutputWorkspace", outputws));

    // This fake calibration file was generated using
    // DiffractionFocussing2Test_helper.py
    TS_ASSERT_THROWS_NOTHING(
        focus.setPropertyValue("GroupingWorkspace", groupWSName));
    TS_ASSERT_THROWS_NOTHING(
        focus.setProperty("PreserveEvents", preserveEvents));
    // OK, run the algorithm
    TS_ASSERT_THROWS_NOTHING(focus.execute(););
    TS_ASSERT(focus.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output =
            AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(
                outputws));
    if (!output)
      return;

    // ---- Did we keep the event workspace ----
    EventWorkspace_const_sptr outputEvent;
    TS_ASSERT_THROWS_NOTHING(
        outputEvent =
            boost::dynamic_pointer_cast<const EventWorkspace>(output));
    if (preserveEvents) {
      TS_ASSERT(outputEvent);
      if (!outputEvent)
        return;
    } else {
      TS_ASSERT(!outputEvent);
    }

    TS_ASSERT_EQUALS(output->getNumberHistograms(), numgroups);
    if (output->getNumberHistograms() != numgroups)
      return;

    TS_ASSERT_EQUALS(output->blocksize(), 4);

    TS_ASSERT_EQUALS(output->getAxis(1)->length(), numgroups);
    TS_ASSERT_EQUALS(output->getSpectrum(0)->getSpectrumNo(), 1);

    // Events in these two banks alone
    if (preserveEvents)
      TS_ASSERT_EQUALS(outputEvent->getNumberEvents(),
                       (numgroups == 2)
                           ? (bankWidthInPixels * bankWidthInPixels * 2)
                           : bankWidthInPixels * bankWidthInPixels);

    // Now let's test the grouping of detector UDETS to groups
    for (size_t wi = 0; wi < output->getNumberHistograms(); wi++) {
      // This is the list of the detectors (grouped)
      std::set<detid_t> mylist = output->getSpectrum(wi)->getDetectorIDs();
      // 1024 pixels in a bank
      TS_ASSERT_EQUALS(mylist.size(), bankWidthInPixels * bankWidthInPixels);
    }

    if (preserveEvents) {
      // Now let's try to rebin using log parameters (this used to fail?)
      Rebin rebin;
      rebin.initialize();
      rebin.setPropertyValue("InputWorkspace", outputws);
      rebin.setPropertyValue("OutputWorkspace", outputws);
      // Check it fails if "Params" property not set
      rebin.setPropertyValue("Params", "2.0,-1.0,65535");
      TS_ASSERT(rebin.execute());
      TS_ASSERT(rebin.isExecuted());

      /* Get the output ws again */
      outputEvent =
          AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputws);
      double events_after_binning = 0;
      for (size_t workspace_index = 0;
           workspace_index < outputEvent->getNumberHistograms();
           workspace_index++) {
        // should be 16 bins
        TS_ASSERT_EQUALS(outputEvent->refX(workspace_index)->size(), 16);
        // There should be some data in the bins
        for (int i = 0; i < 15; i++)
          events_after_binning += outputEvent->dataY(workspace_index)[i];
      }
      // The count sums up to the same as the number of events
      TS_ASSERT_DELTA(events_after_binning,
                      (numgroups == 2)
                          ? double(bankWidthInPixels * bankWidthInPixels) * 2.0
                          : double(bankWidthInPixels * bankWidthInPixels),
                      1e-4);
    }
  }

private:
  DiffractionFocussing2 focus;
};

//================================================================================================
//================================================================================================
//================================================================================================

class DiffractionFocussing2TestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DiffractionFocussing2TestPerformance *createSuite() {
    return new DiffractionFocussing2TestPerformance();
  }
  static void destroySuite(DiffractionFocussing2TestPerformance *suite) {
    delete suite;
  }

  EventWorkspace_sptr ws;

  DiffractionFocussing2TestPerformance() {
    IAlgorithm_sptr alg =
        AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
    alg->initialize();
    alg->setPropertyValue("Filename", "SNAP_Definition.xml");
    alg->setPropertyValue("OutputWorkspace", "SNAP_empty");
    alg->setPropertyValue("MakeEventWorkspace", "1");
    alg->execute();
    ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
        "SNAP_empty");
    ws->sortAll(TOF_SORT, NULL);

    // Fill a whole bunch of events
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < static_cast<int>(ws->getNumberHistograms()); i++) {
      EventList &el = ws->getEventList(i);
      for (int j = 0; j < 20; j++) {
        el.addEventQuickly(TofEvent(double(j) * 1e-3));
      }
    }
    ws->getAxis(0)->setUnit("dSpacing");
    // Create the x-axis for histogramming.
    MantidVecPtr x1;
    MantidVec &xRef = x1.access();
    xRef.clear();
    xRef.push_back(0.0);
    xRef.push_back(1e6);
    ws->setAllX(x1);

    alg = AlgorithmFactory::Instance().create("CreateGroupingWorkspace", 1);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupNames", "bank1");
    alg->setPropertyValue("OutputWorkspace", "SNAP_group_bank1");
    alg->execute();

    alg = AlgorithmFactory::Instance().create("CreateGroupingWorkspace", 1);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupNames", "bank1,bank2,bank3,bank4,bank5,bank6");
    alg->setPropertyValue("OutputWorkspace", "SNAP_group_several");
    alg->execute();
  }

  ~DiffractionFocussing2TestPerformance() {
    AnalysisDataService::Instance().remove("SNAP_empty");
    AnalysisDataService::Instance().remove("SNAP_group_bank1");
    AnalysisDataService::Instance().remove("SNAP_group_several");
  }

  void test_SNAP_event_one_group() {
    IAlgorithm_sptr alg =
        AlgorithmFactory::Instance().create("DiffractionFocussing", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupingWorkspace", "SNAP_group_bank1");
    alg->setPropertyValue("OutputWorkspace", "SNAP_focus");
    alg->setPropertyValue("PreserveEvents", "1");
    alg->execute();
    EventWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "SNAP_focus");

    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outWS->getNumberEvents(), 20 * 65536);
    AnalysisDataService::Instance().remove("SNAP_focus");
  }

  void test_SNAP_event_six_groups() {
    IAlgorithm_sptr alg =
        AlgorithmFactory::Instance().create("DiffractionFocussing", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupingWorkspace", "SNAP_group_several");
    alg->setPropertyValue("OutputWorkspace", "SNAP_focus");
    alg->setPropertyValue("PreserveEvents", "1");
    alg->execute();
    EventWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "SNAP_focus");

    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS(outWS->getNumberEvents(), 6 * 20 * 65536);
    AnalysisDataService::Instance().remove("SNAP_focus");
  }

  void test_SNAP_event_one_group_dontPreserveEvents() {
    IAlgorithm_sptr alg =
        AlgorithmFactory::Instance().create("DiffractionFocussing", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupingWorkspace", "SNAP_group_bank1");
    alg->setPropertyValue("OutputWorkspace", "SNAP_focus");
    alg->setPropertyValue("PreserveEvents", "0");
    alg->execute();
    MatrixWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "SNAP_focus");

    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1);
    AnalysisDataService::Instance().remove("SNAP_focus");
  }

  void test_SNAP_event_six_groups_dontPreserveEvents() {
    IAlgorithm_sptr alg =
        AlgorithmFactory::Instance().create("DiffractionFocussing", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupingWorkspace", "SNAP_group_several");
    alg->setPropertyValue("OutputWorkspace", "SNAP_focus");
    alg->setPropertyValue("PreserveEvents", "0");
    alg->execute();
    MatrixWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "SNAP_focus");

    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 6);
    AnalysisDataService::Instance().remove("SNAP_focus");
  }
};

#endif /*DIFFRACTIONFOCUSSING2TEST_H_*/
