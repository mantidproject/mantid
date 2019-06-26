// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_UNWRAPMONITORSINTOFTEST_H_
#define MANTID_ALGORITHMS_UNWRAPMONITORSINTOFTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/UnwrapMonitorsInTOF.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::UnwrapMonitorsInTOF;

class UnwrapMonitorsInTOFTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static UnwrapMonitorsInTOFTest *createSuite() {
    return new UnwrapMonitorsInTOFTest();
  }
  static void destroySuite(UnwrapMonitorsInTOFTest *suite) { delete suite; }

  void test_Init() {
    UnwrapMonitorsInTOF alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void
  test_that_linear_workspace_has_the_monitors_duplicated_if_no_wavelength_limits_are_set() {
    // Arrange
    auto workspace = provideTestWorkspace(true);
    std::string outputName = "test_output_unwrap_monitors";

    // Act
    UnwrapMonitorsInTOF alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", outputName);
    alg.execute();

    // Assert
    auto &ads = Mantid::API::AnalysisDataService::Instance();
    auto outputWorkspace =
        ads.retrieveWS<Mantid::API::MatrixWorkspace>(outputName);

    // Check that monitor 3 and 4 have their x and y data doubled, ie.
    // Value:  2  2  2  2  2  1  1  1  1  1   2   2   2   2   2   1   1   1   1
    // 1
    // Time : 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18
    // 19  20  | * 1e4 (in microseconds)
    const double tolerance = 1e-8;
    std::array<size_t, 2> indicesToCheck{{3, 4}};
    Mantid::HistogramData::BinEdges expectedBinEdges{
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    expectedBinEdges *= 1e4;
    Mantid::HistogramData::Counts expectedCounts{2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
                                                 2, 2, 2, 2, 2, 1, 1, 1, 1, 1};
    for (const auto &index : indicesToCheck) {
      auto histogram = outputWorkspace->histogram(index);
      auto binEdges = histogram.binEdges();
      auto counts = histogram.dataY();
      TSM_ASSERT_EQUALS("Should have 21 bin edges.", binEdges.size(), 21);
      TSM_ASSERT_EQUALS("Should have 20 counts.", counts.size(), 20);

      auto expectedBinEdgeIt = expectedBinEdges.cbegin();
      for (auto it = binEdges.cbegin();
           it != binEdges.cend() ||
           expectedBinEdgeIt != expectedBinEdges.cend();
           ++it, ++expectedBinEdgeIt) {
        TS_ASSERT(std::abs(*it - *expectedBinEdgeIt) < tolerance);
      }

      auto expectedCountIt = expectedCounts.cbegin();
      for (auto it = counts.cbegin();
           it != counts.cend() || expectedCountIt != expectedCounts.cend();
           ++it, ++expectedCountIt) {
        TS_ASSERT(std::abs(*it - *expectedCountIt) < tolerance);
      }
    }

    // Check that the other elements are untouched. This means that each bin
    // should
    // have a value of 2 and there should be 10 of them. We only check index 0
    // here
    {
      Mantid::HistogramData::BinEdges expectedBinEdgesDetector{0, 1, 2, 3, 4, 5,
                                                               6, 7, 8, 9, 10};
      Mantid::HistogramData::Counts expectedCountsDetector{2, 2, 2, 2, 2,
                                                           2, 2, 2, 2, 2};

      auto histogramDetector = outputWorkspace->histogram(0);
      auto binEdgesDetector = histogramDetector.binEdges();
      auto countsDetector = histogramDetector.dataY();
      TSM_ASSERT_EQUALS("Should have 11 bin edges.", binEdgesDetector.size(),
                        11);
      TSM_ASSERT_EQUALS("Should have 10 counts.", countsDetector.size(), 10);

      auto expectedBinEdgeDetectorIt = expectedBinEdgesDetector.cbegin();
      for (auto it = binEdgesDetector.cbegin();
           it != binEdgesDetector.cend() ||
           expectedBinEdgeDetectorIt != expectedBinEdgesDetector.cend();
           ++it, ++expectedBinEdgeDetectorIt) {
        TS_ASSERT(std::abs(*it - *expectedBinEdgeDetectorIt) < tolerance);
      }

      auto expectedCountDetectorIt = expectedCountsDetector.cbegin();
      for (auto it = expectedCountsDetector.cbegin();
           it != expectedCountsDetector.cend() ||
           expectedCountDetectorIt != expectedCountsDetector.cend();
           ++it, ++expectedCountDetectorIt) {
        TS_ASSERT(std::abs(*it - *expectedCountDetectorIt) < tolerance);
      }
    }

    // Clean up
    if (ads.doesExist(outputName)) {
      ads.remove(outputName);
    }
  }

  void test_that_linear_workspace_selects_time_ranges_around_monitor() {
    // Arrange
    auto workspace = provideTestWorkspace(true);
    std::string outputName = "test_output_unwrap_monitors";
    const double wavelengthMin = 5.0;  // Angstrom
    const double wavelengthMax = 15.0; // Angstrom

    // Act
    UnwrapMonitorsInTOF alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", outputName);
    alg.setProperty("WavelengthMin", wavelengthMin);
    alg.setProperty("WavelengthMax", wavelengthMax);
    alg.execute();

    // Assert
    auto &ads = Mantid::API::AnalysisDataService::Instance();
    auto outputWorkspace =
        ads.retrieveWS<Mantid::API::MatrixWorkspace>(outputName);

    // Check monitor 3 which is position at 11 meters from the source
    // For a wavelength cutoff range in from 5 to 15 Angstrom we expect a TOF
    // range of: 13902mus - 41708mus
    // Every count outside of this range is expected to have been set to 0. Note
    // that the time is compared to Points no BinEdges
    const double tolerance = 1e-8;
    Mantid::HistogramData::BinEdges expectedBinEdges{
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    expectedBinEdges *= 1e4;
    Mantid::HistogramData::Counts expectedCounts3{0, 2, 2, 2, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    auto histogram3 = outputWorkspace->histogram(3);
    auto binEdges3 = histogram3.binEdges();
    auto counts3 = histogram3.counts();
    TSM_ASSERT_EQUALS("Should have 21 bin edges.", binEdges3.size(), 21);
    TSM_ASSERT_EQUALS("Should have 20 bin edges.", counts3.size(), 20);

    auto expectedCount3It = expectedCounts3.cbegin();
    for (auto it = counts3.cbegin();
         it != counts3.cend() || expectedCount3It != expectedCounts3.cend();
         ++it, ++expectedCount3It) {
      TS_ASSERT(std::abs(*it - *expectedCount3It) < tolerance);
    }

    // Check monitor 3 which is position at 18 meters from the source
    // For a wavelength cutoff range in from 5 to 15 Angstrom we expect a TOF
    // range of: 22750mus - 682500mus
    // Every count outside of this range is expected to have been set to 0. Note
    // that the time is compared to Points no BinEdges
    Mantid::HistogramData::Counts expectedCounts4{0, 0, 2, 2, 2, 1, 1, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    auto histogram4 = outputWorkspace->histogram(4);
    auto binEdges4 = histogram4.binEdges();
    auto counts4 = histogram4.counts();
    TSM_ASSERT_EQUALS("Should have 21 bin edges.", binEdges4.size(), 21);
    TSM_ASSERT_EQUALS("Should have 20 bin edges.", counts4.size(), 20);

    auto expectedCount4It = expectedCounts4.cbegin();
    for (auto it = counts4.cbegin();
         it != counts4.cend() || expectedCount4It != expectedCounts4.cend();
         ++it, ++expectedCount4It) {
      TS_ASSERT(std::abs(*it - *expectedCount4It) < tolerance);
    }

    // Clean up
    if (ads.doesExist(outputName)) {
      ads.remove(outputName);
    }
  }

  void test_that_negative_wavelengths_are_not_allowed() {
    // Arrange
    auto workspace = provideTestWorkspace(true);
    std::string outputName = "test_output_unwrap_monitors";
    const double wavelengthMin = -5.0;  // Angstrom
    const double wavelengthMax = -15.0; // Angstrom

    // Act
    UnwrapMonitorsInTOF alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", workspace);
    alg.setProperty("OutputWorkspace", outputName);
    alg.setProperty("WavelengthMin", wavelengthMin);
    alg.setProperty("WavelengthMax", wavelengthMax);
    TSM_ASSERT_THROWS("Negative wavelength cutoffs are not allowed.",
                      alg.execute(), const std::runtime_error &);
  }

private:
  /**
   * This produced a workspace with 5 histograms where the last two are
   *monitors.
   * The binning is set to 10. the monitors are
   * Value:   2  2  2  2  2  1  1  1  1  1
   * Time : 0  1  2  3  4  5  6  7  8  9  10  | * 10^4 (in microseconds)
   * The monitor at workspace index 4 is at 11m
   * The monitor are worksapce index 5 is at 18m
   *
   */
  Mantid::API::MatrixWorkspace_sptr provideTestWorkspace(bool includeMonitors) {
    const int numberOfBins = 10;
    const int numberOfHistograms = 5;
    auto intialWorkspace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            numberOfHistograms, numberOfBins, includeMonitors, false, true,
            "TestInstrument");
    auto workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        intialWorkspace);

    // Set the monitor bins to the expected values
    const auto &spectrumInfo = workspace->spectrumInfo();
    for (size_t index = 0; index < numberOfHistograms; ++index) {
      if (spectrumInfo.isMonitor(index)) {
        auto histogram = workspace->histogram(index);
        Mantid::HistogramData::Counts counts{2, 2, 2, 2, 2, 1, 1, 1, 1, 1};
        auto binEdges = histogram.binEdges();
        binEdges *= 1e4;
        Mantid::HistogramData::Histogram newHistogram(binEdges, counts);
        workspace->setHistogram(index, newHistogram);
      }
    }

    return workspace;
  }
};

#endif /* MANTID_ALGORITHMS_UNWRAPMONITORSINTOFTEST_H_ */
