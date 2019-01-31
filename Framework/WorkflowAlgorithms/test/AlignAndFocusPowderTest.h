// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALIGNANDFOCUSPOWDERTEST_H_
#define ALIGNANDFOCUSPOWDERTEST_H_

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidAlgorithms/AddTimeSeriesLog.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CreateGroupingWorkspace.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/ResampleX.h"
#include "MantidDataHandling/LoadDiffCal.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidDataHandling/MaskDetectors.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataHandling/RotateInstrumentComponent.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidWorkflowAlgorithms/AlignAndFocusPowder.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::WorkflowAlgorithms;

class AlignAndFocusPowderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlignAndFocusPowderTest *createSuite() {
    return new AlignAndFocusPowderTest();
  }
  static void destroySuite(AlignAndFocusPowderTest *suite) { delete suite; }

  /* Test AlignAndFocusPowder basics */
  void testTheBasics() {
    AlignAndFocusPowder align_and_focus;
    TS_ASSERT_EQUALS(align_and_focus.name(), "AlignAndFocusPowder");
    TS_ASSERT_EQUALS(align_and_focus.version(), 1);
  }

  void testInit() {
    AlignAndFocusPowder align_and_focus;
    TS_ASSERT_THROWS_NOTHING(align_and_focus.initialize());
    TS_ASSERT(align_and_focus.isInitialized());
  }

  void testException() {
    AlignAndFocusPowder align_and_focus;
    align_and_focus.initialize();
    TS_ASSERT_THROWS(align_and_focus.execute(), std::runtime_error);
  }

  /* Test AlignAndFocusPowder for HRP38692 raw data */
  void testHRP38692_useCalfile() { doTestHRP38692(true, false, false, false); }

  void testHRP38692_useCalfile_useGroupfile() {
    doTestHRP38692(true, false, true, false);
  }

  void testHRP38692_useCalfile_useGroupWorkspace() {
    doTestHRP38692(true, false, false, true);
  }

  void testHRP38692_useCalWorkspace_useGroupfile() {
    doTestHRP38692(false, true, true, false);
  }

  void testHRP38692_useCalWorkspace_useGroupWorkspace() {
    doTestHRP38692(false, true, false, true);
  }

  /* Test AlignAndFocusPowder for Event Workspace*/
  void testEventWksp_preserveEvents() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;

    // Run the main test function
    doTestEventWksp();

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[80], 1609.2800, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[80], 20);
    TS_ASSERT_DELTA(m_outWS->x(0)[880], 14702.0800, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[880], 587);
  }

  void testEventWksp_preserveEvents_useGroupAll() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = true;
    m_useResamplex = true;

    // Run the main test function
    doTestEventWksp();

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[423], 1634.3791, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[423], 2702);
    TS_ASSERT_DELTA(m_outWS->x(0)[970], 14719.8272, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[970], 149165);
  }

  void testEventWksp_doNotPreserveEvents() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = false;
    m_useGroupAll = false;
    m_useResamplex = true;

    // Run the main test function
    doTestEventWksp();

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[80], 1609.2800, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[80], 20);
    TS_ASSERT_DELTA(m_outWS->x(0)[880], 14702.0800, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[880], 587);
  }

  void testEventWksp_doNotPreserveEvents_useGroupAll() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = false;
    m_useGroupAll = true;
    m_useResamplex = true;

    // Run the main test function
    doTestEventWksp();

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[423], 1634.3791, 0.0001);
    TS_ASSERT_DELTA(m_outWS->y(0)[423], 2419.5680, 0.0001);
    TS_ASSERT_DELTA(m_outWS->x(0)[970], 14719.8272, 0.0001);
    TS_ASSERT_DELTA(m_outWS->y(0)[970], 148503.3853, 0.0001);
  }

  void testEventWksp_rebin_preserveEvents() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = false;

    // Run the main test function
    doTestEventWksp();

    // Test the input
    TS_ASSERT_DELTA(m_inWS->x(0)[170], 1628.3764, 0.0001);
    TS_ASSERT_EQUALS(m_inWS->y(0)[170], 48);
    TS_ASSERT_DELTA(m_inWS->x(0)[391], 14681.7696, 0.0001);
    TS_ASSERT_EQUALS(m_inWS->y(0)[391], 2540);

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[1693], 1629.3502, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[1693], 6);
    TS_ASSERT_DELTA(m_outWS->x(0)[3895], 14718.1436, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[3895], 612);
  }

  void testEventWksp_preserveEvents_dmin_dmax() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    m_dmin = createArgForNumberHistograms(0.5, m_inWS);
    m_dmax = createArgForNumberHistograms(1.5, m_inWS);

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_dmin = "0";
    m_dmax = "0";

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[116], 3270.3908, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[116], 37);
    TS_ASSERT_DELTA(m_outWS->x(0)[732], 6540.7817, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[732], 25);
  }

  void testEventWksp_preserveEvents_tmin_tmax() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    m_tmin = "2000.0";
    m_tmax = "10000.0";

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_tmin = "0";
    m_tmax = "0";

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[149], 3270.7563, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[149], 51);
    TS_ASSERT_DELTA(m_outWS->x(0)[982], 9814.5378, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[982], 138);
  }

  void testEventWksp_preserveEvents_lambdamin_lambdamax() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    m_lambdamin = "0.5";
    m_lambdamax = "3.0";

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_lambdamin = "0";
    m_lambdamax = "0";

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[181], 3262.2460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[181], 105);
    TS_ASSERT_DELTA(m_outWS->x(0)[581], 9808.6460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[581], 290);
    TS_ASSERT_DELTA(m_outWS->x(0)[880], 14702.0800, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[880], 0);
  }

  void testEventWksp_preserveEvents_maskbins() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    m_maskBinTableWS = createMaskBinTable();

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_maskBinTableWS = nullptr;

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[181], 3262.2460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[181], 105);
    TS_ASSERT_DELTA(m_outWS->x(0)[581], 9808.6460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[581], 290);
    TS_ASSERT_DELTA(m_outWS->x(0)[880], 14702.0800, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[880], 0);
  }

  void testEventWksp_preserveEvents_noCompressTolerance() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    m_compressTolerance = "0.0";

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_compressTolerance = "0";

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[181], 3262.2460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[181], 105);
    TS_ASSERT_DELTA(m_outWS->x(0)[581], 9808.6460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[581], 290);
    TS_ASSERT_DELTA(m_outWS->x(0)[880], 14702.0800, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[880], 587);
  }

  void testEventWksp_preserveEvents_highCompressTolerance() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    m_compressTolerance = "5.0";

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_compressTolerance = "0";

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[181], 3262.2460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[181], 96);
    TS_ASSERT_DELTA(m_outWS->x(0)[581], 9808.6460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[581], 427);
    TS_ASSERT_DELTA(m_outWS->x(0)[880], 14702.0800, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[880], 672);
  }

  void testEventWksp_preserveEvents_compressWallClockTolerance() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    m_compressWallClockTolerance = "50.0";
    addPulseTimesForLogs();

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_compressWallClockTolerance = "0";

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[181], 3262.2460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[181], 105);
    TS_ASSERT_DELTA(m_outWS->x(0)[581], 9808.6460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[581], 290);
    TS_ASSERT_DELTA(m_outWS->x(0)[880], 14702.0800, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[880], 587);
  }

  void testEventWksp_preserveEvents_removePromptPulse() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    m_removePromptPulse = true;
    addFrequencyForLogs();

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_removePromptPulse = false;

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[181], 3262.2460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[181], 105);
    TS_ASSERT_DELTA(m_outWS->x(0)[581], 9808.6460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[581], 0);
  }

  void testEventWksp_preserveEvents_compressStartTime() {
    // Setup the event workspace
    setUp_EventWorkspace();

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    // require both inside AlignAndFocusPowder
    m_compressStartTime =
        "2010-01-01T00:20:00"; // start time is "2010-01-01T00:00:00"
    m_compressWallClockTolerance =
        "50.0"; // require both inside AlignAndFocusPowder

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_compressStartTime = "0";
    m_compressWallClockTolerance = "0";

    // Test the input
    docheckEventInputWksp();

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[181], 3262.2460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[181], 72);
    TS_ASSERT_DELTA(m_outWS->x(0)[581], 9808.6460, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[581], 197);
  }

  /** Setup for testing HRPD NeXus data */
  void setUp_HRP38692() {

    LoadNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "HRP38692a.nxs");
    loader.setPropertyValue("OutputWorkspace", m_inputWS);
    loader.execute();
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());
  }

  void doTestHRP38692(bool useCalfile, bool useCalWksp, bool useGroupfile,
                      bool useGroupWksp) {

    setUp_HRP38692();

    AlignAndFocusPowder align_and_focus;
    align_and_focus.initialize();

    align_and_focus.setPropertyValue("InputWorkspace", m_inputWS);
    align_and_focus.setPropertyValue("OutputWorkspace", m_outputWS);
    align_and_focus.setProperty("ResampleX", 1000);
    align_and_focus.setProperty("Dspacing", false);

    TS_ASSERT_THROWS_NOTHING(align_and_focus.execute());
    TS_ASSERT(align_and_focus.isExecuted());

    std::string calfilename("hrpd_new_072_01.cal");
    if (useCalfile)
      align_and_focus.setPropertyValue("CalFilename", calfilename);
    else if (useCalWksp) {
      loadDiffCal(calfilename, false, true, true);
      align_and_focus.setPropertyValue("GroupingWorkspace",
                                       m_loadDiffWSName + "_group");
      align_and_focus.setPropertyValue("CalibrationWorkspace",
                                       m_loadDiffWSName + "_cal");
      align_and_focus.setPropertyValue("MaskWorkspace",
                                       m_loadDiffWSName + "_mask");
    }

    if (useGroupfile)
      align_and_focus.setPropertyValue("GroupFilename", calfilename);
    else if (useGroupWksp) {
      loadDiffCal(calfilename, true, false, true);
      align_and_focus.setPropertyValue("MaskWorkspace",
                                       m_loadDiffWSName + "_mask");
      align_and_focus.setPropertyValue("GroupingWorkspace",
                                       m_loadDiffWSName + "_group");
    }

    TS_ASSERT_THROWS_NOTHING(align_and_focus.execute());
    TS_ASSERT(align_and_focus.isExecuted());

    m_inWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_inputWS);
    m_outWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_outputWS);

    TS_ASSERT_EQUALS(m_inWS->size(), 263857);
    TS_ASSERT_EQUALS(m_inWS->blocksize(), 23987);

    TS_ASSERT_EQUALS(m_outWS->getAxis(0)->unit()->unitID(), "TOF");
    TS_ASSERT_EQUALS(m_outWS->size(), 1000);
    TS_ASSERT_EQUALS(m_outWS->blocksize(), m_outWS->size());
    TS_ASSERT_EQUALS(m_outWS->getNumberHistograms(), 1);

    // Maximum of peak near TOF approx. equal to 22,000 (micro-seconds)
    TS_ASSERT_DELTA(m_outWS->x(0)[333], 21990.0502, 0.0001);
    TS_ASSERT_DELTA(m_outWS->y(0)[333], 770.2515, 0.0001);

    // Maximum of peak near TOF approx. equal to 25,800 (micro-seconds)
    TS_ASSERT_DELTA(m_outWS->x(0)[398], 25750.3113, 0.0001);
    TS_ASSERT_DELTA(m_outWS->y(0)[398], 1522.3778, 0.0001);

    // Maximum of peak near TOF approx. equal to 42,000 (micro-seconds)
    TS_ASSERT_DELTA(m_outWS->x(0)[600], 42056.6091, 0.0001);
    TS_ASSERT_DELTA(m_outWS->y(0)[600], 7283.29652, 0.0001);
  }

  /* Setup for event data */

  void setUp_EventWorkspace() {
    m_inputWS = "eventWS";

    CreateSampleWorkspace createSampleAlg;
    createSampleAlg.initialize();
    createSampleAlg.setPropertyValue("WorkspaceType", "Event");
    createSampleAlg.setPropertyValue("Function", "Powder Diffraction");
    createSampleAlg.setProperty("XMin", m_xmin); // first frame
    createSampleAlg.setProperty("XMax", m_xmax);
    createSampleAlg.setProperty("BinWidth", 1.0);
    createSampleAlg.setProperty("NumBanks", m_numBanks); // detIds = [100,200)
    createSampleAlg.setProperty("BankPixelWidth", m_numPixels);
    createSampleAlg.setProperty("NumEvents", m_numEvents);
    createSampleAlg.setPropertyValue("OutputWorkspace", m_inputWS);
    createSampleAlg.execute();

    for (int i = 1; i <= m_numBanks; i++) {
      std::string bank = "bank" + std::to_string(i);
      RotateInstrumentComponent rotateInstr;
      rotateInstr.initialize();
      rotateInstr.setPropertyValue("Workspace", m_inputWS);
      rotateInstr.setPropertyValue("ComponentName", bank);
      rotateInstr.setProperty("Y", 1.);
      rotateInstr.setProperty("Angle", 90.);
      rotateInstr.execute();

      MoveInstrumentComponent moveInstr;
      moveInstr.initialize();
      moveInstr.setPropertyValue("Workspace", m_inputWS);
      moveInstr.setPropertyValue("ComponentName", bank);
      moveInstr.setProperty("X", 5.);
      moveInstr.setProperty("Y", -.1);
      moveInstr.setProperty("Z", .1);
      moveInstr.setProperty("RelativePosition", false);
      moveInstr.execute();
    }
  }

  void docheckEventInputWksp() {
    TS_ASSERT_DELTA(m_inWS->x(0)[8], 1609.2800, 0.0001);
    TS_ASSERT_EQUALS(m_inWS->y(0)[8], 97);
    TS_ASSERT_DELTA(m_inWS->x(0)[18], 3245.8800, 0.0001);
    TS_ASSERT_EQUALS(m_inWS->y(0)[18], 237);
    TS_ASSERT_DELTA(m_inWS->x(0)[38], 6519.0800, 0.0001);
    TS_ASSERT_EQUALS(m_inWS->y(0)[38], 199);
    TS_ASSERT_DELTA(m_inWS->x(0)[58], 9792.2800, 0.0001);
    TS_ASSERT_EQUALS(m_inWS->y(0)[58], 772);
    TS_ASSERT_DELTA(m_inWS->x(0)[88], 14702.0800, 0.0001);
    TS_ASSERT_EQUALS(m_inWS->y(0)[88], 2162);
  }

  void doTestEventWksp() {
    // Bin events using either ResampleX or Rebin
    int inputHistoBins{100};
    int numHistoBins{1000};
    std::string input_params{"-0.01"};
    std::string params{"-0.001"};
    if (m_useResamplex) {
      resamplex(inputHistoBins);
    } else {
      rebin(params);
      m_inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          m_inputWS);
      numHistoBins = int(m_inWS->blocksize());

      rebin(input_params);
      m_inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          m_inputWS);
      inputHistoBins = int(m_inWS->blocksize());
    }

    // Initialize AlignAndFocusPowder
    AlignAndFocusPowder align_and_focus;
    align_and_focus.initialize();
    align_and_focus.setPropertyValue("InputWorkspace", m_inputWS);
    align_and_focus.setPropertyValue("OutputWorkspace", m_outputWS);
    align_and_focus.setProperty("Dspacing", false);
    align_and_focus.setProperty("PreserveEvents", m_preserveEvents);

    // Use a Mask TableWorkspace created from createMaskBinTable
    if (m_maskBinTableWS)
      align_and_focus.setProperty("MaskBinTable", m_maskBinTableWS);

    // Compress tolerance for events
    if (m_compressTolerance != "0")
      align_and_focus.setProperty("CompressTolerance", m_compressTolerance);

    // Compression for the wall clock time; controls whether all pulses are
    // compressed together
    if (m_compressWallClockTolerance != "0")
      align_and_focus.setProperty("CompressWallClockTolerance",
                                  m_compressWallClockTolerance);

    // Filtering for the start wall clock time; cuts off events before start
    // time
    if (m_compressStartTime != "0")
      align_and_focus.setProperty("CompressStartTime", m_compressStartTime);

    // Remove prompt pulse; will cutoff last 6 long-TOF peaks (freq is 200 Hz)
    if (m_removePromptPulse)
      align_and_focus.setProperty("RemovePromptPulseWidth", 1e4);

    // Setup the binning type
    if (m_useResamplex) {
      align_and_focus.setProperty("ResampleX", numHistoBins);
    } else {
      align_and_focus.setProperty("Params", params);
    }

    // Crop each histogram using dSpacing
    if (m_dmin != "0") {
      align_and_focus.setProperty("Dspacing", true);
      align_and_focus.setPropertyValue("DMin", m_dmin);
    }
    if (m_dmax != "0") {
      align_and_focus.setProperty("Dspacing", true);
      align_and_focus.setPropertyValue("DMax", m_dmax);
    }

    // Crop entire workspace by TOF
    if (m_tmin != "0")
      align_and_focus.setPropertyValue("TMin", m_tmin);
    if (m_tmax != "0")
      align_and_focus.setPropertyValue("TMax", m_tmax);

    // Crop entire workspace by Wavelength
    if (m_lambdamin != "0")
      align_and_focus.setPropertyValue("CropWavelengthMin", m_lambdamin);
    if (m_lambdamax != "0")
      align_and_focus.setPropertyValue("CropWavelengthMax", m_lambdamax);

    int numGroups{m_numBanks * m_numPixels * m_numPixels};
    if (m_useGroupAll) {
      groupAllBanks(m_inputWS);
      auto group_wksp =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
              m_groupWS);
      align_and_focus.setProperty("GroupingWorkspace", group_wksp->getName());
      numGroups = (int)group_wksp->blocksize();
    }

    TS_ASSERT_THROWS_NOTHING(align_and_focus.execute());
    TS_ASSERT(align_and_focus.isExecuted());

    m_inWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_inputWS);
    m_outWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_outputWS);

    TS_ASSERT_EQUALS(m_inWS->size(),
                     m_numBanks * m_numPixels * m_numPixels * inputHistoBins);
    TS_ASSERT_EQUALS(m_inWS->blocksize(), inputHistoBins);

    TS_ASSERT_EQUALS(m_outWS->getAxis(0)->unit()->unitID(), "TOF");
    TS_ASSERT_EQUALS(m_outWS->size(), numGroups * numHistoBins);
    TS_ASSERT_EQUALS(m_outWS->blocksize(), numHistoBins);
    TS_ASSERT_EQUALS(m_outWS->getNumberHistograms(), numGroups);
  }

  /* Utility functions */
  void loadDiffCal(std::string calfilename, bool group, bool cal, bool mask) {
    LoadDiffCal loadDiffAlg;
    loadDiffAlg.initialize();
    loadDiffAlg.setPropertyValue("Filename", calfilename);
    loadDiffAlg.setPropertyValue("InstrumentName", "HRPD");
    loadDiffAlg.setProperty("MakeGroupingWorkspace", group);
    loadDiffAlg.setProperty("MakeCalWorkspace", cal);
    loadDiffAlg.setProperty("MakeMaskWorkspace", mask);
    loadDiffAlg.setPropertyValue("WorkspaceName", m_loadDiffWSName);
    loadDiffAlg.execute();
  }

  void groupAllBanks(std::string m_inputWS) {
    CreateGroupingWorkspace groupAlg;
    groupAlg.initialize();
    groupAlg.setPropertyValue("InputWorkspace", m_inputWS);
    groupAlg.setPropertyValue("GroupDetectorsBy", "All");
    groupAlg.setPropertyValue("OutputWorkspace", m_groupWS);
    groupAlg.execute();
  }

  void rebin(std::string params, bool preserveEvents = true) {
    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", m_inputWS);
    rebin.setPropertyValue("OutputWorkspace", m_inputWS);
    rebin.setPropertyValue("Params", params);
    rebin.setProperty("PreserveEvents", preserveEvents);
    rebin.execute();
    rebin.isExecuted();
  }

  void resamplex(int numHistoBins, bool preserveEvents = true) {
    ResampleX resamplexAlg;
    resamplexAlg.initialize();
    resamplexAlg.setPropertyValue("InputWorkspace", m_inputWS);
    resamplexAlg.setPropertyValue("OutputWorkspace", m_inputWS);
    resamplexAlg.setProperty("NumberBins", numHistoBins);
    resamplexAlg.setProperty("PreserveEvents", preserveEvents);
    resamplexAlg.execute();
  }

  std::string createArgForNumberHistograms(double val, MatrixWorkspace_sptr ws,
                                           std::string delimiter = ",") {
    std::vector<std::string> vec;
    for (size_t i = 0; i < ws->getNumberHistograms(); i++)
      vec.push_back(boost::lexical_cast<std::string>(val));
    std::string joined = boost::algorithm::join(vec, delimiter + " ");
    return joined;
  }

  ITableWorkspace_sptr createMaskBinTable() {
    m_maskBinTableWS = WorkspaceFactory::Instance().createTable();
    m_maskBinTableWS->addColumn("str", "SpectraList");
    m_maskBinTableWS->addColumn("double", "XMin");
    m_maskBinTableWS->addColumn("double", "XMax");
    TableRow row1 = m_maskBinTableWS->appendRow();
    row1 << "" << 0.0 << 2000.0;
    TableRow row2 = m_maskBinTableWS->appendRow();
    row2 << "" << 10000.0 << m_xmax + 1000.0;
    return m_maskBinTableWS;
  }

  void addPulseTimesForLogs() {
    AddTimeSeriesLog logAlg;
    std::string time, minute;
    std::string prefix{"2010-01-01T00:"};
    for (int i = 0; i < 60; i++) {
      minute =
          std::string(2 - std::to_string(i).length(), '0') + std::to_string(i);
      time = prefix + minute + "00";
      logAlg.initialize();
      logAlg.setPropertyValue("Workspace", m_inputWS);
      logAlg.setPropertyValue("Name", "proton_charge");
      logAlg.setPropertyValue("Time", time);
      logAlg.setPropertyValue("Value", "100");
    }
    logAlg.execute();
  }

  void addFrequencyForLogs() {
    AddSampleLog freqAlg;
    freqAlg.initialize();
    freqAlg.setPropertyValue("LogName", "Frequency");
    freqAlg.setPropertyValue("LogText", "200.0");
    freqAlg.setPropertyValue("LogUnit", "Hz");
    freqAlg.setPropertyValue("LogType", "Number Series");
    freqAlg.setPropertyValue("NumberType", "Double");
    freqAlg.setPropertyValue("Workspace", m_inputWS);
    freqAlg.execute();
  }

