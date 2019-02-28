// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GETEIMONDET2TEST_H_
#define GETEIMONDET2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/ExtractSingleSpectrum.h"
#include "MantidAlgorithms/GetEiMonDet2.h"
#include "MantidDataHandling/MaskDetectors.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::PhysicalConstants;
using namespace WorkspaceCreationHelper;

// Some rather random numbers here.
static constexpr double DETECTOR_DISTANCE = 1.78;
static constexpr double EI = 66.6; // meV
static constexpr double MONITOR_DISTANCE = 0.44;

class GetEiMonDet2Test : public CxxTest::TestSuite {
public:
  static GetEiMonDet2Test *createSuite() { return new GetEiMonDet2Test(); }
  static void destroySuite(GetEiMonDet2Test *suite) { delete suite; }
  static double velocity(const double energy) {
    return std::sqrt(2 * energy * meV / NeutronMass);
  }
  static constexpr double time_of_flight(const double velocity) {
    return (MONITOR_DISTANCE + DETECTOR_DISTANCE) / velocity * 1e6;
  }

  void testName() {
    GetEiMonDet2 algorithm;
    TS_ASSERT_EQUALS(algorithm.name(), "GetEiMonDet")
  }

  void testVersion() {
    GetEiMonDet2 algorithm;
    TS_ASSERT_EQUALS(algorithm.version(), 2)
  }

  void testInit() {
    GetEiMonDet2 algorithm;
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
  }

  void testSuccessOnMinimalInput() {
    const double realEi = 0.97 * EI;
    const auto peaks =
        peakCentres(100, realEi, std::numeric_limits<double>::max());
    std::vector<EPPTableRow> eppRows(peaks.size());
    for (size_t i = 0; i < peaks.size(); ++i) {
      eppRows[i].peakCentre = peaks[i];
    }
    auto eppTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    GetEiMonDet2 algorithm;
    setupSimple(ws, eppTable, algorithm);
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    TS_ASSERT_DELTA(
        static_cast<decltype(realEi)>(algorithm.getProperty("IncidentEnergy")),
        realEi, 1e-6)
  }

