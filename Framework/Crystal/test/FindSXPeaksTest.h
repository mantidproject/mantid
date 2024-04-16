// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Axis.h"
#include "MantidCrystal/FindSXPeaks.h"
#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;

// Helper method to overwrite spectra.
void overWriteSpectraY(size_t histo, const Workspace2D_sptr &workspace, const std::vector<double> &Yvalues) {

  workspace->dataY(histo) = Yvalues;
}

// Helper method to make what will be recognised as a single peak.
void makeOnePeak(size_t histo, double peak_intensity, size_t at_bin, const Workspace2D_sptr &workspace) {
  size_t nBins = workspace->y(0).size();
  std::vector<double> peaksInY(nBins);

  for (size_t i = 0; i < nBins; i++) {
    if (i == at_bin) {
      peaksInY[i] = peak_intensity; // overwrite with special value
    } else {
      peaksInY[i] = workspace->y(histo)[i];
    }
  }
  overWriteSpectraY(histo, workspace, peaksInY);
}

/**
 * Helper function to create the FindSXPeaks algorithm.
 *
 * @param workspace :: the workspace to run the algorithm on
 * @param startIndex :: the workspace index to start searching from
 * @param endIndex :: the workspace index to stop searching from
 */
std::unique_ptr<FindSXPeaks> createFindSXPeaks(const Workspace2D_sptr &workspace) {
  auto alg = std::make_unique<FindSXPeaks>();
  alg->setRethrows(true);
  alg->initialize();
  alg->setProperty("InputWorkspace", workspace);
  alg->setProperty("OutputWorkspace", "found_peaks");

  return alg;
}

//=====================================================================================
// Functional tests
//=====================================================================================
class FindSXPeaksTest : public CxxTest::TestSuite {

public:
  void testInvalidIndexRanges() {
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);