private:
  std::string m_inputWS{"nexusWS"};
  std::string m_outputWS{"align_and_focused"};
  MatrixWorkspace_sptr m_inWS;
  MatrixWorkspace_sptr m_outWS;
  ITableWorkspace_sptr m_maskBinTableWS;

  std::string m_loadDiffWSName{"AlignAndFocusPowderTest_diff"};
  std::string m_groupWS{"AlignAndFocusPowderTest_groupWS"};
  std::string m_maskBinTableWSName{"AlignAndFocusPowderTest_maskBinTable"};

  int m_numEvents{10000};
  int m_numBanks{1};
  int m_numPixels{12};
  double m_xmin{300.0};
  double m_xmax{16666.0};

  std::string m_dmin{"0"};
  std::string m_dmax{"0"};
  std::string m_tmin{"0"};
  std::string m_tmax{"0"};
  std::string m_lambdamin{"0"};
  std::string m_lambdamax{"0"};
  std::string m_compressTolerance{"0"};
  std::string m_compressWallClockTolerance{"0"};
  std::string m_compressStartTime{"0"};
  bool m_removePromptPulse{false};
  bool m_preserveEvents{true};
  bool m_useGroupAll{true};
  bool m_useResamplex{true};
};

#endif /*ALIGNANDFOCUSPOWDERTEST_H_*/
