// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidAlgorithms/AddTimeSeriesLog.h"
#include "MantidAlgorithms/ChangeBinOffset.h"
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
    FrameworkManager::Instance();
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
    TS_ASSERT_THROWS(align_and_focus.execute(), const std::runtime_error &);
  }

  /* Test AlignAndFocusPowder for HRP38692 raw data */
  void testHRP38692_useCalfile() { doTestHRP38692(true, false, false, false); }

  void testHRP38692_useCalfile_useGroupfile() { doTestHRP38692(true, false, true, false); }

  void testHRP38692_useCalfile_useGroupWorkspace() { doTestHRP38692(true, false, false, true); }

  void testHRP38692_useCalWorkspace_useGroupfile() { doTestHRP38692(false, true, true, false); }

  void testHRP38692_useCalWorkspace_useGroupWorkspace() { doTestHRP38692(false, true, false, true); }

  /* Test AlignAndFocusPowder for Event Workspace*/
  void testEventWksp_preserveEvents() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents");

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;

    // Run the main test function
    doTestEventWksp();

    // Test the input
    docheckEventInputWksp();
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(m_outWS);
    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 870622);
    // [99] 1920.2339999999983, 41
    TS_ASSERT_DELTA(m_outWS->x(0)[99], 1920.23400, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[99], 41.);
    // [899] 673.0, 15013.033999999987
    TS_ASSERT_DELTA(m_outWS->x(0)[899], 15013.03400, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[899], 673.0);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_preserveEvents_useGroupAll() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_useGroupAll");

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = true;
    m_useResamplex = true;

    // Run the main test function
    doTestEventWksp();

    // Test the input
    docheckEventInputWksp();
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    // [465] 1942.1284, 2498.0
    TS_ASSERT_DELTA(m_outWS->x(0)[465], 1942.1284, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[465], 2498.0);
    // [974] 15076.563461: 59802.0
    TS_ASSERT_DELTA(m_outWS->x(0)[974], 15076.563461, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[974], 59802.0);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_doNotPreserveEvents() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_doNotPreserveEvents");

    // Set the inputs for doTestEventWksp
    m_preserveEvents = false;
    m_useGroupAll = false;
    m_useResamplex = true;

    // Run the main test function
    doTestEventWksp();

    // Test the input
    docheckEventInputWksp();
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    // [99] 1920.2339999999983, 41
    TS_ASSERT_DELTA(m_outWS->x(0)[99], 1920.23400, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[99], 41.);
    // [899] 673.0, 15013.033999999987
    TS_ASSERT_DELTA(m_outWS->x(0)[899], 15013.03400, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[899], 673.0);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_doNotPreserveEvents_useGroupAll() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_doNotPreserveEvents_useGroupAll");

    // Set the inputs for doTestEventWksp
    m_preserveEvents = false;
    m_useGroupAll = true;
    m_useResamplex = true;

    // Run the main test function
    doTestEventWksp();
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the input
    docheckEventInputWksp();

    // Test the output
    // [465] 1942.1284, 2415.9
    TS_ASSERT_DELTA(m_outWS->x(0)[465], 1942.1284, 0.0001);
    TS_ASSERT_DELTA(m_outWS->y(0)[465], 2498, 0.1);
    // [974] 15076.563463: 60043.5
    TS_ASSERT_DELTA(m_outWS->x(0)[974], 15076.563463, 0.0001);
    TS_ASSERT_DELTA(m_outWS->y(0)[974], 59802, 0.1);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_rebin_preserveEvents() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_rebin_preserveEvents");

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = false;

    // Run the main test function
    doTestEventWksp();

    // Test the input
    TS_ASSERT_DELTA(m_inWS->x(0)[187], 1928.4933786037175, 0.0001);
    TS_ASSERT_EQUALS(m_inWS->y(0)[187], 53);
    TS_ASSERT_DELTA(m_inWS->x(0)[393], 14976.873144731135, 0.0001);
    TS_ASSERT_EQUALS(m_inWS->y(0)[393], 2580);
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[1872], 1948.5623011850066, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[1872], 4);
    TS_ASSERT_DELTA(m_outWS->x(0)[3915], 15015.319796791482, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[3915], 620);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_preserveEvents_dmin_dmax() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_dmin_dmax");

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
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[172], 3567.6990819051966, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[172], 37);
    TS_ASSERT_DELTA(m_outWS->x(0)[789], 6843.398982999533, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[789], 27);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_preserveEvents_tmin_tmax() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_tmin_tmax");

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    m_tmin = "2000.0";
    m_tmax = "12000.0";

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_tmin = "0";
    m_tmax = "0";

    // Test the input
    docheckEventInputWksp();
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[149], 3563.380399999972, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[149], 63);
    TS_ASSERT_DELTA(m_outWS->x(0)[816], 10113.053600000023, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[816], 175);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_preserveEvents_lambdamin_lambdamax() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_lambdamin_lambdamax");

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
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[199], 3556.833999999997, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[199], 92.);
    TS_ASSERT_DELTA(m_outWS->x(0)[599], 10103.233999999991, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[599], 277.);
    TS_ASSERT_DELTA(m_outWS->x(0)[899], 15013.033999999987, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[899], 0);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_preserveEvents_maskbins() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_maskbins");

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
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[199], 3556.833999999997, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[199], 92.);
    TS_ASSERT_DELTA(m_outWS->x(0)[599], 10103.233999999991, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[599], 277.);
    TS_ASSERT_DELTA(m_outWS->x(0)[899], 15013.033999999987, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[899], 0);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_preserveEvents_noCompressTolerance() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_noCompressTolerance");

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
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[199], 3556.833999999997, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[199], 92.);
    TS_ASSERT_DELTA(m_outWS->x(0)[599], 10103.233999999991, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[599], 277.);
    TS_ASSERT_DELTA(m_outWS->x(0)[899], 15013.033999999987, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[899], 673.);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_preserveEvents_highCompressTolerance() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_highCompressTolerance");

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
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[199], 3556.833999999997, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[199], 119.);
    TS_ASSERT_DELTA(m_outWS->x(0)[599], 10103.233999999991, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[599], 263.);
    TS_ASSERT_DELTA(m_outWS->x(0)[899], 15013.033999999987, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[899], 827.);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_preserveEvents_compressWallClockTolerance() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_compressWallClockTolerance");

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
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output: expected result shall be same as testEventWksp_preserveEvents_noCompressTolerance
    // because comparess time clock won't change the result
    TS_ASSERT_DELTA(m_outWS->x(0)[199], 3556.833999999997, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[199], 92.);
    TS_ASSERT_DELTA(m_outWS->x(0)[599], 10103.233999999991, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[599], 277.);
    TS_ASSERT_DELTA(m_outWS->x(0)[899], 15013.033999999987, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[899], 673.);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_preserveEvents_logCompressTolerance() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_logCompressTolerance");

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    m_compressTolerance = "-1e-5";

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_compressTolerance = "0";

    // Test the input
    docheckEventInputWksp();
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output: expected result shall be same as testEventWksp_preserveEvents but have fewer events
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(m_outWS);
    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 451436);
    // [99] 1920.2339999999983, 41
    TS_ASSERT_DELTA(m_outWS->x(0)[99], 1920.23400, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[99], 41.);
    // [899] 673.0, 15013.033999999987
    TS_ASSERT_DELTA(m_outWS->x(0)[899], 15013.03400, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[899], 673.0);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_preserveEvents_removePromptPulse() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_removePromptPulse");

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
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[199], 3556.833999999997, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[199], 92.);
    TS_ASSERT_DELTA(m_outWS->x(0)[599], 10103.233999999991, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[599], 0.);
    AnalysisDataService::Instance().remove(m_outputWS);
  }

  void testEventWksp_filterResonance() {

    /* Create fake event data in an event workspace */
    /* fake data is the "Powder Diffraction" composite function from
     * CreateSampleWorkspace.cpp: a series of 9 peaks */
    setUp_EventWorkspace("EventWksp_filterResonance");

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    addFrequencyForLogs();

    /* Run 0: produces the aligned and focused output workspace with no wavelength ranges
     * filtered out: */
    m_filterResonanceLower = "";
    m_filterResonanceUpper = "";

    /* Run AlignAndFocus */
    doTestEventWksp();

    TS_ASSERT_EQUALS(m_outWS->getNumberHistograms(), 144);

    /* Convert the units of the workspace to wavelength, the same units as the resonance
     * filtering limits */
    ConvertUnits convert_units;
    convert_units.initialize();
    convert_units.setPropertyValue("InputWorkspace", m_outputWS);
    convert_units.setPropertyValue("OutputWorkspace", m_outputWS);
    convert_units.setPropertyValue("Target", "Wavelength");
    convert_units.execute();

    /* get the raw output data */
    const auto &y0 = m_outWS->y(0);
    const auto &x0 = m_outWS->x(0).rawData();

    /* Obtain data values from peaks 2 and 5 (zero-indexed): */
    /* 1.3 - 1.5 for peak 2 */
    int peak_2_index = 299;
    double peak_2_x = 1.36951;
    double peak_2_y = 126;
    /* 2.6 - 2.8 */
    int peak_5_index = 599;
    double peak_5_x = 2.66423;
    double peak_5_y = 277;
    double tol = 1e-5;

    TS_ASSERT_DELTA(x0[peak_2_index], peak_2_x, tol);
    TS_ASSERT_DELTA(y0[peak_2_index], peak_2_y, tol);
    TS_ASSERT_DELTA(x0[peak_5_index], peak_5_x, tol);
    TS_ASSERT_DELTA(y0[peak_5_index], peak_5_y, tol);

    /* cleanup */
    AnalysisDataService::Instance().remove(m_outputWS);

    /* Run 2: produces the aligned and focused output workspace with two peaks
     * filtered out: the indices they occupied should now contain zero. */

    /* filter out peak 2 and peak 5 from above */
    m_filterResonanceLower = "1.3, 2.6";
    m_filterResonanceUpper = "1.5, 2.8";

    /* Run AlignAndFocus */
    doTestEventWksp();

    /* Convert the units of the workspace to get wavelength ranges */
    ConvertUnits convert_units_0;
    convert_units_0.initialize();
    convert_units_0.setPropertyValue("InputWorkspace", m_outputWS);
    convert_units_0.setPropertyValue("OutputWorkspace", m_outputWS);
    convert_units_0.setPropertyValue("Target", "Wavelength");
    convert_units_0.execute();

    const auto &y2 = m_outWS->y(0).rawData();
    const auto &x2 = m_outWS->x(0).rawData();

    /* y values of filtered peaks should be zero */
    TS_ASSERT_DELTA(x2[peak_2_index], peak_2_x, tol);
    TS_ASSERT_EQUALS(y2[peak_2_index], 0);
    TS_ASSERT_DELTA(x2[peak_5_index], peak_5_x, tol);
    TS_ASSERT_EQUALS(y2[peak_5_index], 0);

    /* Check the input workspace here for some reason otherwise segfault */
    docheckEventInputWksp();

    /* remove trashed workspaces */
    AnalysisDataService::Instance().remove(m_inputWS);
    AnalysisDataService::Instance().remove(m_outputWS);

    /* reset state variables for future tests... */
    m_filterResonanceLower = "";
    m_filterResonanceUpper = "";
  }

  void testEventWksp_preserveEvents_compressStartTime() {
    // Setup the event workspace
    setUp_EventWorkspace("EventWksp_preserveEvents_compressStartTime");

    // Set the inputs for doTestEventWksp
    m_preserveEvents = true;
    m_useGroupAll = false;
    m_useResamplex = true;
    // require both inside AlignAndFocusPowder
    m_compressStartTime = "2010-01-01T00:20:00"; // start time is "2010-01-01T00:00:00"
    m_compressWallClockTolerance = "50.0";       // require both inside AlignAndFocusPowder

    // Run the main test function
    doTestEventWksp();

    // Reset inputs to default values
    m_compressStartTime = "0";
    m_compressWallClockTolerance = "0";

    // Test the input
    docheckEventInputWksp();
    AnalysisDataService::Instance().remove(m_inputWS);

    // Test the output
    TS_ASSERT_DELTA(m_outWS->x(0)[199], 3556.833999999997, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[199], 68.);
    TS_ASSERT_DELTA(m_outWS->x(0)[599], 10103.233999999991, 0.0001);
    TS_ASSERT_EQUALS(m_outWS->y(0)[599], 190.);
    AnalysisDataService::Instance().remove(m_outputWS);
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

  void doTestHRP38692(bool useCalfile, bool useCalWksp, bool useGroupfile, bool useGroupWksp) {
    setUp_HRP38692();

    AlignAndFocusPowder align_and_focus;
    align_and_focus.initialize();

    align_and_focus.setPropertyValue("InputWorkspace", m_inputWS);
    align_and_focus.setPropertyValue("OutputWorkspace", m_outputWS);
    align_and_focus.setProperty("ResampleX", 1000);
    align_and_focus.setProperty("Dspacing", false);

    const std::string instrfilename("HRPD_Definition_pre20210301.xml");
    const std::string calfilename("hrpd_new_072_01.cal");
    const std::string grpfilename("hrpd_new_072_01_grp.xml"); // TODO add to external data repo
    if (useCalfile)
      align_and_focus.setPropertyValue("CalFilename", calfilename);
    else if (useCalWksp) {
      loadDiffCal(instrfilename, calfilename, false, true, true);
      // didn't load group
      align_and_focus.setPropertyValue("CalibrationWorkspace", m_loadDiffWSName + "_cal");
      align_and_focus.setPropertyValue("MaskWorkspace", m_loadDiffWSName + "_mask");
    }

    if (useGroupfile)
      align_and_focus.setPropertyValue("GroupFilename", grpfilename);
    else if (useGroupWksp) {
      loadDiffCal(instrfilename, calfilename, true, false, true);
      align_and_focus.setPropertyValue("GroupingWorkspace", m_loadDiffWSName + "_group");
      // didn't load calibration
      align_and_focus.setPropertyValue("MaskWorkspace", m_loadDiffWSName + "_mask");
    }

    TS_ASSERT_THROWS_NOTHING(align_and_focus.execute());
    TS_ASSERT(align_and_focus.isExecuted());

    m_inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_inputWS);

    TS_ASSERT_EQUALS(m_inWS->size(), 263857);
    TS_ASSERT_EQUALS(m_inWS->blocksize(), 23987);

    AnalysisDataService::Instance().remove(m_inputWS);

    m_outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_outputWS);

    TS_ASSERT_EQUALS(m_outWS->getAxis(0)->unit()->unitID(), "TOF");
    TS_ASSERT_EQUALS(m_outWS->size(), 1000);
    TS_ASSERT_EQUALS(m_outWS->blocksize(), m_outWS->size());
    TS_ASSERT_EQUALS(m_outWS->getNumberHistograms(), 1);

    // Maximum of peak near TOF approx. equal to 22,000 (micro-seconds)
    TS_ASSERT_DELTA(m_outWS->x(0)[333], 22011.6726, 0.0001);
    TS_ASSERT_DELTA(m_outWS->y(0)[333], 743.4881, 0.0001);

    // Maximum of peak near TOF approx. equal to 25,800 (micro-seconds)
    TS_ASSERT_DELTA(m_outWS->x(0)[398], 25780.5763, 0.0001);
    TS_ASSERT_DELTA(m_outWS->y(0)[398], 1584.2907, 0.0001);

    // Maximum of peak near TOF approx. equal to 42,000 (micro-seconds)
    TS_ASSERT_DELTA(m_outWS->x(0)[600], 42131.1493, 0.0001);
    TS_ASSERT_DELTA(m_outWS->y(0)[600], 7343.1294, 0.0001);

    AnalysisDataService::Instance().remove(m_outputWS);
  }

  /* Setup for event data. The caller supplies the workspace name */

  void setUp_EventWorkspace(const std::string &wkspname) {
    m_inputWS = wkspname;
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
    // peak 0
    TS_ASSERT_DELTA(m_inWS->x(0)[9], 1772.94, 0.01);
    TS_ASSERT_EQUALS(m_inWS->y(0)[9], 50);
    // peak 1
    TS_ASSERT_DELTA(m_inWS->x(0)[19], 3409.54, 0.01);
    TS_ASSERT_EQUALS(m_inWS->y(0)[19], 125);
    // peak 3: index = 39  6682.74  118
    TS_ASSERT_DELTA(m_inWS->x(0)[39], 6682.74, 0.01);
    TS_ASSERT_EQUALS(m_inWS->y(0)[39], 118);
    // peak 5: index = 59  9955.94  483
    TS_ASSERT_DELTA(m_inWS->x(0)[59], 9955.94, 0.01);
    TS_ASSERT_EQUALS(m_inWS->y(0)[59], 483);
    // peak 7: index = 89  14865.7  1524
    TS_ASSERT_DELTA(m_inWS->x(0)[89], 14865.7, 0.1);
    TS_ASSERT_EQUALS(m_inWS->y(0)[89], 1524);
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
      m_inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_inputWS);
      numHistoBins = int(m_inWS->blocksize());

      rebin(input_params);
      m_inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_inputWS);
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
      align_and_focus.setProperty("CompressWallClockTolerance", m_compressWallClockTolerance);

    // Filtering for the start wall clock time; cuts off events before start
    // time
    if (m_compressStartTime != "0")
      align_and_focus.setProperty("CompressStartTime", m_compressStartTime);

    // Remove prompt pulse; will cutoff the first peak from 6 long-TOF peaks (freq is 200 Hz)
    if (m_removePromptPulse)
      align_and_focus.setProperty("RemovePromptPulseWidth", 2200.0);

    // Filter absorption resonances - default unit is wavelength
    align_and_focus.setPropertyValue("ResonanceFilterLowerLimits", m_filterResonanceLower);
    align_and_focus.setPropertyValue("ResonanceFilterUpperLimits", m_filterResonanceUpper);

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
      auto group_wksp = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_groupWS);
      align_and_focus.setProperty("GroupingWorkspace", group_wksp->getName());
      numGroups = (int)group_wksp->blocksize();
    }

    TS_ASSERT_THROWS_NOTHING(align_and_focus.execute());
    TS_ASSERT(align_and_focus.isExecuted());

    m_inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_inputWS);
    m_outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_outputWS);

    TS_ASSERT_EQUALS(m_inWS->size(), m_numBanks * m_numPixels * m_numPixels * inputHistoBins);
    TS_ASSERT_EQUALS(m_inWS->blocksize(), inputHistoBins);

    TS_ASSERT_EQUALS(m_outWS->getAxis(0)->unit()->unitID(), "TOF");
    TS_ASSERT_EQUALS(m_outWS->size(), numGroups * numHistoBins);
    TS_ASSERT_EQUALS(m_outWS->blocksize(), numHistoBins);
    TS_ASSERT_EQUALS(m_outWS->getNumberHistograms(), numGroups);
  }

  /* Utility functions */
  void loadDiffCal(const std::string &instrfilename, const std::string &calfilename, bool group, bool cal, bool mask) {
    LoadDiffCal loadDiffAlg;
    loadDiffAlg.initialize();
    loadDiffAlg.setPropertyValue("Filename", calfilename);
    loadDiffAlg.setPropertyValue("InstrumentFilename", instrfilename);
    loadDiffAlg.setProperty("MakeGroupingWorkspace", group);
    loadDiffAlg.setProperty("MakeCalWorkspace", cal);
    loadDiffAlg.setProperty("MakeMaskWorkspace", mask);
    loadDiffAlg.setPropertyValue("WorkspaceName", m_loadDiffWSName);
    loadDiffAlg.execute();
  }

  void groupAllBanks(const std::string &m_inputWS) {
    CreateGroupingWorkspace groupAlg;
    groupAlg.initialize();
    groupAlg.setPropertyValue("InputWorkspace", m_inputWS);
    groupAlg.setPropertyValue("GroupDetectorsBy", "All");
    groupAlg.setPropertyValue("OutputWorkspace", m_groupWS);
    groupAlg.execute();
  }

  void rebin(const std::string &params, bool preserveEvents = true) {
    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", m_inputWS);
    rebin.setPropertyValue("OutputWorkspace", m_inputWS);
    rebin.setPropertyValue("Params", params);
    rebin.setProperty("PreserveEvents", preserveEvents);
    rebin.execute();
    TS_ASSERT(rebin.isExecuted());
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

  std::string createArgForNumberHistograms(double val, const MatrixWorkspace_sptr &ws,
                                           const std::string &delimiter = ",") {
    std::vector<std::string> vec;
    for (size_t i = 0; i < ws->getNumberHistograms(); i++)
      vec.emplace_back(boost::lexical_cast<std::string>(val));
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
    row2 << "" << 12000.0 << m_xmax + 1000.0;
    return m_maskBinTableWS;
  }

  void addPulseTimesForLogs() {
    AddTimeSeriesLog logAlg;
    std::string time, minute;
    std::string prefix{"2010-01-01T00:"};
    for (int i = 0; i < 60; i++) {
      minute = std::string(2 - std::to_string(i).length(), '0') + std::to_string(i);
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
  std::string m_filterResonanceLower;
  std::string m_filterResonanceUpper;
  bool m_preserveEvents{true};
  bool m_useGroupAll{true};
  bool m_useResamplex{true};
};