    auto alg = createFindSXPeaks(workspace);
    alg->setProperty("StartWorkspaceIndex", 3);
    alg->setProperty("EndWorkspaceIndex", 2);
    TSM_ASSERT_THROWS("Cannot have start index > end index", alg->execute(), const std::invalid_argument &);
  }

  void testFindNoPeaks() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);

    auto alg = createFindSXPeaks(workspace);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Nothing above background in input workspace, should "
                      "have found no peaks!",
                      0, result->rowCount());
  }

  void testFindSinglePeak() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick a peak in histoIndex = 1.
    makeOnePeak(1, 40, 5, workspace);

    auto alg = createFindSXPeaks(workspace);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found one peak!", 1, result->rowCount());
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 40, result->getPeak(0).getIntensity());
  }

  void testFindZeroPeaksWithBoostedBackground() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick a peak in histoIndex = 1.
    makeOnePeak(1, 40, 5, workspace);

    auto alg = createFindSXPeaks(workspace);
    double theresholdIntensity = 40;
    alg->setProperty("SignalBackground",
                     theresholdIntensity); // Boost the background intensity
                                           // threshold level to be the same as
                                           // that of the peak
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Background has been set to 40, should have found no peaks!", 0, result->rowCount());
  }

  void testFindBiggestPeakInSpectra() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick three peaks in histoIndex = 1.
    makeOnePeak(1, 30, 2, workspace);
    makeOnePeak(1, 40, 4, workspace);
    makeOnePeak(1, 60, 6, workspace); // This is the biggest!

    auto alg = createFindSXPeaks(workspace);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found one peak!", 1, result->rowCount());
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 60, result->getPeak(0).getIntensity());
  }

  void testFindManyPeaksInSpectra() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick three peaks in different histograms.
    makeOnePeak(1, 40, 2, workspace);
    makeOnePeak(2, 60, 2, workspace);
    makeOnePeak(3, 45, 2, workspace); // This is the biggest!

    auto alg = createFindSXPeaks(workspace);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found three peaks!", 3, result->rowCount());

    std::vector<double> results(3);
    results[0] = result->getPeak(0).getIntensity();
    results[1] = result->getPeak(1).getIntensity();
    results[2] = result->getPeak(2).getIntensity();
    std::sort(results.begin(), results.end(), std::less<double>());

    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 40, results[0]);
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 45, results[1]);
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 60, results[2]);
  }

  void testWhenMinSpectrasNotFound() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick three peaks in different histograms.
    makeOnePeak(1, 40, 2, workspace);
    makeOnePeak(2, 60, 2, workspace);
    makeOnePeak(3, 45, 2, workspace);

    auto alg = createFindSXPeaks(workspace);
    alg->setProperty("MinNSpectraPerPeak", 2);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found no peaks!", 0, result->rowCount());
  }

  void testWhenMaxSpectraSpecified() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick three peaks in different histograms.
    makeOnePeak(1, 40, 2, workspace);
    makeOnePeak(2, 60, 2, workspace);
    makeOnePeak(3, 45, 2, workspace);

    auto alg = createFindSXPeaks(workspace);
    alg->setProperty("maxNSpectraPerPeak", 3);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found three peaks!", 3, result->rowCount());
  }

  void updateYAndEData(Mantid::HistogramData::HistogramY &y, const std::vector<double> &newYValues,
                       Mantid::HistogramData::HistogramE &e, const std::vector<double> &newErrorValues) {

    if (y.size() != newYValues.size() || e.size() != newErrorValues.size()) {
      throw std::runtime_error("The data sizes don't match. This is a test setup issue. "
                               "Make sure there is one fake data point per entry in the histogram.");
    }

    for (size_t index = 0; index < y.size(); ++index) {
      y[index] = newYValues[index];
      e[index] = newErrorValues[index];
    }
  }

  void testSpectrumWithoutUniqueDetectorsDoesNotThrow() {
    const int nHist = 10;
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nHist, 10);
    makeOnePeak(2, 400, 5, workspace);
    Mantid::DataHandling::GroupDetectors2 grouping;
    grouping.setChild(true);
    grouping.initialize();
    grouping.setProperty("InputWorkspace", workspace);
    grouping.setProperty("OutputWorkspace", "unused_for_child");
    grouping.setProperty("GroupingPattern", "0,1-3,4,5");
    grouping.execute();
    MatrixWorkspace_sptr grouped = grouping.getProperty("OutputWorkspace");
    std::cout << grouped->getNumberHistograms() << '\n';
    auto alg = createFindSXPeaks(workspace);
    TSM_ASSERT_THROWS_NOTHING("FindSXPeak should have thrown.", alg->execute());
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());
  }

  void testSpectrumWithNaNValuesDoesNotThrow() {
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 3);
    // move instrument component so peak
    workspace->dataY(0) = std::vector<double>(3, -1.0);
    workspace->dataY(1) = std::vector<double>{1.0, std::nan(""), 2.0};
    workspace->dataY(2) = std::vector<double>(3, std::nan(""));
    auto alg = createFindSXPeaks(workspace);
    alg->setProperty("PeakFindingStrategy", "AllPeaks");
    alg->setProperty("AbsoluteBackground", "0");
    alg->setProperty("ResolutionStrategy", "AbsoluteResolution");
    alg->setProperty("XResolution", "1000");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));

    TS_ASSERT_EQUALS(result->getNumberPeaks(), 1);
    TS_ASSERT_EQUALS(result->getPeak(0).getIntensity(), 2.0);
  }

  void testUseWorkspaceRangeCropping() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // One peak at an early part (bin) in range.
    makeOnePeak(1, 40, 1, workspace);
    // One peak at a late part (bin) in range
    makeOnePeak(1, 40, 9, workspace);

    auto alg = createFindSXPeaks(workspace);

    double rangeLower = 2;
    double rangeUpper = 8;
    alg->setProperty("RangeLower", rangeLower);
    alg->setProperty("RangeUpper", rangeUpper);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found zero peaks after cropping", 0, result->rowCount());
  }

  void testUseWorkspaceIndexCropping() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);

    // Make two peaks with none in the middle of the workspace (by nhistos).
    makeOnePeak(1, 40, 5, workspace);
    makeOnePeak(9, 40, 5, workspace);

    auto alg = createFindSXPeaks(workspace);

    // Crop leaving only the narrow few histos in the center of the workspace.
    int startIndex = 2;
    int endIndex = 4;
    alg->setProperty("StartWorkspaceIndex", startIndex);
    alg->setProperty("EndWorkspaceIndex", endIndex);

    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found zero peaks after cropping", 0, result->rowCount());
  }

  void testSetGoniometer() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick a peak in histoIndex = 1.
    makeOnePeak(1, 40, 5, workspace);

    // Get baseline for Q of Peak
    auto alg = createFindSXPeaks(workspace);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found one peak!", 1, result->rowCount());

    Mantid::Kernel::V3D qNoRot = result->getPeak(0).getQSampleFrame();

    // Set Goniometer to 180 degrees
    Mantid::Geometry::Goniometer gonio;
    gonio.makeUniversalGoniometer();
    gonio.setRotationAngle(1, 180);
    workspace->mutableRun().setGoniometer(gonio, false);

    // Find peaks again
    alg = createFindSXPeaks(workspace);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found one peak!", 1, result->rowCount());

    Mantid::Kernel::V3D qRot = result->getPeak(0).getQSampleFrame();

    // Peak should be rotated by 180 degrees around y in Q compared to baseline
    // Use ASSERT_DELTA to account for minor error introduced by deg/rad
    // conversion
    TSM_ASSERT_DELTA("Q_x should be unchanged!", qNoRot.X(), qRot.X(), 10e-10);
    TSM_ASSERT_DELTA("Q_y should be inverted!", qNoRot.Y(), qRot.Y() * (-1), 10e-10);
    TSM_ASSERT_DELTA("Q_z should be unchanged!", qNoRot.Z(), qRot.Z(), 10e-10);
  }

  void testFindBiggestPeakInSpectraWithDSpacing() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);

    // Change units of workspace
    const auto xAxis = workspace->getAxis(0);
    xAxis->setUnit("dSpacing");

    // Stick three peaks in histoIndex = 1.
    makeOnePeak(1, 30, 2, workspace);
    makeOnePeak(1, 40, 4, workspace);
    makeOnePeak(1, 60, 6, workspace); // This is the biggest!

    auto alg = createFindSXPeaks(workspace);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found one peak!", 1, result->rowCount());
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 60, result->getPeak(0).getIntensity());
    TSM_ASSERT_DELTA("Wrong peak TOF matched on found peak", 821.43, result->getPeak(0).getTOF(), 1e-2);
  }

  void testFindManyPeaksInSpectraWithDSpacing() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);

    const auto xAxis = workspace->getAxis(0);
    xAxis->setUnit("dSpacing");

    // Stick three peaks in different histograms.
    makeOnePeak(1, 40, 2, workspace);
    makeOnePeak(4, 60, 5, workspace);
    makeOnePeak(8, 45, 8, workspace);

    auto alg = createFindSXPeaks(workspace);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found three peaks!", 3, result->rowCount());

    std::array<double, 3> results;
    results[0] = result->getPeak(0).getIntensity();
    results[1] = result->getPeak(1).getIntensity();
    results[2] = result->getPeak(2).getIntensity();
    std::sort(results.begin(), results.end(), std::less<double>());

    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 40, results[0]);
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 45, results[1]);
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 60, results[2]);

    std::array<double, 3> tof;
    tof[0] = result->getPeak(0).getTOF();
    tof[1] = result->getPeak(1).getTOF();
    tof[2] = result->getPeak(2).getTOF();
    std::sort(tof.begin(), tof.end(), std::less<double>());

    TSM_ASSERT_DELTA("Wrong peak TOF matched on found peak", 315.938, tof[0], 1e-1);
    TSM_ASSERT_DELTA("Wrong peak TOF matched on found peak", 2775.689, tof[1], 1e-1);
    TSM_ASSERT_DELTA("Wrong peak TOF matched on found peak", 8534.953, tof[2], 1e-1);
  }
};

//=====================================================================================
// Performance tests
//=====================================================================================
class FindSXPeaksTestPerformance : public CxxTest::TestSuite {
private:
  int m_nHistograms;
  Workspace2D_sptr m_workspace2D;

public:
  static FindSXPeaksTestPerformance *createSuite() { return new FindSXPeaksTestPerformance(); }
  static void destroySuite(FindSXPeaksTestPerformance *suite) { delete suite; }

  FindSXPeaksTestPerformance() : m_nHistograms(5000) {}

  void setUp() override {
    m_workspace2D = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(m_nHistograms, 10);
    // Make 99 well separated peaks
    for (int i = 1; i < m_nHistograms; i += 5) {
      makeOnePeak(i, 40, 5, m_workspace2D);
    }
  }

  void testSXPeakFinding() {
    auto alg = createFindSXPeaks(m_workspace2D);
    alg->execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg->isExecuted());

    IPeaksWorkspace_sptr result = std::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    std::cout << "Number of Peaks Found: " << result->rowCount() << '\n';
    TSM_ASSERT("Should have found many peaks!", 0 < result->rowCount());
  }
};
