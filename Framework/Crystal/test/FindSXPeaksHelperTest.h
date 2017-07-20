#ifndef MANTID_CRYSTAL_FINDSXPEAKSHELPERTEST_H_
#define MANTID_CRYSTAL_FINDSXPEAKSHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/FindSXPeaksHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"


using namespace Mantid::Crystal::FindSXPeaksHelper;

class FindSXPeaksHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindSXPeaksHelperTest *createSuite() { return new FindSXPeaksHelperTest(); }
  static void destroySuite( FindSXPeaksHelperTest *suite ) { delete suite; }


/* ------------------------------------------------------------------------------------------
 * Single Cruystal peak representation
 * ------------------------------------------------------------------------------------------
 */

  // Test out of bounds constuction arguments
  void testSXPeakConstructorThrowsIfNegativeIntensity() {
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    const auto &spectrumInfo = workspace->spectrumInfo();
    double intensity = -1; // Negative intensity.
    std::vector<int> spectra(1, 1);
    TSM_ASSERT_THROWS("SXPeak: Should not construct with a negative intensity",
                      SXPeak(0.001, 0.02, intensity, spectra, 0, spectrumInfo),
                      std::invalid_argument);
  }

  // Test out of bounds construction arguments.
  void testSXPeakConstructorThrowsIfSpectraSizeZero() {
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    const auto &spectrumInfo = workspace->spectrumInfo();
    double intensity = 1;
    std::vector<int> spectra; // Zero size spectra list
    TSM_ASSERT_THROWS(
        "SXPeak: Should not construct with a zero size specral list",
        SXPeak(0.001, 0.02, intensity, spectra, 0, spectrumInfo),
        std::invalid_argument);
  }

  void testSXPeakGetters() {
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    const auto &spectrumInfo = workspace->spectrumInfo();
    double intensity = 1;
    std::vector<int> spectra(1, 1);
    SXPeak peak(0.001, 0.02, intensity, spectra, 1, spectrumInfo);

    TSM_ASSERT_EQUALS("Intensity getter is not wired-up correctly", 1,
                      peak.getIntensity());
    TSM_ASSERT_EQUALS("Detector Id getter is not wired-up correctly", 2,
                      peak.getDetectorId());
    // QSpace is also a getter, but is tested more thouroughly below.
  }


