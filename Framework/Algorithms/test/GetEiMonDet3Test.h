// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GETEIMONDET3TEST_H_
#define GETEIMONDET3TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/ExtractSpectra2.h"
#include "MantidAlgorithms/GetEiMonDet3.h"
#include "MantidDataHandling/MaskDetectors.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PhysicalConstants;
using namespace WorkspaceCreationHelper;

// Some rather random numbers here.
static constexpr double DETECTOR_DISTANCE = 1.78;
static constexpr double EI = 66.6; // meV
static constexpr double MONITOR_DISTANCE = 0.44;
static constexpr double TOF_START = 200.;
static constexpr double TOF_END = 2300.;
static constexpr double TOF_WIDTH = TOF_END - TOF_START;

class GetEiMonDet3Test : public CxxTest::TestSuite {
public:
  static GetEiMonDet3Test *createSuite() { return new GetEiMonDet3Test(); }
  static void destroySuite(GetEiMonDet3Test *suite) { delete suite; }
  static double velocity(const double energy) {
    return std::sqrt(2 * energy * meV / NeutronMass);
  }
  static constexpr double time_of_flight(const double velocity) {
    return (MONITOR_DISTANCE + DETECTOR_DISTANCE) / velocity * 1e6;
  }

  GetEiMonDet3Test() : CxxTest::TestSuite() { FrameworkManager::Instance(); }

  void testName() {
    GetEiMonDet3 algorithm;
    TS_ASSERT_EQUALS(algorithm.name(), "GetEiMonDet")
  }

  void testVersion() {
    GetEiMonDet3 algorithm;
    TS_ASSERT_EQUALS(algorithm.version(), 3)
  }

  void testInit() {
    GetEiMonDet3 algorithm;
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
  }

  void testSuccessOnMinimalInput() {
    constexpr auto realEi = 0.97 * EI;
    const auto peaks = peakCentres(100., realEi);
    auto ws = createWorkspace(peaks);
    GetEiMonDet3 algorithm;
    setupSimple(ws, algorithm);
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    TS_ASSERT_DELTA(
        static_cast<double>(algorithm.getProperty("IncidentEnergy")), realEi,
        1e-6)
  }

  void testSuccessWithPulseInterval() {
    constexpr auto pulseInterval = 1.05 * TOF_WIDTH;
    constexpr auto realEi = 0.83 * EI;
    const auto peaks = peakCentres(0.8 * TOF_WIDTH, realEi, pulseInterval);
    auto ws = createWorkspace(peaks);
    GetEiMonDet3 algorithm;
    setupSimple(ws, algorithm);
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("PulseInterval", pulseInterval))
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    TS_ASSERT_DELTA(
        static_cast<double>(algorithm.getProperty("IncidentEnergy")), realEi,
        1e-6)
  }

  void testPulseIntervalInSampleLogs() {
    constexpr auto pulseInterval = 1.05 * TOF_WIDTH;
    constexpr auto realEi = 1.12 * EI;
    const auto peaks = peakCentres(0.9 * TOF_WIDTH, realEi, pulseInterval);
    auto ws = createWorkspace(peaks);
    ws->mutableRun().addProperty("pulse_interval", pulseInterval * 1e-6);
    GetEiMonDet3 algorithm;
    setupSimple(ws, algorithm);
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    TS_ASSERT_DELTA(
        static_cast<double>(algorithm.getProperty("IncidentEnergy")), realEi,
        1e-6)
  }

  void testMonitorWorkspace() {
    constexpr auto realEi = 0.89 * EI;
    const auto peaks = peakCentres(230., realEi);
    auto fullWs = createWorkspace(peaks);
    MatrixWorkspace_sptr monWs;
    MatrixWorkspace_sptr detWs;
    std::tie(monWs, detWs) = splitMonitorAndDetectors(fullWs);
    GetEiMonDet3 algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("DetectorWorkspace", detWs))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorWorkspaceIndexSet", "0"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("MonitorIndex", 0))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("MonitorWorkspace", monWs))
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    TS_ASSERT_DELTA(
        static_cast<double>(algorithm.getProperty("IncidentEnergy")), realEi,
        1e-6)
  }

  void testMonitorWorkspaceWithEPPTable() {
    constexpr auto realEi = 0.89 * EI;
    constexpr auto monitorTimeOfFlight = 230.;
    const auto peaks = peakCentres(monitorTimeOfFlight, realEi);
    auto fullWs = createWorkspace(peaks);
    MatrixWorkspace_sptr monWs;
    MatrixWorkspace_sptr detWs;
    std::tie(monWs, detWs) = splitMonitorAndDetectors(fullWs);
    monWs->mutableY(0) = 0.; // Make sure we cannot fit Gaussian here.
    std::vector<EPPTableRow> monEppRow(1);
    monEppRow.front().peakCentre = monitorTimeOfFlight + TOF_START;
    auto monitorEPPWs = createEPPTableWorkspace(monEppRow);
    GetEiMonDet3 algorithm;
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("DetectorWorkspace", detWs))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorWorkspaceIndexSet", "0"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("MonitorIndex", 0))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("MonitorWorkspace", monWs))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("MonitorEPPTable", monitorEPPWs))
    TS_ASSERT_THROWS_NOTHING(algorithm.execute())
    TS_ASSERT(algorithm.isExecuted())
    TS_ASSERT_DELTA(
        static_cast<double>(algorithm.getProperty("IncidentEnergy")), realEi,
        1e-6)
  }

