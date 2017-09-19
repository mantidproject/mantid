#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadILLReflectometry.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLReflectometry;
using Mantid::DataHandling::LoadEmptyInstrument;

class LoadILLReflectometryTest : public CxxTest::TestSuite {
private:
  const std::string m_d17DirectBeamFile{"ILL/D17/317369.nxs"};
  const std::string m_d17File{"ILL/D17/317370.nxs"};
  const std::string m_figaroFile{"ILL/Figaro/598488.nxs"};
  // Name of the default output workspace
  const std::string m_outWSName{"LoadILLReflectometryTest_OutputWS"};

  void commonProperties(MatrixWorkspace_sptr output,
                        const std::string &instrName) {
    TS_ASSERT(output->isHistogramData());
    const auto &spectrumInfo = output->spectrumInfo();
    const auto spectrumInfoSize = spectrumInfo.size();
    TS_ASSERT(spectrumInfo.isMonitor(spectrumInfoSize - 1));
    TS_ASSERT(spectrumInfo.isMonitor(spectrumInfoSize - 2));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 256 + 2);
    TS_ASSERT_EQUALS(output->blocksize(), 1000);
    TS_ASSERT_EQUALS(output->run().getProperty("Facility")->value(), "ILL");
    TS_ASSERT_EQUALS(output->getInstrument()->getName(), instrName);
    // check the sum of all detector counts against Nexus file entry detsum
    TS_ASSERT_EQUALS(output->run().getPropertyValueAsType<double>("PSD.detsum"),
                     detCounts(output));
  }

  double detCounts(MatrixWorkspace_sptr output) {
    // sum of detector counts
    double counts{0.0};
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      if (output->spectrumInfo().isMonitor(i)) {
        continue;
      }
      auto &values = output->y(i);
      counts = std::accumulate(values.begin(), values.end(), counts);
    }
    return counts;
  }

  void getWorkspaceFor(MatrixWorkspace_sptr &output, const std::string fileName,
                       const std::string outFile, std::string property = "",
                       std::string value = "") {
    bool success = loadSpecific(fileName, outFile, property, value);
    if (success) {
      output =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outFile);
      TS_ASSERT(output);
    }
  }

  bool loadSpecific(const std::string fileName, const std::string outFile,
                    std::string property = "", std::string value = "") {
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", fileName));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outFile));
    if (property != "" && value != "") {
      loader.setPropertyValue(property, value);
    }
    TS_ASSERT_THROWS_NOTHING(loader.execute(););
    TS_ASSERT(loader.isExecuted());
    return loader.isExecuted();
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLReflectometryTest *createSuite() {
    return new LoadILLReflectometryTest();
  }
  static void destroySuite(LoadILLReflectometryTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testName() {
    LoadILLReflectometry loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    TS_ASSERT_EQUALS(loader.name(), "LoadILLReflectometry");
  }

  void testVersion() {
    LoadILLReflectometry loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT_EQUALS(loader.version(), 1);
  }

  void testExecD17() { loadSpecific(m_d17File, m_outWSName); }

  // D17

  void testTOFD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, m_outWSName, "XUnit", "TimeOfFlight");
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "TOF");
    const auto &run = output->run();
    const auto channelWidth =
        run.getPropertyValueAsType<double>("PSD.time_of_flight_0");
    const auto channelCount = static_cast<size_t>(
        run.getPropertyValueAsType<double>("PSD.time_of_flight_1"));
    const auto tofDelay =
        run.getPropertyValueAsType<double>("PSD.time_of_flight_2");
    const auto chopper1Speed = run.getPropertyValueAsType<double>(
        "VirtualChopper.chopper1_speed_average");
    const auto chopper1Phase =
        run.getPropertyValueAsType<double>("Chopper1.phase");
    const auto chopper2Phase = run.getPropertyValueAsType<double>(
        "VirtualChopper.chopper2_phase_average");
    const auto pOffset =
        run.getPropertyValueAsType<double>("VirtualChopper.poff");
    const auto openOffset =
        run.getPropertyValueAsType<double>("VirtualChopper.open_offset");
    const auto tof0 =
        tofDelay + 0.5 * channelWidth -
        60e6 * (pOffset - 45 + chopper2Phase - chopper1Phase + openOffset) /
            (2 * 360 * chopper1Speed);
    TS_ASSERT_EQUALS(output->blocksize(), channelCount);
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < 1; ++j) {
        const auto tof = tof0 + static_cast<double>(j) * channelWidth;
        TS_ASSERT_DELTA(output->x(i)[j], tof, 1e-12)
      }
    }
  }

  void testSourcePositionD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, m_outWSName, "XUnit", "TimeOfFlight");
    const auto &run = output->run();
    const auto chopperCentre =
        run.getPropertyValueAsType<double>("VirtualChopper.dist_chop_samp");
    const auto chopperSeparation =
        run.getPropertyValueAsType<double>("Distance.ChopperGap") / 100;
    const auto sourceSample = chopperCentre - 0.5 * chopperSeparation;
    const auto &spectrumInfo = output->spectrumInfo();
    const auto l1 = spectrumInfo.l1();
    TS_ASSERT_DELTA(sourceSample, l1, 1e-12)
    const auto samplePos = spectrumInfo.samplePosition();
    const auto sourcePos = spectrumInfo.sourcePosition();
    for (size_t i = 0; i < 3; ++i) {
      TS_ASSERT_EQUALS(samplePos[i], 0)
    }
    TS_ASSERT_EQUALS(sourcePos.X(), 0)
    TS_ASSERT_EQUALS(sourcePos.Y(), 0)
    TS_ASSERT_EQUALS(sourcePos.Z(), -sourceSample)
  }

  void testDetectorPositionAndRotationD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, m_outWSName);
    const auto &spectrumInfo = output->spectrumInfo();
    const auto &run = output->run();
    const auto detDist = run.getPropertyValueAsType<double>("det.value") / 1000;
    const auto pixWidth = run.getPropertyValueAsType<double>("PSD.mppx") / 1000;
    const auto detAngle =
        run.getPropertyValueAsType<double>("dan.value") * M_PI / 180;
    for (size_t i = 0; i < spectrumInfo.size(); ++i) {
      if (spectrumInfo.isMonitor(i)) {
        continue;
      }
      const auto p = spectrumInfo.position(i);
      TS_ASSERT_EQUALS(p.Y(), 0)
      const auto pixOffset = (127.5 - static_cast<double>(i)) * pixWidth;
      const auto pixAngle = detAngle + std::atan2(pixOffset, detDist);
      const auto pixDist = std::sqrt(pixOffset * pixOffset + detDist * detDist);
      const auto idealX = pixDist * std::sin(pixAngle);
      const auto idealZ = pixDist * std::cos(pixAngle);
      TS_ASSERT_DELTA(p.X(), idealX, 1e-8)
      TS_ASSERT_DELTA(p.Z(), idealZ, 1e-8)
    }
  }

  void test2ThetaD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, m_outWSName);
    // Compare angles in rad
    const auto &spectrumInfo = output->spectrumInfo();
    // Check twoTheta between two center detectors.
    const auto dan = output->run().getPropertyValueAsType<double>("dan.value");
    TS_ASSERT_LESS_THAN_EQUALS(spectrumInfo.twoTheta(128) * 180 / M_PI, dan)
    TS_ASSERT_LESS_THAN_EQUALS(dan, spectrumInfo.twoTheta(127) * 180 / M_PI)
    const auto stheta = output->run().getPropertyValueAsType<double>("stheta");
    TS_ASSERT_EQUALS(2 * stheta * 180 / M_PI, dan)
  }

  void testUserAngleD17() {
    MatrixWorkspace_sptr output;
    const double angle = 23.23;
    getWorkspaceFor(output, m_d17File, m_outWSName, "BraggAngle",
                    std::to_string(angle));
    const auto &spectrumInfo = output->spectrumInfo();
    TS_ASSERT_LESS_THAN_EQUALS(spectrumInfo.twoTheta(128) * 180 / M_PI, angle)
    TS_ASSERT_LESS_THAN_EQUALS(angle, spectrumInfo.twoTheta(127) * 180 / M_PI)
    const auto stheta = output->run().getPropertyValueAsType<double>("stheta");
    TS_ASSERT_EQUALS(2 * stheta * 180 / M_PI, angle)
  }

  void testPropertiesD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, m_outWSName);
    commonProperties(output, "D17");
    const auto &spectrumInfo = output->spectrumInfo();
    const auto detAngle =
        (spectrumInfo.twoTheta(127) + spectrumInfo.twoTheta(128)) / 2;
    TS_ASSERT_DELTA(2 * output->run().getPropertyValueAsType<double>("stheta"),
                    detAngle, 1e-10)
  }

  void testDirectBeamOutput() {
    using namespace Mantid::DataObjects;
    MatrixWorkspace_sptr output;
    const std::string beamPosWSName{"LoadILLReflectometryTest_BeapPositionWS"};
    getWorkspaceFor(output, m_d17DirectBeamFile, m_outWSName,
                    "OutputBeamPosition", beamPosWSName);
    TableWorkspace_sptr beamPosWS =
        AnalysisDataService::Instance().retrieveWS<TableWorkspace>(
            beamPosWSName);
    TS_ASSERT(beamPosWS)
    TS_ASSERT_EQUALS(beamPosWS->rowCount(), 1)
    TS_ASSERT_EQUALS(beamPosWS->columnCount(), 4)
    const auto colNames = beamPosWS->getColumnNames();
    TS_ASSERT_EQUALS(
        std::count(colNames.cbegin(), colNames.cend(), "DetectorAngle"), 1)
    const auto &detAngles = beamPosWS->getColVector<double>("DetectorAngle");
    const auto &run = output->run();
    const auto dan = run.getPropertyValueAsType<double>("dan.value");
    TS_ASSERT_EQUALS(detAngles.front(), dan)
    TS_ASSERT_EQUALS(
        std::count(colNames.cbegin(), colNames.cend(), "DetectorDistance"), 1)
    const auto &detDistances =
        beamPosWS->getColVector<double>("DetectorDistance");
    const auto detDist = run.getPropertyValueAsType<double>("det.value") / 1000;
    TS_ASSERT_EQUALS(detDistances.front(), detDist)
    TS_ASSERT_EQUALS(
        std::count(colNames.cbegin(), colNames.cend(), "PositionOfMaximum"), 1)
    const auto maxPositions =
        beamPosWS->getColVector<double>("PositionOfMaximum");
    TS_ASSERT_EQUALS(maxPositions.front(), 202)
    TS_ASSERT_EQUALS(
        std::count(colNames.cbegin(), colNames.cend(), "FittedPeakCentre"), 1)
    const auto peakCentres =
        beamPosWS->getColVector<double>("FittedPeakCentre");
    TS_ASSERT_DELTA(peakCentres.front(), maxPositions.front(), 0.5)
  }

  void testDirectBeamInput() {
    using namespace Mantid::DataObjects;
    MatrixWorkspace_sptr dbOutput;
    const std::string dbBeamPosWSName{
        "LoadILLReflectometryTest_DbBeamPositionWS"};
    getWorkspaceFor(dbOutput, m_d17DirectBeamFile,
                    "LoadILLReflectometryTest_DirectBeamWS",
                    "OutputBeamPosition", dbBeamPosWSName);
    TableWorkspace_sptr dbBeamPosWS =
        AnalysisDataService::Instance().retrieveWS<TableWorkspace>(
            dbBeamPosWSName);
    MatrixWorkspace_sptr refOutput;
    // Due to limitation of getWorkspaceFor, we run it twice for the reflected
    // beam.
    const std::string refBeamPosWSName{
        "LoadILLReflectometryTest_RefBeamPositionWS"};
    getWorkspaceFor(refOutput, m_d17File, m_outWSName, "OutputBeamPosition",
                    refBeamPosWSName);
    TableWorkspace_sptr refBeamPosWS =
        AnalysisDataService::Instance().retrieveWS<TableWorkspace>(
            refBeamPosWSName);
    getWorkspaceFor(refOutput, m_d17File, m_outWSName, "BeamPosition",
                    dbBeamPosWSName);
    const auto dbDetAngle = dbBeamPosWS->cell_cast<double>(0, "DetectorAngle");
    const auto dbDetDist =
        dbBeamPosWS->cell_cast<double>(0, "DetectorDistance");
    const auto dbPeakPos =
        dbBeamPosWS->cell_cast<double>(0, "FittedPeakCentre");
    const auto dbPixWidth =
        dbOutput->run().getPropertyValueAsType<double>("PSD.mppx") / 1000;
    const auto dbPeakOffset = (127.5 - dbPeakPos) * dbPixWidth;
    const auto dbOffsetAngle = std::atan2(dbPeakOffset, dbDetDist) * 180 / M_PI;
    const auto refDetAngle =
        refOutput->run().getPropertyValueAsType<double>("dan.value");
    const auto refDetDist =
        refOutput->run().getPropertyValueAsType<double>("det.value") / 1000;
    const auto refPeakPos =
        refBeamPosWS->cell_cast<double>(0, "FittedPeakCentre");
    const auto refPixWidth =
        refOutput->run().getPropertyValueAsType<double>("PSD.mppx") / 1000;
    const auto refPeakOffset = (127.5 - refPeakPos) * refPixWidth;
    const auto refOffsetAngle =
        std::atan2(refPeakOffset, refDetDist) * 180 / M_PI;
    const auto newDetAngle =
        refDetAngle - dbDetAngle - 2 * dbOffsetAngle + refOffsetAngle;
    const auto &spectrumInfo = refOutput->spectrumInfo();
    TS_ASSERT_LESS_THAN_EQUALS(spectrumInfo.twoTheta(128) * 180 / M_PI,
                               newDetAngle)
    TS_ASSERT_LESS_THAN_EQUALS(newDetAngle,
                               spectrumInfo.twoTheta(127) * 180 / M_PI)
  }
};

class LoadILLReflectometryTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    for (int i = 0; i < numberOfIterations; ++i) {
      loadAlgPtrs.emplace_back(setupAlg());
    }
  }

  void testLoadILLReflectometryPerformance() {
    for (auto alg : loadAlgPtrs) {
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

  void tearDown() override {
    for (int i = 0; i < numberOfIterations; i++) {
      delete loadAlgPtrs[i];
      loadAlgPtrs[i] = nullptr;
    }
    Mantid::API::AnalysisDataService::Instance().remove(m_outWSName);
  }

private:
  std::vector<LoadILLReflectometry *> loadAlgPtrs;

  const int numberOfIterations = 5;

  const std::string inFileName = "ILL/D17/317370.nxs";
  const std::string m_outWSName = "LoadILLReflectomeryWsOut";

  LoadILLReflectometry *setupAlg() {
    LoadILLReflectometry *loader = new LoadILLReflectometry;
    loader->initialize();
    loader->isInitialized();
    loader->setPropertyValue("Filename", inFileName);
    loader->setPropertyValue("OutputWorkspace", m_outWSName);

    loader->setRethrows(true);
    return loader;
  }
};

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRYTEST_H_ */