/* ------------------------------------------------------------------------------------------
 * Background Strategy
 * ------------------------------------------------------------------------------------------
 */
  void testThatAbsoluteBackgroundPerformsRightComparison () {
    // GIVEN
    auto workspace =
        WorkspaceCreationHelper::create1DWorkspaceConstant(10 /*size*/, 1.5 /*value*/, 1. /*error*/, true /*isHisto*/);
    const auto y =  workspace->y(0);

    // WHEN
    auto backgroundStrategy = AbsoluteBackgroundStrategy(2.);

    // THEN
    TSM_ASSERT("The intensity should be below the background", backgroundStrategy.isBelowBackground(1., y));
    TSM_ASSERT("The intensity should be above the background", !backgroundStrategy.isBelowBackground(2., y));
  }

  void testThatPerSpectrumBackgroundStrategyPerformsRightComparison () {
    // GIVEN
    auto workspace =
        WorkspaceCreationHelper::create1DWorkspaceConstant(10 /*size*/, 1.5 /*value*/, 1. /*error*/, true /*isHisto*/);
    const auto y =  workspace->y(0);

    // WHEN
    auto backgroundStrategy = PerSpectrumBackgroundStrategy(1.);

    // THEN
    TSM_ASSERT("The intensity should be below the background", backgroundStrategy.isBelowBackground(1., y));
    TSM_ASSERT("The intensity should be above the background", !backgroundStrategy.isBelowBackground(2., y));
  }

  /* ------------------------------------------------------------------------------------------
   * Peak Finding strategy
   * ------------------------------------------------------------------------------------------
   */
  void testThatFindsStrongestPeakWhenPerSpectrumBackgroundStrategyIsUsed () {
    auto backgroundStrategy = Mantid::Kernel::make_unique<PerSpectrumBackgroundStrategy>(1.);
    doRunStrongestPeakTest(backgroundStrategy.get());
  }

  void testThatFindsStrongestPeakWhenAbsoluteBackgroundStrategyIsUsed () {
    auto backgroundStrategy = Mantid::Kernel::make_unique<AbsoluteBackgroundStrategy>(3.);
    doRunStrongestPeakTest(backgroundStrategy.get());
  }

  void testThatFindsAllPeaksWhenAbsoluteBackgroundStrategyIsUsed () {
    // GIVEN
    auto backgroundStrategy = Mantid::Kernel::make_unique<AbsoluteBackgroundStrategy>(3.);
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1 /*nhist*/, 15 /*nbins*/);
    auto& mutableY = workspace->mutableY(0);
    doAddDoublePeakToData(mutableY);

    const int workspaceIndex = 0;
    const auto& x =  workspace->x(0);
    const auto& y =  workspace->y(0);
    const auto& spectrumInfo = workspace->spectrumInfo();

    // WHEN
    auto peakFindingStrategy = Mantid::Kernel::make_unique<AllPeaksStrategy>(backgroundStrategy.get(), spectrumInfo);
    auto peaks = peakFindingStrategy->findSXPeaks(x, y, workspaceIndex);

    // THEN
    TSM_ASSERT("There should only be one peak that is found.", peaks.get().size() == 2);
    TSM_ASSERT("The first peak should have a signal value of 7.", peaks.get()[0].getIntensity() == 7.);
    TSM_ASSERT("The first peak should have a signal value of 11.", peaks.get()[1].getIntensity() == 11.);
  }


  void testThatThrowsWhenBackgroundStrategyIsNotAbsoluteBackgroundStrategy() {
    // GIVEN
    auto backgroundStrategy = Mantid::Kernel::make_unique<PerSpectrumBackgroundStrategy>(3.);
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1 /*nhist*/, 15 /*nbins*/);
    const auto& spectrumInfo = workspace->spectrumInfo();

    // WHEN + THEN
    TSM_ASSERT_THROWS("Should throw a invalid argument error when background strategy is not AbsoluteBackgroundStrategy",
                      Mantid::Kernel::make_unique<AllPeaksStrategy>(backgroundStrategy.get(), spectrumInfo);,
                      std::invalid_argument);

  }

private:

  void doRunStrongestPeakTest(BackgroundStrategy* backgroundStrategy) {
    // GIVEN
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1 /*nhist*/, 15 /*nbins*/);
    auto& mutableY = workspace->mutableY(0);
    doAddDoublePeakToData(mutableY);

    const int workspaceIndex = 0;
    const auto& x =  workspace->x(0);
    const auto& y =  workspace->y(0);
    const auto& spectrumInfo = workspace->spectrumInfo();

    // WHEN
    auto peakFindingStrategy = Mantid::Kernel::make_unique<StrongestPeaksStrategy>(backgroundStrategy, spectrumInfo);
    auto peaks = peakFindingStrategy->findSXPeaks(x, y, workspaceIndex);

    // THEN
    TSM_ASSERT("There should only be one peak that is found.", peaks.get().size() == 1);
    TSM_ASSERT("The peak should have a signal value of 11.", peaks.get()[0].getIntensity() == 11.);
  }

  void doAddDoublePeakToData(Mantid::HistogramData::HistogramY& y) {
    std::vector<double> newDataValues = {1.5, 1.5, 3.0, 5.0, 7.0, 4.0, 1.5, 1.5, 1.5, 6.0, 9.0, 11.0, 2.5, 1.5, 1.5};
    if (y.size() != newDataValues.size()) {
      throw std::runtime_error("The data sizes don't match. This is a test setup issue. "
                               "Make sure there is one fake data point per entry in the histogram.");
    }

    for (size_t index=0; index < y.size(); ++index) {
      y[index] = newDataValues[index];
    }
  }

};


#endif /* MANTID_CRYSTAL_FINDSXPEAKSHELPERTEST_H_ */
