#ifndef FIND_SX_PEAKSTEST_H_
#define FIND_SX_PEAKSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidCrystal/FindSXPeaks.h"
#include "MantidGeometry/Crystal/IPeak.h"

using namespace Mantid::API;
using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;

// Helper method to overwrite spectra.
void overWriteSpectraY(size_t histo, Workspace2D_sptr workspace,
                       const Mantid::MantidVec &Yvalues) {
  Mantid::MantidVec &Y = workspace->dataY(histo);
  for (size_t i = 0; i < Y.size(); i++) {
    Y[i] = Yvalues[i];
  }
}

// Helper method to make what will be recognised as a single peak.
void makeOnePeak(size_t histo, double peak_intensity, size_t at_bin,
                 Workspace2D_sptr workspace) {
  size_t nBins = workspace->readY(0).size();
  Mantid::MantidVec peaksInY(nBins);

  for (size_t i = 0; i < nBins; i++) {
    if (i == at_bin) {
      peaksInY[i] = peak_intensity; // overwrite with special value
    } else {
      peaksInY[i] = workspace->readY(histo)[i];
    }
  }
  overWriteSpectraY(histo, workspace, peaksInY);
}

//=====================================================================================
// Functional tests
//=====================================================================================
class FindSXPeaksTest : public CxxTest::TestSuite {

public:
  // Test out of bounds constuction arguments
  void testSXPeakConstructorThrowsIfNegativeIntensity() {
    double intensity = -1; // Negative intensity.
    std::vector<int> spectra(1, 1);
    double detectorDistance = 3;
    TSM_ASSERT_THROWS(
        "SXPeak: Should not construct with a negative intensity",
        SXPeak(0.001, 0.02, 0.01, intensity, spectra, detectorDistance, 1),
        std::invalid_argument);
  }

  // Test out of bounds construction arguments.
  void testSXPeakConstructorThrowsIfSpectraSizeZero() {
    double intensity = 1;
    std::vector<int> spectra; // Zero size spectra list
    double detectorDistance = 3;
    TSM_ASSERT_THROWS(
        "SXPeak: Should not construct with a zero size specral list",
        SXPeak(0.001, 0.02, 0.01, intensity, spectra, detectorDistance, 1),
        std::invalid_argument);
  }

  // Test out of bounds construction arguments.
  void testSXPeakConstructorThrowsIfNegativeDetectorDistance() {
    double intensity = 1;
    std::vector<int> spectra(1, 1);
    double detectorDistance = -1; // Negative detector distance
    TSM_ASSERT_THROWS(
        "SXPeak: Should not construct with a zero size specral list",
        SXPeak(0.001, 0.02, 0.01, intensity, spectra, detectorDistance, 1),
        std::invalid_argument);
  }

  void testSXPeakGetters() {
    double intensity = 1;
    std::vector<int> spectra(1, 1);
    double detectorDistance = 3;
    SXPeak peak(0.001, 0.02, 0.01, intensity, spectra, detectorDistance, 2);

    TSM_ASSERT_EQUALS("Intensity getter is not wired-up correctly", 1,
                      peak.getIntensity());
    TSM_ASSERT_EQUALS("Detector Id getter is not wired-up correctly", 2,
                      peak.getDetectorId());
    // QSpace is also a getter, but is tested more thouroughly below.
  }

  void testInvalidIndexRanges() {
    Workspace2D_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);

