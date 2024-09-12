// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <algorithm>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ChangeBinOffset.h"
#include "MantidAlgorithms/ConvertToMatrixWorkspace.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidAlgorithms/PDCalibration.h"
#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataHandling/RotateInstrumentComponent.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/Unit.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <ctime>

using Mantid::Algorithms::ChangeBinOffset;
using Mantid::Algorithms::ConvertToMatrixWorkspace;
using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::CropWorkspace;
using Mantid::Algorithms::PDCalibration;
using Mantid::API::AnalysisDataService;
using Mantid::API::FrameworkManager;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::DataHandling::GroupDetectors2;
using Mantid::DataHandling::MoveInstrumentComponent;
using Mantid::DataHandling::RotateInstrumentComponent;
using Mantid::DataObjects::EventList;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::DataObjects::MaskWorkspace;
using Mantid::DataObjects::MaskWorkspace_const_sptr;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::Kernel::IPropertyManager;
using Mantid::Kernel::MersenneTwister;
using Mantid::Kernel::PseudoRandomNumberGenerator;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

namespace {
// -- constants for creating the input event workspace

const int TOTAL_EVENT_COUNT = 100000;

// detID 155 is the middle at r,theta,phi = 5,90,0; DIFC = 5362.24
const double DIFC_155 = 5362.24;
const size_t WKSPINDEX_155 = 55; // spectrum number 56
// detID 195 is off to the side at r,theta,phi = 5.00063995,90,9166542; DIFC =
// 5405.21
const double DIFC_195 = 5405.21;
const size_t WKSPINDEX_195 = 95; // spectrum number 96

const double TOF_MIN = 300.; // first frame for 60Hz source
const double TOF_MAX = 16666.7;
const double BIN_WIDTH = 1.0; // microseconds
const std::vector<double> TOF_BINNING = {TOF_MIN, BIN_WIDTH, TOF_MAX};
// "Powder Diffraction" function makes 9 values of varying height and width that
// are equally spaced
const double PEAK_TOF_DELTA = (TOF_MAX - TOF_MIN) / 10.;

// there is a systematic shift where all found peaks are at too
// high of TOF by a few microseconds the first values are where
// CreateSampleWorkspace puts the peaks, the second is where
// FindPeaks locates them
// const std::vector<double> PEAK_TOFS = {1635.7, 3271.4, 4907.1, 6542.8,
// 8178.5,
//                                       9814.2, 11449.9, 13085.6, 14721.3};
const std::vector<double> PEAK_TOFS = {1636.5, 3272.5, 4908.5, 6544.5, 8180.5, 9816.5, 11452.5, 13088.5, 14724.5};

std::unique_ptr<PseudoRandomNumberGenerator> _randomGenerator;
auto &randomNumberGenerator(int seedValue = 0) {
  if (!_randomGenerator) {
    if (seedValue == 0)
      seedValue = static_cast<int>(std::time(nullptr));
    _randomGenerator = std::make_unique<MersenneTwister>(seedValue);
  }
  return _randomGenerator;
}

/**
 * Creates a workspace with peaks at 400, 800, 1300, 1600 us
 */
void createSampleWS() {
  // all values are at the same TOF so calibrations will be the same with
  // different starting guesses

  CreateSampleWorkspace createSampleWS;
  createSampleWS.initialize();
  createSampleWS.setPropertyValue("WorkspaceType", "Event");
  createSampleWS.setPropertyValue("Function", "Powder Diffraction");
  createSampleWS.setProperty("XMin", TOF_MIN); // first frame
  createSampleWS.setProperty("XMax", TOF_MAX);
  createSampleWS.setProperty("BinWidth", BIN_WIDTH); // micro-seconds
  createSampleWS.setProperty("NumBanks", 1);         // detIds = [100,200)
  createSampleWS.setProperty("NumEvents", TOTAL_EVENT_COUNT);
  createSampleWS.setProperty("PixelSpacing", .02); // 2cm pixels
  createSampleWS.setPropertyValue("OutputWorkspace", "PDCalibrationTest_WS");
  createSampleWS.execute();

  // In order to make it same as before CreateSampleWorkspace is fixed by shifting TOF back -TOF_MIN
  // such that peaks' positions will be kept unchanged.
  ChangeBinOffset changeBinOffset;
  changeBinOffset.initialize();
  changeBinOffset.setPropertyValue("InputWorkspace", "PDCalibrationTest_WS");
  changeBinOffset.setPropertyValue("OutputWorkspace", "PDCalibrationTest_WS");
  changeBinOffset.setProperty("Offset", -1 * TOF_MIN);
  changeBinOffset.execute();

  // move it to the right place - DIFC of this location vary from 5308 to 5405
  RotateInstrumentComponent rotateInstr;
  rotateInstr.initialize();
  rotateInstr.setPropertyValue("Workspace", "PDCalibrationTest_WS");
  rotateInstr.setPropertyValue("ComponentName", "bank1");
  rotateInstr.setProperty("Y", 1.);
  rotateInstr.setProperty("Angle", 90.);
  rotateInstr.execute();
  MoveInstrumentComponent moveInstr;
  moveInstr.initialize();
  moveInstr.setPropertyValue("Workspace", "PDCalibrationTest_WS");
  moveInstr.setPropertyValue("ComponentName", "bank1");
  moveInstr.setProperty("X", 5.);
  moveInstr.setProperty("Y", -.1);
  moveInstr.setProperty("Z", .1);
  moveInstr.setProperty("RelativePosition", false);
  moveInstr.execute();
}

std::vector<double> convertPosToD(const double difc) {
  using Mantid::Kernel::UnitParams;
  std::vector<double> dValues(PEAK_TOFS);
  Mantid::Kernel::Units::dSpacing dSpacingUnit;
  std::vector<double> unusedy;
  dSpacingUnit.fromTOF(dValues, unusedy, -1., 0, Mantid::Kernel::UnitParametersMap{{UnitParams::difc, difc}});

  return dValues;
}

// Add uniform noise to an event list.
void addUniformNoiseToEventList(EventList &es, double tofMin, double tofMax, int totalCounts) {
  // Based on the analogous section from `CreateSampleWorkspace`:

  DateAndTime run_start("2010-01-01T00:00:00");
  const double hourInSeconds = 60.0 * 60.0;
  auto &randGen = randomNumberGenerator();
  const double tofSpan = tofMax - tofMin;

  for (int i = 0; i < totalCounts; ++i) {
    // Create randomised events within the TOF span.
    DateAndTime pulseTime = run_start + (randGen->nextValue() * hourInSeconds);
    es += TofEvent(randGen->nextValue() * tofSpan + tofMin, pulseTime);
  }
}

} // namespace

class PDCalibrationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PDCalibrationTest *createSuite() { return new PDCalibrationTest(); }
  static void destroySuite(PDCalibrationTest *suite) { delete suite; }

  PDCalibrationTest() { FrameworkManager::Instance(); }

  void setUp() override {
    // individual spectra
    createSampleWS();

    // group detectors
    GroupDetectors2 groupDet;
    groupDet.initialize();
    groupDet.setPropertyValue("InputWorkspace", "PDCalibrationTest_WS");
    groupDet.setPropertyValue("OutputWorkspace", "PDCalibrationTest_WS_grouped");
    groupDet.setPropertyValue("DetectorList", "100,101,102,103");
    groupDet.execute();
  }

  void test_Init() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void checkDSpacing(const std::string &wsname, const std::vector<double> &dValues) {
    ITableWorkspace_sptr peaksTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(wsname);
    Mantid::DataObjects::TableColumn_ptr<int> col0 = peaksTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // check for workspace index 55 which is spectrum 56
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    for (size_t i = 0; i < dValues.size(); ++i) {
      TS_ASSERT_DELTA(peaksTable->cell<double>(index, 1 + i), dValues[i], 0.0002);
    }
    // checks for chisq, first one is strange because of test framework missing
    // > operator
    TS_ASSERT_LESS_THAN(0., peaksTable->cell<double>(index, 1 + dValues.size()));
    TS_ASSERT_LESS_THAN(peaksTable->cell<double>(index, 1 + dValues.size()), 10.);

    // check for workspace index 95 which is spectrum 96 - last peak is out of
    // range???
    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    for (size_t i = 0; i < dValues.size() - 1; ++i) {
      TS_ASSERT_DELTA(peaksTable->cell<double>(index, 1 + i), dValues[i], 0.0002);
    }
    // checks for chisq, first one is strange because of test framework missing
    // > operator
    TS_ASSERT_LESS_THAN(0., peaksTable->cell<double>(index, 1 + dValues.size()));
    TS_ASSERT_LESS_THAN(peaksTable->cell<double>(index, 1 + dValues.size()), 10.);
  }

  void test_exec_difc() {
    // setup the peak postions based on transformation from detID=155
    std::vector<double> dValues = convertPosToD(DIFC_155);

    const std::string prefix{"PDCalibration_difc"};

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaskWorkspace", prefix + "_mask"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", TOF_BINNING));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputCalibrationTable", prefix + "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DiagnosticWorkspaces", prefix + "diag"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix + "cal");

    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    MatrixWorkspace_const_sptr mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(prefix + "_mask");
    // 0 is keep
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_155)[0], 0);
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_195)[0], 0);

    checkDSpacing(prefix + "diag_dspacing", dValues);

    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "_mask");
  }

  void test_exec_difc_tzero() {
    using Mantid::Kernel::UnitParams;
    // setup the peak postions based on transformation from detID=155
    const double TZERO = 20.;
    std::vector<double> dValues(PEAK_TOFS);
    Mantid::Kernel::Units::dSpacing dSpacingUnit;
    std::vector<double> unusedy;
    dSpacingUnit.fromTOF(dValues, unusedy, -1., 0,
                         Mantid::Kernel::UnitParametersMap{{UnitParams::difc, DIFC_155}, {UnitParams::tzero, TZERO}});

    const std::string prefix{"PDCalibration_difc_tzero"};

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaskWorkspace", prefix + "_mask"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", TOF_BINNING));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputCalibrationTable", prefix + "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DiagnosticWorkspaces", prefix + "diag"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CalibrationParameters", "DIFC+TZERO"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix + "cal");

    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, 0.1); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), TZERO, 0.1);    // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, 0.1); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), TZERO, 0.1);    // tzero

    MatrixWorkspace_const_sptr mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(prefix + "_mask");

    // 0 is keep
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_155)[0], 0);
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_195)[0], 0);

    checkDSpacing(prefix + "diag_dspacing", dValues);

    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "_mask");
  }

  void test_exec_difc_tzero_difa() {
    using Mantid::Kernel::UnitParams;
    // setup the peak postions based on transformation from detID=155
    // allow refining DIFA, but don't set the transformation to require it
    const double TZERO = 20.;
    const double DIFA = 0.05;
    std::vector<double> dValues(PEAK_TOFS);
    Mantid::Kernel::Units::dSpacing dSpacingUnit;
    std::vector<double> unusedy;
    dSpacingUnit.fromTOF(dValues, unusedy, -1., 0,
                         Mantid::Kernel::UnitParametersMap{
                             {UnitParams::difc, DIFC_155}, {UnitParams::tzero, TZERO}, {UnitParams::difa, DIFA}});

    const std::string prefix{"PDCalibration_difc_tzero_difa"};

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaskWorkspace", prefix + "_mask"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", TOF_BINNING));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputCalibrationTable", prefix + "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DiagnosticWorkspaces", prefix + "diag"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseChiSq", true)); // don't bias fit on pk height
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CalibrationParameters", "DIFC+TZERO+DIFA"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix + "cal");

    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, 0.1); // difc
    TS_ASSERT_DELTA(calTable->cell<double>(index, 2), DIFA, 0.01);    // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), TZERO, 0.1);    // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, 0.1); // difc
    TS_ASSERT_DELTA(calTable->cell<double>(index, 2), DIFA, 0.01);    // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), TZERO, 0.1);    // tzero

    MatrixWorkspace_const_sptr mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(prefix + "_mask");

    // 0 is keep
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_155)[0], 0);
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_195)[0], 0);

    checkDSpacing(prefix + "diag_dspacing", dValues);

    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "_mask");
  }

  // Crop workspace so that final peak is evaluated over a range that includes
  // the last bin (stop regression out of range bug for histo workspaces)
  void test_exec_difc_histo() {
    using Mantid::Kernel::UnitParams;
    // convert to histo
    ConvertToMatrixWorkspace convMatWS;
    convMatWS.initialize();
    convMatWS.setPropertyValue("InputWorkspace", "PDCalibrationTest_WS");
    convMatWS.setPropertyValue("OutputWorkspace", "PDCalibrationTest_WS");
    convMatWS.execute();
    // crop
    std::string xmax = "15104"; // only keep TOF < xmax
    CropWorkspace cropWS;
    cropWS.initialize();
    cropWS.setPropertyValue("InputWorkspace", "PDCalibrationTest_WS");
    cropWS.setPropertyValue("OutputWorkspace", "PDCalibrationTest_WS");
    cropWS.setPropertyValue("XMin", "300");
    cropWS.setPropertyValue("XMax", xmax);
    cropWS.execute();

    // setup the peak postions based on transformation from detID=155
    std::vector<double> dValues(PEAK_TOFS);

    Mantid::Kernel::Units::dSpacing dSpacingUnit;
    std::vector<double> unusedy;
    dSpacingUnit.fromTOF(dValues, unusedy, -1., 0, Mantid::Kernel::UnitParametersMap{{UnitParams::difc, DIFC_155}});

    const std::string prefix{"PDCalibration_difc"};

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaskWorkspace", prefix + "_mask"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", std::to_string(TOF_BINNING[0]) + "," +
                                                               std::to_string(TOF_BINNING[1]) + "," + xmax));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputCalibrationTable", prefix + "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DiagnosticWorkspaces", prefix + "diag"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // test that the difc values are the same as for event
    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix + "cal");

    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "_mask");
  }

  void test_exec_fit_diff_constants_with_chisq() {
    // setup the peak postions based on transformation from detID=155
    // allow refining DIFA, but don't set the transformation to require it
    // setup the peak postions based on transformation from detID=155
    std::vector<double> dValues = convertPosToD(DIFC_155);

    const std::string prefix{"PDCalibration_difc"};

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaskWorkspace", prefix + "_mask"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", TOF_BINNING));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputCalibrationTable", prefix + "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DiagnosticWorkspaces", prefix + "diag"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseChiSq", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // check that a table containing the fit parameter errors is returned
    ITableWorkspace_sptr errorTable =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix + "diag_fiterror");
    TS_ASSERT(errorTable);
    // check the column titles correpsond to names of Gaussian fit parameters
    // not the generic height, centre, width
    TS_ASSERT_EQUALS(errorTable->getColumnNames()[4], "Sigma");

    // check cal table
    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix + "cal");
    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    // check get roughly same result as UseChiSQ = false
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "_mask");
  }

  void test_exec_grouped_detectors() {
    // setup the peak postions based on transformation from detID=155
    std::vector<double> dValues = convertPosToD(DIFC_155);

    const std::string prefix{"PDCalibration_difc"};

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "PDCalibrationTest_WS_grouped"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaskWorkspace", prefix + "_mask"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", TOF_BINNING));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputCalibrationTable", prefix + "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DiagnosticWorkspaces", prefix + "diag"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix + "cal");
    TS_ASSERT(calTable);
    TS_ASSERT_EQUALS(calTable->rowCount(), 100); // all detids are included
    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();
    // test that the cal table has the same difc value for grouped dets
    size_t index = std::find(detIDs.begin(), detIDs.end(), 100) - detIDs.begin();
    TS_ASSERT_DELTA(calTable->cell<double>(index + 0, 1), calTable->cell<double>(index, 1), 1E-5); // det 100
    TS_ASSERT_DELTA(calTable->cell<double>(index + 1, 1), calTable->cell<double>(index, 1), 1E-5); // det 101
    TS_ASSERT_DELTA(calTable->cell<double>(index + 2, 1), calTable->cell<double>(index, 1), 1E-5); // det 102
    TS_ASSERT_DELTA(calTable->cell<double>(index + 3, 1), calTable->cell<double>(index, 1), 1E-5); // det 103

    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "_mask");
  }

  void test_exec_grouped_detectors_limit_spectra() {
    // setup the peak postions based on transformation from detID=155
    std::vector<double> dValues = convertPosToD(DIFC_155);

    const std::string prefix{"PDCalibration_difc"};

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "PDCalibrationTest_WS_grouped"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaskWorkspace", prefix + "_mask"));
    // selecting only the pixel that was grouped
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartWorkspaceIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StopWorkspaceIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", TOF_BINNING));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputCalibrationTable", prefix + "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DiagnosticWorkspaces", prefix + "diag"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix + "cal");
    TS_ASSERT(calTable);
    TS_ASSERT_EQUALS(calTable->rowCount(), 4); // only the grouped detectors should be included
    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();
    // test that the cal table has the same difc value for grouped dets
    size_t index = std::find(detIDs.begin(), detIDs.end(), 100) - detIDs.begin();
    TS_ASSERT_EQUALS(index, 0); // should start at zero
    TS_ASSERT_DELTA(calTable->cell<double>(index + 0, 1), calTable->cell<double>(index, 1), 1E-5); // det 100
    TS_ASSERT_DELTA(calTable->cell<double>(index + 1, 1), calTable->cell<double>(index, 1), 1E-5); // det 101
    TS_ASSERT_DELTA(calTable->cell<double>(index + 2, 1), calTable->cell<double>(index, 1), 1E-5); // det 102
    TS_ASSERT_DELTA(calTable->cell<double>(index + 3, 1), calTable->cell<double>(index, 1), 1E-5); // det 103

    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "_mask");
  }

  void test_exec_ikeda_carpenter() {

    const std::string prefix{"PDCalibration_ikeda_carpenter"};

    // test the algorithm using the IkedaCarpenterPV peak function
    const double ref_difc = 2208.287616521762;

    const std::vector<double> dValues{.8920, 1.0758, 1.2615, 2.0599};

    std::stringstream function;
    for (const auto &val : dValues) {
      function << "name=IkedaCarpenterPV, X0=" << ref_difc * val << ", I=50;";
    }

    CreateSampleWorkspace wsalg;
    TS_ASSERT_THROWS_NOTHING(wsalg.initialize());
    TS_ASSERT(wsalg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(wsalg.setPropertyValue("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(wsalg.setPropertyValue("WorkspaceType", "Event"));
    TS_ASSERT_THROWS_NOTHING(wsalg.setPropertyValue("Function", "User Defined"));
    TS_ASSERT_THROWS_NOTHING(wsalg.setPropertyValue("UserDefinedFunction", function.str()));
    TS_ASSERT_THROWS_NOTHING(wsalg.setProperty("XMin", 1.0));
    TS_ASSERT_THROWS_NOTHING(wsalg.setProperty("XMax", 16666.7));
    TS_ASSERT_THROWS_NOTHING(wsalg.setProperty("BinWidth", 1.0));
    TS_ASSERT_THROWS_NOTHING(wsalg.setProperty("NumEvents", 100000));
    TS_ASSERT_THROWS_NOTHING(wsalg.setProperty("BankPixelWidth", 1));
    TS_ASSERT_THROWS_NOTHING(wsalg.setProperty("NumBanks", 1));
    TS_ASSERT_THROWS_NOTHING(wsalg.execute());
    TS_ASSERT(wsalg.isExecuted());

    MatrixWorkspace_const_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("ws");
    TS_ASSERT(ws);

    MoveInstrumentComponent movealg;
    TS_ASSERT_THROWS_NOTHING(movealg.initialize());
    TS_ASSERT(movealg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(movealg.setProperty("Workspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(movealg.setPropertyValue("ComponentName", "bank1"));
    TS_ASSERT_THROWS_NOTHING(movealg.setProperty("X", 1.01));
    TS_ASSERT_THROWS_NOTHING(movealg.setProperty("Y", 0.0));
    TS_ASSERT_THROWS_NOTHING(movealg.setProperty("Z", 1.01));
    TS_ASSERT_THROWS_NOTHING(movealg.setProperty("RelativePosition", false));
    TS_ASSERT_THROWS_NOTHING(movealg.execute());
    TS_ASSERT(movealg.isExecuted());

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaskWorkspace", prefix + "_mask"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", std::vector<double>{1, 1, 16666}));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakFunction", "IkedaCarpenterPV"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputCalibrationTable", "ikeda_cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DiagnosticWorkspaces", "ikeda_diag"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("ikeda_cal");
    TS_ASSERT(calTable);
    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();
    TS_ASSERT_DELTA(calTable->cell<double>(0, 1), ref_difc, 1E-2 * ref_difc);

    Mantid::API::AnalysisDataService::Instance().remove("ws");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "_mask");
    Mantid::API::AnalysisDataService::Instance().remove("ikeda_cal");
  }

  auto maskTestsInitialization(IPropertyManager &algorithmProperties, const std::string &uniquePrefix) {
    // In order to avoid collisions in the AnalysisDataService, which is *shared* by all tests,
    //   any workspaces that will be modified must be unique to the current test.
    const std::string inputWSName = uniquePrefix + "_input";
    const std::string maskWSName = uniquePrefix + "_mask";
    const std::string diagnosticWSName = uniquePrefix + "_diag";
    const std::string outputWSName = uniquePrefix + "_cal";

    // Create a unique clone of the input workspace:
    //    this should be a clone of: either of "PDCalibrationTest_WS_grouped", or "PDCalibrationTest_WS", as
    MatrixWorkspace_const_sptr input_ =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("PDCalibrationTest_WS");
    MatrixWorkspace_sptr input = input_->clone();
    AnalysisDataService::Instance().add(inputWSName, input);

    // Create a compatible mask workspace with one spectrum per detector:
    MaskWorkspace_sptr mask = std::make_shared<MaskWorkspace>(input->getInstrument());
    AnalysisDataService::Instance().add(maskWSName, mask);

    // Note: this section duplicates setup from "test_exec_difc".

    // setup the peak postions based on transformation from detID=155
    std::vector<double> dValues = convertPosToD(DIFC_155);

    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setProperty("InputWorkspace", inputWSName));
    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setPropertyValue("MaskWorkspace", maskWSName));
    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setProperty("TofBinning", TOF_BINNING));
    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setPropertyValue("OutputCalibrationTable", outputWSName));
    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setPropertyValue("DiagnosticWorkspaces", diagnosticWSName));
    TS_ASSERT_THROWS_NOTHING(algorithmProperties.setProperty("PeakPositions", dValues));

    auto cleanup = [=]() {
      try {
        AnalysisDataService::Instance().remove(inputWSName);
        AnalysisDataService::Instance().remove(maskWSName);
        AnalysisDataService::Instance().remove(diagnosticWSName + "_fitparam");
        AnalysisDataService::Instance().remove(diagnosticWSName + "_fitted");
        AnalysisDataService::Instance().remove(diagnosticWSName + "_fiterrors");
        AnalysisDataService::Instance().remove(diagnosticWSName + "_dspacing");
        AnalysisDataService::Instance().remove(diagnosticWSName + "_width");
        AnalysisDataService::Instance().remove(diagnosticWSName + "_height");
        AnalysisDataService::Instance().remove(diagnosticWSName + "_resolution");
        AnalysisDataService::Instance().remove(outputWSName);
      } catch (std::exception &x) {
        std::cout << "error during workspaces cleanup: " << x.what() << std::endl;
      }
    };
    return std::make_tuple(inputWSName, maskWSName, diagnosticWSName, outputWSName, dValues, cleanup);
  }

  /**
   * Verify that the optional mask workspace input parameter is properly treated:
   *   when the parameter is specified, but the mask workspace does not exist,
   *      a mask workspace will be created, and will exist in the ADS after exit.
   */
  void test_mask_is_created() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    const std::string uniquePrefix = "test_MIC";
    auto [inputWSName, maskWSName, diagnosticWSName, outputWSName, dValues, cleanup] =
        maskTestsInitialization(alg, uniquePrefix);

    // Ensure incoming mask workspace doesn't exist.
    AnalysisDataService::Instance().remove(maskWSName);
    TS_ASSERT(!AnalysisDataService::Instance().doesExist(maskWSName));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outputWSName);
    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero
    checkDSpacing(diagnosticWSName + "_dspacing", dValues);

    MaskWorkspace_const_sptr mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    TS_ASSERT_EQUALS(mask->getNumberMasked(), 0);

    cleanup();
  }
  /**
   * Verify that the optional mask workspace input parameter is properly treated:
   *   when the parameter is specified and the workspace already exists,
   *     no new mask workspace will be created.
   */
  void test_input_mask_is_used() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    const std::string uniquePrefix = "test_IMIU";
    auto [inputWSName, maskWSName, diagnosticWSName, outputWSName, dValues, cleanup] =
        maskTestsInitialization(alg, uniquePrefix);

    TS_ASSERT(AnalysisDataService::Instance().doesExist(maskWSName));
    // Set the mask-workspace title to a random string:
    const std::string maskWSTitle("42601ecc-b7ab-426f-874e-c39662a8a295");
    MaskWorkspace_sptr mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    mask->setTitle(maskWSTitle);
    TS_ASSERT(mask->getTitle() == maskWSTitle);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outputWSName);
    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero
    checkDSpacing(diagnosticWSName + "_dspacing", dValues);

    TS_ASSERT_THROWS_NOTHING(mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName));
    TS_ASSERT(mask);
    TS_ASSERT(mask->getTitle() == maskWSTitle);
    TS_ASSERT_EQUALS(mask->getNumberMasked(), 0);

    cleanup();
  }

  /**
   * Verify that no spectra in the mask test input workspace will be masked in normal circumstances.
   */
  void test_none_are_masked() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    const std::string uniquePrefix = "test_NAM";
    auto [inputWSName, maskWSName, diagnosticWSName, outputWSName, dValues, cleanup] =
        maskTestsInitialization(alg, uniquePrefix);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outputWSName);
    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero
    checkDSpacing(diagnosticWSName + "_dspacing", dValues);

    MaskWorkspace_const_sptr mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    TS_ASSERT_EQUALS(mask->getNumberMasked(), 0);

    cleanup();
  }

  /**
   * Verify that failing spectra are masked: event lists corresponding to specific input spectra are cleared.
   */
  void test_failures_are_masked_zero_pixels() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    const std::string uniquePrefix = "test_FAMZ";
    auto [inputWSName, maskWSName, diagnosticWSName, outputWSName, dValues, cleanup] =
        maskTestsInitialization(alg, uniquePrefix);

    const std::set<size_t> spectraToFail{1, 27, 35, 36, 54, 88, 99};
    EventWorkspace_sptr inputWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputWSName);
    for (std::size_t idx : spectraToFail) {
      // Important: in order to zero a spectrum, "clear" the event list:
      //   this is what `PDCalibration` expects.
      inputWS->getSpectrum(idx).clear(false);
      TS_ASSERT(inputWS->getSpectrum(idx).empty())
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outputWSName);
    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero
    checkDSpacing(diagnosticWSName + "_dspacing", dValues);

    MaskWorkspace_const_sptr mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    TS_ASSERT(mask);
    TS_ASSERT_EQUALS(mask->getNumberMasked(), spectraToFail.size());
    for (std::size_t idx : spectraToFail)
      TS_ASSERT(mask->isMaskedIndex(idx));

    cleanup();
  }

  /**
   * Verify that failing spectra are masked: event lists corresponding to specific spectra are initialized with uniform
   * noise.
   */
  void test_failures_are_masked_noise_pixels() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    const std::string uniquePrefix = "test_FAMN";
    auto [inputWSName, maskWSName, diagnosticWSName, outputWSName, dValues, cleanup] =
        maskTestsInitialization(alg, uniquePrefix);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxChiSq", 1.0));

    const std::set<size_t> spectraToFail{1, 27, 35, 36, 54, 88, 99};
    // In `createSampleWorkspace`: TOF bin offset is shifted towards zero by TOF_MIN.
    const double tofMin = 0.0;
    const double tofMax = TOF_MAX - TOF_MIN;
    EventWorkspace_sptr inputWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputWSName);

    for (std::size_t idx : spectraToFail) {
      EventList &es = inputWS->getSpectrum(idx);
      es.clear(false);
      addUniformNoiseToEventList(es, tofMin, tofMax, TOTAL_EVENT_COUNT);
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outputWSName);
    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero
    checkDSpacing(diagnosticWSName + "_dspacing", dValues);

    MaskWorkspace_const_sptr mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    TS_ASSERT(mask);

    TS_ASSERT_EQUALS(mask->getNumberMasked(), spectraToFail.size());

    for (std::size_t idx : spectraToFail)
      TS_ASSERT(mask->isMaskedIndex(idx));

    cleanup();
  }

  /**
   * Verify that masked incoming spectra stay masked: regardless of whether PDCalibration would otherwise succeed
   *   in fitting the spectra.
   */
  void test_masked_stay_masked() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    const std::string uniquePrefix = "test_MSM";
    auto [inputWSName, maskWSName, diagnosticWSName, outputWSName, dValues, cleanup] =
        maskTestsInitialization(alg, uniquePrefix);

    const std::set<size_t> spectraToMask{3, 24, 38, 43, 60, 85, 96};
    MaskWorkspace_sptr inputMaskWS = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    for (std::size_t idx : spectraToMask) {
      inputMaskWS->setMaskedIndex(idx, true);
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outputWSName);
    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero
    checkDSpacing(diagnosticWSName + "_dspacing", dValues);

    MaskWorkspace_const_sptr mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    TS_ASSERT(mask);
    TS_ASSERT_EQUALS(mask->getNumberMasked(), spectraToMask.size());
    for (std::size_t idx : spectraToMask)
      TS_ASSERT(mask->isMaskedIndex(idx));

    cleanup();
  }

  /**
   * Verify that masks for incoming spectra are combined with masks for failing spectra, in the output mask workspace
   */
  void test_masks_are_combined() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    const std::string uniquePrefix = "test_MAC";
    auto [inputWSName, maskWSName, diagnosticWSName, outputWSName, dValues, cleanup] =
        maskTestsInitialization(alg, uniquePrefix);

    const std::set<size_t> spectraToMask{3, 24, 38, 43, 60, 85, 96};
    MaskWorkspace_sptr inputMaskWS = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    for (std::size_t idx : spectraToMask) {
      inputMaskWS->setMaskedIndex(idx, true);
    }
    const std::set<size_t> spectraToFail{1, 27, 35, 36, 54, 88, 99};
    EventWorkspace_sptr inputWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputWSName);
    for (std::size_t idx : spectraToFail) {
      inputWS->getSpectrum(idx).clear(false);
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outputWSName);
    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero
    checkDSpacing(diagnosticWSName + "_dspacing", dValues);

    MaskWorkspace_const_sptr mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    TS_ASSERT(mask);
    TS_ASSERT_EQUALS(mask->getNumberMasked(), spectraToMask.size() + spectraToFail.size());
    for (std::size_t idx : spectraToMask)
      TS_ASSERT(mask->isMaskedIndex(idx));
    for (std::size_t idx : spectraToFail)
      TS_ASSERT(mask->isMaskedIndex(idx));

    cleanup();
  }

  /**
   * Verify that the output offset and mask workspaces have detector mask flags which are consistent with the mask
   * values.
   */
  void test_masks_are_consistent_with_detector_flags() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    const std::string uniquePrefix = "test_MACWDF";
    auto [inputWSName, maskWSName, diagnosticWSName, outputWSName, dValues, cleanup] =
        maskTestsInitialization(alg, uniquePrefix);

    const std::set<size_t> spectraToMask{3, 24, 38, 43, 60, 85, 96};
    MaskWorkspace_sptr inputMaskWS = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    for (std::size_t idx : spectraToMask) {
      inputMaskWS->setMaskedIndex(idx, true);
    }
    const std::set<size_t> spectraToFail{1, 27, 35, 36, 54, 88, 99};
    EventWorkspace_sptr inputWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputWSName);
    for (std::size_t idx : spectraToFail) {
      inputWS->getSpectrum(idx).clear(false);
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outputWSName);
    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index = std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero
    checkDSpacing(diagnosticWSName + "_dspacing", dValues);

    MaskWorkspace_const_sptr mask = AnalysisDataService::Instance().retrieveWS<MaskWorkspace>(maskWSName);
    TS_ASSERT(mask);
    TS_ASSERT_EQUALS(mask->getNumberMasked(), spectraToMask.size() + spectraToFail.size());
    TS_ASSERT(mask->isConsistentWithDetectorMasks());

    cleanup();
  }
};