  void testSuccessOnComplexInput() {
    const double realEi = 1.18 * EI;
    const double pulseInterval = std::floor(time_of_flight(velocity(EI)) / 2);
    const double timeAtMonitor = 0.34 * pulseInterval;
    auto detectorPeakCentres =
        peakCentres(timeAtMonitor, realEi, pulseInterval);
    detectorPeakCentres.erase(detectorPeakCentres.begin());
    std::vector<EPPTableRow> eppRows(detectorPeakCentres.size());
    for (size_t i = 0; i < detectorPeakCentres.size(); ++i) {
      eppRows[i].peakCentre = detectorPeakCentres[i];
    }
    auto detectorEPPTable = createEPPTableWorkspace(eppRows);
    eppRows.clear();
    auto monitorPeakCentres = peakCentres(timeAtMonitor, realEi, pulseInterval);
    monitorPeakCentres.erase(monitorPeakCentres.begin() + 1,
                             monitorPeakCentres.end());
    eppRows.resize(monitorPeakCentres.size());
    for (size_t i = 0; i < monitorPeakCentres.size(); ++i) {
      eppRows[i].peakCentre = monitorPeakCentres[i];
    }
    auto monitorEPPTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    ws->mutableRun().removeProperty("Ei");
    // Break workspace into separate monitor and detector workspaces.
    const std::string extractedWsName(
        "GetEiMonDet2Test_testSuccessOnComplexInput_extracted");
    ExtractSingleSpectrum spectrumExtraction;
    spectrumExtraction.initialize();
    spectrumExtraction.setChild(true);
    spectrumExtraction.setProperty("InputWorkspace", ws);
    spectrumExtraction.setProperty("WorkspaceIndex", 0);
    spectrumExtraction.setProperty("OutputWorkspace", extractedWsName);
    spectrumExtraction.execute();
    MatrixWorkspace_sptr monitorWs =
        spectrumExtraction.getProperty("OutputWorkspace");
    spectrumExtraction.setProperty("InputWorkspace", ws);
    spectrumExtraction.setProperty("WorkspaceIndex", 1);
    spectrumExtraction.setProperty("OutputWorkspace", extractedWsName);
    spectrumExtraction.execute();
    MatrixWorkspace_sptr detectorWs =
        spectrumExtraction.getProperty("OutputWorkspace");
    GetEiMonDet2 algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorWorkspace", detectorWs));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorEPPTable", detectorEPPTable))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("IndexType", "Spectrum Number"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Detectors", "2"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("NominalIncidentEnergy", EI))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("MonitorWorkspace", monitorWs))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("MonitorEPPTable", monitorEPPTable))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Monitor", 1))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("PulseInterval", pulseInterval))
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    TS_ASSERT_DELTA(
        static_cast<decltype(realEi)>(algorithm.getProperty("IncidentEnergy")),
        realEi, 1e-6)
  }

  void testSuccessOnPulseIntervalInProperties() {
    runPulseIntervalInputsTest(PulseIntervalInputs::AS_PROPERTY);
  }

  void testSuccessOnPulseIntervalInSampleLogs() {
    runPulseIntervalInputsTest(PulseIntervalInputs::AS_SAMLPE_LOG);
  }

  void testFailureOnAllDetectorsMasked() {
    const double realEi = EI;
    const auto peaks =
        peakCentres(100, realEi, std::numeric_limits<double>::max());
    std::vector<EPPTableRow> eppRows(peaks.size());
    for (size_t i = 0; i < peaks.size(); ++i) {
      eppRows[i].peakCentre = peaks[i];
    }
    auto eppTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    MaskDetectors maskDetectors;
    maskDetectors.initialize();
    maskDetectors.setChild(true);
    maskDetectors.setProperty("Workspace", ws);
    maskDetectors.setProperty("WorkspaceIndexList", "1");
    maskDetectors.execute();
    GetEiMonDet2 algorithm;
    setupSimple(ws, eppTable, algorithm);
    TS_ASSERT_THROWS(algorithm.execute(), std::runtime_error)
    TS_ASSERT(!algorithm.isExecuted())
  }

  void testFailureOnMonitorMasked() {
    const double realEi = EI;
    const auto peaks =
        peakCentres(100, realEi, std::numeric_limits<double>::max());
    std::vector<EPPTableRow> eppRows(peaks.size());
    for (size_t i = 0; i < peaks.size(); ++i) {
      eppRows[i].peakCentre = peaks[i];
    }
    auto eppTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    MaskDetectors maskDetectors;
    maskDetectors.initialize();
    maskDetectors.setChild(true);
    maskDetectors.setProperty("Workspace", ws);
    maskDetectors.setProperty("WorkspaceIndexList", "0");
    maskDetectors.execute();
    GetEiMonDet2 algorithm;
    setupSimple(ws, eppTable, algorithm);
    TS_ASSERT_THROWS(algorithm.execute(), std::runtime_error)
    TS_ASSERT(!algorithm.isExecuted())
  }

  void testFailureOnEPPUnsuccessfulOnAllDetectors() {
    const double realEi = EI;
    const auto peaks =
        peakCentres(100, realEi, std::numeric_limits<double>::max());
    std::vector<EPPTableRow> eppRows(peaks.size());
    for (size_t i = 0; i < peaks.size(); ++i) {
      eppRows[i].peakCentre = peaks[i];
      eppRows[i].fitStatus = EPPTableRow::FitStatus::FAILURE;
    }
    // Monitor should still say 'success'.
    eppRows.front().fitStatus = EPPTableRow::FitStatus::SUCCESS;
    auto eppTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    GetEiMonDet2 algorithm;
    setupSimple(ws, eppTable, algorithm);
    TS_ASSERT_THROWS(algorithm.execute(), std::runtime_error)
    TS_ASSERT(!algorithm.isExecuted())
  }

  void testFailureOnEPPUnsuccessfulOnMonitor() {
    const double realEi = EI;
    const auto peaks =
        peakCentres(100, realEi, std::numeric_limits<double>::max());
    std::vector<EPPTableRow> eppRows(peaks.size());
    for (size_t i = 0; i < peaks.size(); ++i) {
      eppRows[i].peakCentre = peaks[i];
    }
    eppRows.front().fitStatus = EPPTableRow::FitStatus::FAILURE;
    auto eppTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    GetEiMonDet2 algorithm;
    setupSimple(ws, eppTable, algorithm);
    TS_ASSERT_THROWS(algorithm.execute(), std::runtime_error)
    TS_ASSERT(!algorithm.isExecuted())
  }

  void testFailureOnMonitorDetectorIndexClash() {
    const double realEi = EI;
    const auto peaks =
        peakCentres(100, realEi, std::numeric_limits<double>::max());
    std::vector<EPPTableRow> eppRows(peaks.size());
    for (size_t i = 0; i < peaks.size(); ++i) {
      eppRows[i].peakCentre = peaks[i];
    }
    auto eppTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    GetEiMonDet2 algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("DetectorWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorEPPTable", eppTable))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Detectors", "1"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setPropertyValue("Monitor", "1"))
    TS_ASSERT_THROWS(algorithm.execute(), std::runtime_error)
    TS_ASSERT(!algorithm.isExecuted())
  }

  void testFailureOnNegativeMonitorWorkspaceIndex() {
    const double realEi = EI;
    const auto peaks =
        peakCentres(100, realEi, std::numeric_limits<double>::max());
    std::vector<EPPTableRow> eppRows(peaks.size());
    for (size_t i = 0; i < peaks.size(); ++i) {
      eppRows[i].peakCentre = peaks[i];
    }
    auto eppTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    GetEiMonDet2 algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("DetectorWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorEPPTable", eppTable))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("IndexType", "Workspace Index"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Detectors", "1"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setPropertyValue("Monitor", "-1"))
    const std::string exceptionMessage("Monitor cannot be negative.");
    TS_ASSERT_THROWS_EQUALS(algorithm.execute(), const std::runtime_error &e,
                            e.what(), exceptionMessage)
    TS_ASSERT(!algorithm.isExecuted())
  }

  void testFailureOnNonexistentDetectorIndex() {
    const double realEi = EI;
    const auto peaks =
        peakCentres(100, realEi, std::numeric_limits<double>::max());
    std::vector<EPPTableRow> eppRows(peaks.size());
    for (size_t i = 0; i < peaks.size(); ++i) {
      eppRows[i].peakCentre = peaks[i];
    }
    auto eppTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    GetEiMonDet2 algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("DetectorWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorEPPTable", eppTable))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Detectors", "42"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setPropertyValue("Monitor", "0"))
    TS_ASSERT_THROWS(algorithm.execute(), std::runtime_error)
    TS_ASSERT(!algorithm.isExecuted())
  }

  void testFailureOnNonexistentMonitorIndex() {
    const double realEi = EI;
    const auto peaks =
        peakCentres(100, realEi, std::numeric_limits<double>::max());
    std::vector<EPPTableRow> eppRows(peaks.size());
    for (size_t i = 0; i < peaks.size(); ++i) {
      eppRows[i].peakCentre = peaks[i];
    }
    auto eppTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    GetEiMonDet2 algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("DetectorWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorEPPTable", eppTable))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Detectors", "1"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setPropertyValue("Monitor", "42"))
    TS_ASSERT_THROWS(algorithm.execute(), std::runtime_error)
    TS_ASSERT(!algorithm.isExecuted())
  }

  void testFailureOnPulseIntervalMissing() {
    runPulseIntervalInputsTest(PulseIntervalInputs::NONE);
  }

