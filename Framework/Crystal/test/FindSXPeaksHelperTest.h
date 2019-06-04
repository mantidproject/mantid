// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_FINDSXPEAKSHELPERTEST_H_
#define MANTID_CRYSTAL_FINDSXPEAKSHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/FindSXPeaksHelper.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <gmock/gmock.h>

#include <string>

using namespace Mantid::Crystal::FindSXPeaksHelper;
using namespace testing;

namespace {

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  MOCK_METHOD1(doReport, void(const std::string &));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
} // namespace

class FindSXPeaksHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindSXPeaksHelperTest *createSuite() {
    return new FindSXPeaksHelperTest();
  }
  static void destroySuite(FindSXPeaksHelperTest *suite) { delete suite; }

  /* ------------------------------------------------------------------------------------------
   * Single Crystal peak representation
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
                      const std::invalid_argument &);
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
        const std::invalid_argument &);
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
  }

  /* ------------------------------------------------------------------------------------------
   * Background Strategy
   * ------------------------------------------------------------------------------------------
   */
  void testThatAbsoluteBackgroundPerformsRightComparison() {
    // GIVEN
    auto workspace = WorkspaceCreationHelper::create1DWorkspaceConstant(
        10 /*size*/, 1.5 /*value*/, 1. /*error*/, true /*isHisto*/);
    const auto &y = workspace->y(0);

    // WHEN
    auto backgroundStrategy = AbsoluteBackgroundStrategy(2.);

    // THEN
    TSM_ASSERT("The intensity should be below the background",
               backgroundStrategy.isBelowBackground(1., y));
    TSM_ASSERT("The intensity should be above the background",
               !backgroundStrategy.isBelowBackground(2., y));
  }

  void testThatPerSpectrumBackgroundStrategyPerformsRightComparison() {
    // GIVEN
    auto workspace = WorkspaceCreationHelper::create1DWorkspaceConstant(
        10 /*size*/, 1.5 /*value*/, 1. /*error*/, true /*isHisto*/);
    const auto &y = workspace->y(0);

    // WHEN
    auto backgroundStrategy = PerSpectrumBackgroundStrategy(1.);

    // THEN
    TSM_ASSERT("The intensity should be below the background",
               backgroundStrategy.isBelowBackground(1., y));
    TSM_ASSERT("The intensity should be above the background",
               !backgroundStrategy.isBelowBackground(2., y));
  }

  /* ------------------------------------------------------------------------------------------
   * Peak Finding strategy
   * ------------------------------------------------------------------------------------------
   */
  void testThatFindsStrongestPeakWhenPerSpectrumBackgroundStrategyIsUsed() {
    auto backgroundStrategy =
        std::make_unique<PerSpectrumBackgroundStrategy>(1.);
    doRunStrongestPeakTest(backgroundStrategy.get());
  }

  void testThatFindsStrongestPeakWhenAbsoluteBackgroundStrategyIsUsed() {
    auto backgroundStrategy =
        std::make_unique<AbsoluteBackgroundStrategy>(3.);
    doRunStrongestPeakTest(backgroundStrategy.get());
  }

  void testThatFindsAllPeaksWhenAbsoluteBackgroundStrategyIsUsed() {
    // GIVEN
    auto backgroundStrategy =
        std::make_unique<AbsoluteBackgroundStrategy>(3.);
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            1 /*nhist*/, 15 /*nbins*/);
    auto &mutableY = workspace->mutableY(0);
    doAddDoublePeakToData(mutableY);

    const int workspaceIndex = 0;
    const auto &x = workspace->x(0);
    const auto &y = workspace->y(0);
    const auto &spectrumInfo = workspace->spectrumInfo();

    // WHEN
    auto peakFindingStrategy = std::make_unique<AllPeaksStrategy>(
        backgroundStrategy.get(), spectrumInfo);
    auto peaks = peakFindingStrategy->findSXPeaks(x, y, workspaceIndex);

    // THEN
    TSM_ASSERT("There should be two peaks that are found.",
               peaks.get().size() == 2);
    TSM_ASSERT("The first peak should have a signal value of 7.",
               peaks.get()[0].getIntensity() == 7.);
    TSM_ASSERT("The second peak should have a signal value of 11.",
               peaks.get()[1].getIntensity() == 11.);
  }

  void
  testThatThrowsWhenBackgroundStrategyIsNotAbsoluteBackgroundStrategyWhenUsingAllPeaksStrategy() {
    // Note that the AllPeaksStrategy is currently only supporting the absolute
    // background strategy
    // GIVEN
    auto backgroundStrategy =
        std::make_unique<PerSpectrumBackgroundStrategy>(3.);
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            1 /*nhist*/, 15 /*nbins*/);
    const auto &spectrumInfo = workspace->spectrumInfo();

    // WHEN + THEN
    TSM_ASSERT_THROWS("Should throw a invalid argument error when background "
                      "strategy is not AbsoluteBackgroundStrategy",
                      std::make_unique<AllPeaksStrategy>(
                          backgroundStrategy.get(), spectrumInfo);
                      , const std::invalid_argument &);
  }

  void testThatCanReduceWithSimpleReduceStrategy() {
    // GIVEN
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    const auto &spectrumInfo = workspace->spectrumInfo();

    const auto resolution = 0.001;
    auto compareStrategy =
        std::make_unique<RelativeCompareStrategy>(resolution);
    auto simpleStrategy = std::make_unique<SimpleReduceStrategy>(
        compareStrategy.get());

    std::vector<SXPeak> peaks;
    peaks.emplace_back(1 /*TOF*/, 1 /*phi*/, 0.1 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);
    peaks.emplace_back(1 /*TOF*/, 1 /*phi*/, 0.2 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);

    peaks.emplace_back(1 /*TOF*/, 1.1 /*phi*/, 0.3 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);
    peaks.emplace_back(1 /*TOF*/, 1.1001 /*phi*/, 0.4 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);

    peaks.emplace_back(3 /*TOF*/, 2 /*phi*/, 0.5 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);
    peaks.emplace_back(3 /*TOF*/, 2.0001 /*phi*/, 0.6 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);

    PeakList peakList = peaks;

    NiceMock<MockProgressBase> progress;
    EXPECT_CALL(progress, doReport(_))
        .Times(0); // We only report if there are more than 50 peaks

    // WHEN
    auto reducedPeaks = simpleStrategy->reduce(peakList.get(), progress);

    // THEN
    const double tolerance = 1e-6;
    TSM_ASSERT("Should have three peaks", reducedPeaks.size() == 3);
    TSM_ASSERT("Should have a value of 0.1 + 0.2 = 0.3",
               std::abs(reducedPeaks[0].getIntensity() - 0.3) < tolerance);
    TSM_ASSERT("Should have a value of 0.3 + 0.4 = 0.7",
               std::abs(reducedPeaks[1].getIntensity() - 0.7) < tolerance);
    TSM_ASSERT("Should have a value of 0.5 + 0.6 = 01.1",
               std::abs(reducedPeaks[2].getIntensity() - 1.1) < tolerance);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&progress));
  }

  void testThatCanReduceWithFindMaxReduceStrategy() {
    // GIVEN
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10);
    const auto &spectrumInfo = workspace->spectrumInfo();

    const auto resolution = 0.001;
    auto compareStrategy =
        std::make_unique<RelativeCompareStrategy>(resolution);
    auto findMaxReduceStrategy =
        std::make_unique<FindMaxReduceStrategy>(
            compareStrategy.get());

    std::vector<SXPeak> peaks;
    peaks.emplace_back(1 /*TOF*/, 0.99 /*phi*/, 0.1 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);
    peaks.emplace_back(1 /*TOF*/, 0.99 /*phi*/, 0.2 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);

    peaks.emplace_back(1 /*TOF*/, 1.1 /*phi*/, 0.3 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);
    peaks.emplace_back(1 /*TOF*/, 1.1001 /*phi*/, 0.4 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);

    peaks.emplace_back(3 /*TOF*/, 2 /*phi*/, 0.5 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);
    peaks.emplace_back(3 /*TOF*/, 2.0001 /*phi*/, 0.6 /*intensity*/,
                       std::vector<int>(1, 1), 1, spectrumInfo);

    PeakList peakList = peaks;

    NiceMock<MockProgressBase> progress;
    EXPECT_CALL(progress, doReport(_))
        .Times(0); // We only report if there are more than 50 peaks

    // WHEN
    const auto reducedPeaks =
        findMaxReduceStrategy->reduce(peakList.get(), progress);

    // THEN
    const double tolerance = 1e-6;
    TSM_ASSERT("Should have three peaks", reducedPeaks.size() == 3);
    TSM_ASSERT("Should have a value of max(0.1, 0.2) = 0.2",
               std::abs(reducedPeaks[0].getIntensity() - 0.2) < tolerance);
    TSM_ASSERT("Should have a value of max(0.1, 0.2) = 0.2",
               std::abs(reducedPeaks[1].getIntensity() - 0.4) < tolerance);
    TSM_ASSERT("Should have a value of max(0.1, 0.2) = 0.2",
               std::abs(reducedPeaks[2].getIntensity() - 0.6) < tolerance);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&progress));
  }

  /* ------------------------------------------------------------------------------------------
   * Comparison Strategy
   * ------------------------------------------------------------------------------------------
   */
  void testThatRelativeComparisonWorks() {
    // GIVEN
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 2);
    const auto &spectrumInfo = workspace->spectrumInfo();
    SXPeak peak1(1. /*TOF*/, 0.99 /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);
    SXPeak peak2(1. /*TOF*/, 0.90 /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);
    SXPeak peak3(1. /*TOF*/, 1.99 /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);

    const auto resolution = 0.1;
    auto compareStrategy =
        std::make_unique<RelativeCompareStrategy>(resolution);

    // WHEN
    auto result12 = compareStrategy->compare(peak1, peak2);
    auto result13 = compareStrategy->compare(peak1, peak3);

    // THEN
    TSM_ASSERT("The peaks should be the same", result12)
    TSM_ASSERT("The peaks should not be the same", !result13)
  }

  void testThatAbsoluteComparisonWorks() {
    // GIVEN
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 2);
    const auto &spectrumInfo = workspace->spectrumInfo();

    const double degreeToRad = M_PI / 180.;
    SXPeak peak1(1. /*TOF*/, degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);
    SXPeak peak2(1.5 /*TOF*/, degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);
    SXPeak peak3(3. /*TOF*/, degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);

    SXPeak peak4(1. /*TOF*/, degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);
    SXPeak peak5(1. /*TOF*/, 1.5 * degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);
    SXPeak peak6(1. /*TOF*/, 3. * degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);

    const auto tofResolution = 1.;
    const auto thetaResolution = 1.;
    const auto phiResolution = 1.;
    auto compareStrategy = std::make_unique<AbsoluteCompareStrategy>(
        tofResolution, thetaResolution, phiResolution);

    // WHEN
    auto result12 = compareStrategy->compare(peak1, peak2);
    auto result13 = compareStrategy->compare(peak1, peak3);

    auto result45 = compareStrategy->compare(peak4, peak5);
    auto result46 = compareStrategy->compare(peak4, peak6);

    // THEN
    TSM_ASSERT("The peaks should be the same", result12)
    TSM_ASSERT("The peaks should not be the same", !result13)
    TSM_ASSERT("The peaks should be the same", result45)
    TSM_ASSERT("The peaks should not be the same", !result46)
  }

  void
  testGivenWorkspaceInDSpacingWhenAbsoluteComparisonThatCorrectNumberOfPeaks() {
    // GIVEN
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 2);
    const auto &spectrumInfo = workspace->spectrumInfo();

    constexpr double degreeToRad = M_PI / 180.;
    SXPeak peak1(1. /*TOF*/, degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);
    SXPeak peak2(1.5 /*TOF*/, degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);
    SXPeak peak3(3. /*TOF*/, degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);

    SXPeak peak4(1. /*TOF*/, degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);
    SXPeak peak5(1. /*TOF*/, 1.5 * degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);
    SXPeak peak6(1. /*TOF*/, 3. * degreeToRad /*phi*/, 0.1 /*intensity*/,
                 std::vector<int>(1, 1), 1, spectrumInfo);

    const auto dResolution = 0.01;
    const auto thetaResolution = 1.;
    const auto phiResolution = 1.;
    auto compareStrategy = std::make_unique<AbsoluteCompareStrategy>(
        dResolution, thetaResolution, phiResolution, XAxisUnit::DSPACING);

    // WHEN
    auto result12 = compareStrategy->compare(peak1, peak2);
    auto result13 = compareStrategy->compare(peak1, peak3);

    auto result45 = compareStrategy->compare(peak4, peak5);
    auto result46 = compareStrategy->compare(peak4, peak6);

    // THEN
    TSM_ASSERT("The peaks should be the same", result12)
    TSM_ASSERT("The peaks should not be the same", !result13)
    TSM_ASSERT("The peaks should be the same", result45)
    TSM_ASSERT("The peaks should not be the same", !result46)
  }

