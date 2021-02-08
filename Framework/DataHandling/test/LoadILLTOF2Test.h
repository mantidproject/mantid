// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadILLTOF2.h"
#include "MantidTypes/Core/DateAndTimeHelpers.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLTOF2;

class LoadILLTOF2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLTOF2Test *createSuite() { return new LoadILLTOF2Test(); }
  static void destroySuite(LoadILLTOF2Test *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testName() {
    LoadILLTOF2 loader;
    TS_ASSERT_EQUALS(loader.name(), "LoadILLTOF")
  }

  void testVersion() {
    LoadILLTOF2 loader;
    TS_ASSERT_EQUALS(loader.version(), 2)
  }

  void testInit() {
    LoadILLTOF2 loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT(loader.isInitialized())
  }

  /*
   * This test only loads the Sample Data
   * The elastic peak is obtained on the fly from the sample data.
   */
  MatrixWorkspace_sptr
  loadDataFile(const std::string &dataFile, const size_t numberOfHistograms,
               const size_t numberOfMonitors, const size_t numberOfChannels,
               const double tofDelay, const double tofChannelWidth,
               const bool convertToTOF) {
    LoadILLTOF2 loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", dataFile))
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("convertToTOF", convertToTOF))

    std::string outputSpace = "LoadILLTOFTest_out";
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outputSpace))
    TS_ASSERT_THROWS_NOTHING(loader.execute())

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);

    TS_ASSERT(output->run().hasProperty("start_time"));
    TS_ASSERT(Mantid::Types::Core::DateAndTimeHelpers::stringIsISO8601(
        output->run().getProperty("start_time")->value()));

    TS_ASSERT_EQUALS(output->getNumberHistograms(), numberOfHistograms)
    const auto &spectrumInfo = output->spectrumInfo();
    for (size_t wsIndex = 0; wsIndex != output->getNumberHistograms();
         ++wsIndex) {
      if (wsIndex < numberOfHistograms - numberOfMonitors) {
        TS_ASSERT(!spectrumInfo.isMonitor(wsIndex))
      } else {
        TS_ASSERT(spectrumInfo.isMonitor(wsIndex))
      }
      const auto histogram = output->histogram(wsIndex);
      TS_ASSERT_EQUALS(histogram.xMode(),
                       Mantid::HistogramData::Histogram::XMode::BinEdges)
      TS_ASSERT_EQUALS(histogram.yMode(),
                       Mantid::HistogramData::Histogram::YMode::Counts)
      TS_ASSERT_EQUALS(histogram.size(), numberOfChannels)
      const auto &xs = histogram.x();
      if (convertToTOF) {
        for (size_t channelIndex = 0; channelIndex != xs.size();
             ++channelIndex) {
          const double binEdge =
              tofDelay + static_cast<double>(channelIndex) * tofChannelWidth -
              tofChannelWidth / 2;
          TS_ASSERT_DELTA(xs[channelIndex], binEdge, 1e-3)
        }
      } else {
        for (size_t channelIndex = 0; channelIndex != xs.size();
             ++channelIndex) {
          TS_ASSERT_EQUALS(xs[channelIndex], channelIndex)
        }
      }
      const auto &ys = histogram.y();
      const auto &es = histogram.e();
      for (size_t channelIndex = 0; channelIndex != es.size(); ++channelIndex) {
        TS_ASSERT_EQUALS(es[channelIndex], std::sqrt(ys[channelIndex]))
      }
    }

    // Check all detectors have a defined detector ID >= 0
    Mantid::detid2index_map detectorMap;
    TS_ASSERT_THROWS_NOTHING(detectorMap =
                                 output->getDetectorIDToWorkspaceIndexMap(true))

    // Check all detectors have a unique detector ID
    TS_ASSERT_EQUALS(detectorMap.size(), output->getNumberHistograms())

    for (const auto value : detectorMap) {
      TS_ASSERT(value.first >= 0)
    }

    return output;
  }

  void test_IN4_load() {
    // From the input test file.
    const double tofDelay = 238.34;
    const double tofChannelWidth = 5.85;
    const size_t channelCount = 512;
    const size_t histogramCount = 397;
    const size_t monitorCount = 1;
    const bool convertToTOF = true;
    MatrixWorkspace_sptr ws =
        loadDataFile("ILL/IN4/084446.nxs", histogramCount, monitorCount,
                     channelCount, tofDelay, tofChannelWidth, convertToTOF);

    const double pulseInterval =
        ws->run().getLogAsSingleValue("pulse_interval");
    TS_ASSERT_DELTA(0.003, pulseInterval, 1e-10)
  }

  void test_IN5_load() {
    // From the input test file.
    const double tofDelay = 5982.856;
    const double tofChannelWidth = 14.6349;
    const size_t channelCount = 512;
    const size_t histogramCount = 98305;
    const size_t monitorCount = 1;
    const bool convertToTOF = true;
    loadDataFile("ILL/IN5/104007.nxs", histogramCount, monitorCount,
                 channelCount, tofDelay, tofChannelWidth, convertToTOF);
  }

  void test_IN6_load() {
    // From the input test file.
    const double tofDelay = 430;
    const double tofChannelWidth = 5.8;
    const size_t channelCount = 1024;
    const size_t histogramCount = 340;
    const size_t monitorCount = 3;
    const bool convertToTOF = true;
    MatrixWorkspace_sptr ws =
        loadDataFile("ILL/IN6/164192.nxs", histogramCount, monitorCount,
                     channelCount, tofDelay, tofChannelWidth, convertToTOF);

    const double pulseInterval =
        ws->run().getLogAsSingleValue("pulse_interval");
    TS_ASSERT_DELTA(0.0060337892, pulseInterval, 1e-10)
  }

  void test_PANTHER_diffraction_load() {
    const double wavelength = 7.;
    const size_t histogram_count = 73729;
    const size_t monitor_count = 1;

    // mostly the same code as loadDataFile, but some of the TOF features cant
    // be called
    LoadILLTOF2 loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", "ILL/PANTHER/001036.nxs"))

    std::string outputSpace = "LoadILLTOFTest_out";
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outputSpace))
    TS_ASSERT_THROWS_NOTHING(loader.execute())

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);

    TS_ASSERT_EQUALS(output->getNumberHistograms(), histogram_count)
    const auto &spectrumInfo = output->spectrumInfo();
    for (size_t wsIndex = 0; wsIndex != output->getNumberHistograms();
         ++wsIndex) {
      if (wsIndex < histogram_count - monitor_count) {
        TS_ASSERT(!spectrumInfo.isMonitor(wsIndex))
      } else {
        TS_ASSERT(spectrumInfo.isMonitor(wsIndex))
      }
      const auto histogram = output->histogram(wsIndex);
      TS_ASSERT_EQUALS(histogram.xMode(),
                       Mantid::HistogramData::Histogram::XMode::BinEdges)
      TS_ASSERT_EQUALS(histogram.yMode(),
                       Mantid::HistogramData::Histogram::YMode::Counts)
      TS_ASSERT_EQUALS(histogram.size(), 1)

      const auto &xs = histogram.x();
      TS_ASSERT_DELTA(xs[0], 0.9 * wavelength, 1E-5)
      TS_ASSERT_DELTA(xs[1], 1.1 * wavelength, 1E-5)

      const auto &ys = histogram.y();
      const auto &es = histogram.e();
      TS_ASSERT_EQUALS(es[0], std::sqrt(ys[0]))
    }
    // Check all detectors have a defined detector ID >= 0
    Mantid::detid2index_map detectorMap;
    TS_ASSERT_THROWS_NOTHING(detectorMap =
                                 output->getDetectorIDToWorkspaceIndexMap(true))

    // Check all detectors have a unique detector ID
    TS_ASSERT_EQUALS(detectorMap.size(), output->getNumberHistograms())

    for (const auto value : detectorMap) {
      TS_ASSERT(value.first >= 0)
    }
  }

  void test_PANTHER_load() {
    // From the input test file.
    const double tofDelay = 350;
    const double tofChannelWidth = 4.88;
    const size_t channelCount = 512;
    const size_t histogramCount = 73729;
    const size_t monitorCount = 1;
    const bool convertToTOF = false;
    loadDataFile("ILL/PANTHER/001723.nxs", histogramCount, monitorCount,
                 channelCount, tofDelay, tofChannelWidth, convertToTOF);
  }

  void test_convertToTOF() {
    // From the input test file.
    const double tofDelay = 0;
    const double tofChannelWidth = 0; // should not be usesd
    const size_t channelCount = 512;
    const size_t histogramCount = 98305;
    const size_t monitorCount = 1;
    const bool convertToTOF = false;
    loadDataFile("ILL/IN5/104007.nxs", histogramCount, monitorCount,
                 channelCount, tofDelay, tofChannelWidth, convertToTOF);
  }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadILLTOF2TestPerformance : public CxxTest::TestSuite {
public:
  LoadILLTOF2TestPerformance() : m_dataFile("ILL/IN5/104007.nxs") {}

  void testDefaultLoad() {
    Mantid::DataHandling::LoadILLTOF2 loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize())
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_dataFile))
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("OutputWorkspace", "ws"))
    TS_ASSERT_THROWS_NOTHING(loader.execute())
    TS_ASSERT(loader.isExecuted())
  }

private:
  std::string m_dataFile;
};