private:
  static void attachInstrument(MatrixWorkspace_sptr targetWs) {
    // The reference frame used by createInstrumentForWorkspaceWithDistances
    // is left handed with y pointing up, x along beam (2016-08-12).

    const V3D sampleR(0, 0, 0);
    // Source can be positioned arbitrarily.
    const V3D sourceR(-2 * MONITOR_DISTANCE, 0, 0);
    std::vector<V3D> detectorRs;
    // Add monitor as the first detector --- it won't be marked as monitor,
    // but here it matters not.
    detectorRs.emplace_back(-MONITOR_DISTANCE, 0, 0);
    // Add more detectors --- these should be treated as the real ones.
    detectorRs.emplace_back(0, DETECTOR_DISTANCE, 0);
    createInstrumentForWorkspaceWithDistances(targetWs, sampleR, sourceR,
                                              detectorRs);
  }

  static MatrixWorkspace_sptr createWorkspace() {
    const size_t nDetectors = 1;
    // Number of spectra = detectors + monitor.
    auto ws = create2DWorkspace(nDetectors + 1, 2);
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    attachInstrument(ws);
    ws->mutableRun().addProperty("Ei", EI, true);
    return ws;
  }

  static std::vector<double> peakCentres(double timeAtMonitor, double energy,
                                         double pulseInterval) {
    std::vector<double> centres;
    centres.emplace_back(timeAtMonitor);
    double timeOfFlight = timeAtMonitor + time_of_flight(velocity(energy));
    while (timeOfFlight > pulseInterval) {
      timeOfFlight -= pulseInterval;
    }
    centres.emplace_back(timeOfFlight);
    return centres;
  }

  // Mininum setup for GetEiMonDet2.
  void setupSimple(MatrixWorkspace_sptr ws, ITableWorkspace_sptr eppTable,
                   GetEiMonDet2 &algorithm) {
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("DetectorWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorEPPTable", eppTable))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Detectors", "1"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setPropertyValue("Monitor", "0"))
  }

  enum class PulseIntervalInputs { AS_PROPERTY, AS_SAMLPE_LOG, NONE };

  void
  runPulseIntervalInputsTest(const PulseIntervalInputs pulseIntervalInput) {
    const double realEi = 1.18 * EI;
    const double pulseInterval = std::floor(time_of_flight(velocity(EI)) / 2);
    const double timeAtMonitor = 0.34 * pulseInterval;
    auto peaks = peakCentres(timeAtMonitor, realEi, pulseInterval);
    std::vector<EPPTableRow> eppRows(peaks.size());
    for (size_t i = 0; i != peaks.size(); ++i) {
      eppRows[i] =
          EPPTableRow(peaks[i], 1.0, 1.0, EPPTableRow::FitStatus::SUCCESS);
    }
    auto eppTable = createEPPTableWorkspace(eppRows);
    auto ws = createWorkspace();
    if (pulseIntervalInput == PulseIntervalInputs::AS_SAMLPE_LOG) {
      ws->mutableRun().addProperty("pulse_interval", pulseInterval * 1e-6);
    }
    GetEiMonDet2 algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("DetectorWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorEPPTable", eppTable))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("IndexType", "Spectrum Number"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Detectors", "2"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("Monitor", 1))
    if (pulseIntervalInput == PulseIntervalInputs::AS_PROPERTY) {
      TS_ASSERT_THROWS_NOTHING(
          algorithm.setProperty("PulseInterval", pulseInterval));
    }
    if (pulseIntervalInput == PulseIntervalInputs::NONE) {
      TS_ASSERT_THROWS(algorithm.execute(), std::runtime_error)
      TS_ASSERT(!algorithm.isExecuted())
    } else {
      TS_ASSERT_THROWS_NOTHING(algorithm.execute())
      TS_ASSERT(algorithm.isExecuted())
      TS_ASSERT_DELTA(static_cast<decltype(realEi)>(
                          algorithm.getProperty("IncidentEnergy")),
                      realEi, 1e-6)
    }
  }
};

#endif // GETEIMONDET2TEST_H_