class PDCalibrationTestPerformance : public CxxTest::TestSuite { // TODO

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PDCalibrationTestPerformance *createSuite() { return new PDCalibrationTestPerformance(); }

  static void destroySuite(PDCalibrationTestPerformance *suite) { delete suite; }

  PDCalibrationTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override {
    using Mantid::Kernel::UnitParams;
    // setup the peak postions based on transformation from detID=155
    std::vector<double> dValues(PEAK_TOFS.size());
    Mantid::Kernel::Units::dSpacing dSpacingUnit;
    std::vector<double> unusedy;
    dSpacingUnit.fromTOF(dValues, unusedy, -1., 0, Mantid::Kernel::UnitParametersMap{{UnitParams::difc, DIFC_155}});
    createSampleWS();
    pdc.initialize();
    pdc.setProperty("InputWorkspace", "PDCalibrationTest_WS");
    pdc.setProperty("MaskWorkspace", "outputWS_mask");
    pdc.setProperty("TofBinning", TOF_BINNING);
    pdc.setPropertyValue("OutputCalibrationTable", "outputWS");
    pdc.setPropertyValue("DiagnosticWorkspaces", "diag");
    pdc.setProperty("PeakPositions", dValues);
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("outputWS");
    Mantid::API::AnalysisDataService::Instance().remove("outputWS_mask");
    Mantid::API::AnalysisDataService::Instance().remove("diag");
  }

  void testPerformanceWS() { pdc.execute(); }

private:
  PDCalibration pdc;
};