private:
  void doRunStrongestPeakTest(BackgroundStrategy *backgroundStrategy) {
    // GIVEN
    auto workspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            1 /*nhist*/, 15 /*nbins*/);
    auto &mutableY = workspace->mutableY(0);
    doAddDoublePeakToData(mutableY);

    const int workspaceIndex = 0;
    const auto &x = workspace->x(0);
    const auto &y = workspace->y(0);
    const auto &spectrumInfo = workspace->spectrumInfo();

    // WHEN
    auto peakFindingStrategy =
        std::make_unique<StrongestPeaksStrategy>(backgroundStrategy,
                                                            spectrumInfo);
    auto peaks = peakFindingStrategy->findSXPeaks(x, y, workspaceIndex);

    // THEN
    TSM_ASSERT("There should only be one peak that is found.",
               peaks.get().size() == 1);
    TSM_ASSERT("The peak should have a signal value of 11.",
               peaks.get()[0].getIntensity() == 11.);
  }

  void doAddDoublePeakToData(Mantid::HistogramData::HistogramY &y) {
    std::vector<double> newDataValues = {1.5, 1.5,  3.0, 5.0, 7.0,
                                         4.0, 1.5,  1.5, 1.5, 6.0,
                                         9.0, 11.0, 2.5, 1.5, 1.5};
    if (y.size() != newDataValues.size()) {
      throw std::runtime_error(
          "The data sizes don't match. This is a test setup issue. "
          "Make sure there is one fake data point per entry in the histogram.");
    }

    for (size_t index = 0; index < y.size(); ++index) {
      y[index] = newDataValues[index];
    }
  }
};

#endif /* MANTID_CRYSTAL_FINDSXPEAKSHELPERTEST_H_ */