    FindSXPeaks alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "found_peaks");
    alg.setProperty("StartWorkspaceIndex", 3);
    alg.setProperty("EndWorkspaceIndex", 2);
    TSM_ASSERT_THROWS("Cannot have start index > end index", alg.execute(),
                      std::invalid_argument);
  }

  void testFindNoPeaks() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);

    FindSXPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "found_peaks");
    alg.execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg.isExecuted());

    IPeaksWorkspace_sptr result = boost::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Nothing above background in input workspace, should "
                      "have found no peaks!",
                      0, result->rowCount());
  }

  void testFindSinglePeak() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick a peak in histoIndex = 1.
    makeOnePeak(1, 40, 5, workspace);

    FindSXPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "found_peaks");
    alg.execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg.isExecuted());

    IPeaksWorkspace_sptr result = boost::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found one peak!", 1, result->rowCount());
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 40,
                      result->getPeak(0).getIntensity());
  }

  void testFindZeroPeaksWithBoostedBackground() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick a peak in histoIndex = 1.
    makeOnePeak(1, 40, 5, workspace);

    FindSXPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "found_peaks");
    double theresholdIntensity = 40;
    alg.setProperty("SignalBackground",
                    theresholdIntensity); // Boost the background intensity
                                          // threshold level to be the same as
                                          // that of the peak
    alg.execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg.isExecuted());

    IPeaksWorkspace_sptr result = boost::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS(
        "Background has been set to 40, should have found no peaks!", 0,
        result->rowCount());
  }

  void testFindBiggestPeakInSpectra() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick three peaks in histoIndex = 1.
    makeOnePeak(1, 30, 2, workspace);
    makeOnePeak(1, 40, 4, workspace);
    makeOnePeak(1, 60, 6, workspace); // This is the biggest!

    FindSXPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "found_peaks");
    alg.execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg.isExecuted());

    IPeaksWorkspace_sptr result = boost::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found one peak!", 1, result->rowCount());
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 60,
                      result->getPeak(0).getIntensity());
  }

  void testFindManyPeaksInSpectra() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // Stick three peaks in histoIndex = 1.
    makeOnePeak(1, 40, 2, workspace);
    makeOnePeak(2, 60, 2, workspace);
    makeOnePeak(3, 45, 2, workspace); // This is the biggest!

    FindSXPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "found_peaks");
    alg.execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg.isExecuted());

    IPeaksWorkspace_sptr result = boost::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found three peaks!", 3, result->rowCount());

    std::vector<double> results(3);
    results[0] = result->getPeak(0).getIntensity();
    results[1] = result->getPeak(1).getIntensity();
    results[2] = result->getPeak(2).getIntensity();
    std::sort(results.begin(), results.end(), std::less<double>());

    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 40,
                      results[0]);
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 45,
                      results[1]);
    TSM_ASSERT_EQUALS("Wrong peak intensity matched on found peak", 60,
                      results[2]);
  }

  void testUseWorkspaceRangeCropping() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    // One peak at an early part (bin) in range.
    makeOnePeak(1, 40, 1, workspace);
    // One peak at a late part (bin) in range
    makeOnePeak(1, 40, 9, workspace);

    FindSXPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "found_peaks");

    double rangeLower = 2;
    double rangeUpper = 8;
    alg.setProperty("RangeLower", rangeLower);
    alg.setProperty("RangeUpper", rangeUpper);
    alg.execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg.isExecuted());

    IPeaksWorkspace_sptr result = boost::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found zero peaks after cropping", 0,
                      result->rowCount());
  }

  void testUseWorkspaceIndexCropping() {
    // creates a workspace where all y-values are 2
    Workspace2D_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);

    // Make two peaks with none in the middle of the workspace (by nhistos).
    makeOnePeak(1, 40, 5, workspace);
    makeOnePeak(9, 40, 5, workspace);

    FindSXPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", "found_peaks");
    // Crop leaving only the narrow few histos in the center of the workspace.
    int startIndex = 2;
    int endIndex = 4;
    alg.setProperty("StartWorkspaceIndex", startIndex);
    alg.setProperty("EndWorkspaceIndex", endIndex);

    alg.execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg.isExecuted());

    IPeaksWorkspace_sptr result = boost::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    TSM_ASSERT_EQUALS("Should have found zero peaks after cropping", 0,
                      result->rowCount());
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
  static FindSXPeaksTestPerformance *createSuite() {
    return new FindSXPeaksTestPerformance();
  }
  static void destroySuite(FindSXPeaksTestPerformance *suite) { delete suite; }

  FindSXPeaksTestPerformance() : m_nHistograms(5000) {}

  void setUp() {
    m_workspace2D =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            m_nHistograms, 10);
    // Make 99 well separated peaks
    for (int i = 1; i < m_nHistograms; i += 5) {
      makeOnePeak(i, 40, 5, m_workspace2D);
    }
  }

  void testSXPeakFinding() {
    FindSXPeaks alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", m_workspace2D);
    alg.setProperty("OutputWorkspace", "found_peaks");
    alg.execute();
    TSM_ASSERT("FindSXPeak should have been executed.", alg.isExecuted());

    IPeaksWorkspace_sptr result = boost::dynamic_pointer_cast<IPeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("found_peaks"));
    std::cout << "Number of Peaks Found: " << result->rowCount() << std::endl;
    TSM_ASSERT("Should have found many peaks!", 0 < result->rowCount());
  }
};

#endif