private:
  static void attachInstrument(MatrixWorkspace_sptr targetWs) {
    // The reference frame used by createInstrumentForWorkspaceWithDistances
    // is left handed with y pointing up, x along beam.

    const V3D sampleR(0., 0., 0.);
    // Source can be positioned arbitrarily.
    const V3D sourceR(-2. * MONITOR_DISTANCE, 0., 0.);
    std::vector<V3D> detectorRs;
    // Add monitor as the first detector --- it won't be marked as monitor,
    // but here it matters not.
    detectorRs.emplace_back(-MONITOR_DISTANCE, 0., 0.);
    // Add more detectors --- these should be treated as the real ones.
    detectorRs.emplace_back(0., 0., DETECTOR_DISTANCE);
    createInstrumentForWorkspaceWithDistances(targetWs, sampleR, sourceR,
                                              detectorRs);
  }

  static MatrixWorkspace_sptr
  createWorkspace(const std::vector<double> &peakPositions) {
    constexpr size_t nDetectors{1};
    constexpr size_t nBins{512};
    constexpr double X0{TOF_START};
    constexpr double dX{TOF_WIDTH / nBins};
    // Number of spectra = detectors + monitor.
    MatrixWorkspace_sptr ws =
        create2DWorkspaceBinned(nDetectors + 1, nBins, X0, dX);
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    attachInstrument(ws);
    for (size_t i = 0; i < peakPositions.size(); ++i) {
      const auto pos = peakPositions[i];
      const auto &Xs = ws->x(i);
      if (pos <= Xs.front() || pos >= Xs.back()) {
        throw std::runtime_error("Peak position not within the TOF range.");
      }
      auto &Ys = ws->mutableY(i);
      for (size_t binIndex = 0; binIndex < Ys.size(); ++binIndex) {
        const auto x = (Xs[binIndex + 1] + Xs[binIndex]) / 2.;
        const auto xponent = (x - pos) / (4. * dX);
        const double I = 1000. * std::exp(-xponent * xponent);
        Ys[binIndex] = I;
      }
    }
    ws->mutableRun().addProperty("Ei", EI, true);
    return ws;
  }

  static std::vector<double> peakCentres(const double timeAtMonitor,
                                         const double energy,
                                         const double pulseInterval = 0.) {
    std::vector<double> centres;
    centres.emplace_back(timeAtMonitor + TOF_START);
    const double timeOfFlight =
        timeAtMonitor + time_of_flight(velocity(energy)) - pulseInterval;
    centres.emplace_back(timeOfFlight + TOF_START);
    return centres;
  }

  // Mininum setup for GetEiMonDet3.
  static void setupSimple(MatrixWorkspace_sptr ws, GetEiMonDet3 &algorithm) {
    algorithm.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("DetectorWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(
        algorithm.setProperty("DetectorWorkspaceIndexSet", "1"))
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("MonitorIndex", 0))
  }

  static std::pair<MatrixWorkspace_sptr, MatrixWorkspace_sptr>
  splitMonitorAndDetectors(MatrixWorkspace_sptr &ws) {
    ExtractSpectra2 extract;
    extract.initialize();
    extract.setChild(true);
    extract.setRethrows(true);
    extract.setProperty("InputWorkspace", ws);
    extract.setProperty("OutputWorkspace", "unused");
    extract.setProperty("InputWorkspaceIndexSet", "0");
    extract.execute();
    MatrixWorkspace_sptr monWs = extract.getProperty("OutputWorkspace");
    extract.setProperty("InputWorkspaceIndexSet", "1");
    extract.execute();
    MatrixWorkspace_sptr detWs = extract.getProperty("OutputWorkspace");
    return std::make_pair(monWs, detWs);
  }
};

#endif // GETEIMONDET3TEST_H_
