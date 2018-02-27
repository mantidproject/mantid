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

  static void commonProperties(MatrixWorkspace_sptr output,
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

  static double detCounts(MatrixWorkspace_sptr output) {
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

  static auto emptyProperties() {
    return std::vector<std::pair<std::string, std::string>>();
  }

  static void getWorkspaceFor(
      MatrixWorkspace_sptr &output, const std::string &fileName,
      const std::string &outFile,
      const std::vector<std::pair<std::string, std::string>> &properties) {
    bool success = loadSpecific(fileName, outFile, properties);
    if (success) {
      output =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outFile);
      TS_ASSERT(output);
    }
  }

  static bool loadSpecific(
      const std::string &fileName, const std::string &outFile,
      const std::vector<std::pair<std::string, std::string>> &properties) {
    LoadILLReflectometry loader;
    loader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", fileName));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outFile));
    for (const auto &p : properties) {
      loader.setPropertyValue(p.first, p.second);
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

  void testExecD17() {
    loadSpecific(m_d17File, m_outWSName, emptyProperties());
  }

  void testExecFigaro() {
    loadSpecific(m_figaroFile, m_outWSName, emptyProperties());
  }

  void testTOFD17() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("XUnit", "TimeOfFlight");
    getWorkspaceFor(output, m_d17File, m_outWSName, prop);
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
    const auto chopper1Phase = run.getPropertyValueAsType<double>(
        "VirtualChopper.chopper1_phase_average");
    const auto chopper2Phase = run.getPropertyValueAsType<double>(
        "VirtualChopper.chopper2_phase_average");
    const auto pOffset =
        run.getPropertyValueAsType<double>("VirtualChopper.poff");
    const auto openOffset =
        run.getPropertyValueAsType<double>("VirtualChopper.open_offset");
    const auto tof0 =
        tofDelay -
        60e6 * (pOffset - 45 + chopper2Phase - chopper1Phase + openOffset) /
            (2 * 360 * chopper1Speed);
    TS_ASSERT_EQUALS(output->blocksize(), channelCount);
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      const auto &xs = output->x(i);
      for (size_t j = 0; j < xs.size(); ++j) {
        const auto tof = tof0 + static_cast<double>(j) * channelWidth;
        TS_ASSERT_DELTA(xs[j], tof, 1e-12)
      }
    }
  }

  void testTOFFigaro() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("XUnit", "TimeOfFlight");
    getWorkspaceFor(output, m_figaroFile, m_outWSName, prop);
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "TOF");
    const auto &run = output->run();
    const auto channelWidth =
        run.getPropertyValueAsType<double>("PSD.time_of_flight_0");
    const auto channelCount = static_cast<size_t>(
        run.getPropertyValueAsType<double>("PSD.time_of_flight_1"));
    const auto tofDelay =
        run.getPropertyValueAsType<double>("PSD.time_of_flight_2") +
        run.getPropertyValueAsType<double>("Theta.edelay_delay");
    // Using choppers 1 and 4.
    const auto chopper1Speed =
        run.getPropertyValueAsType<double>("CH1.rotation_speed");
    const double chopper1Phase{0.}; // The value in NeXus is trash.
    const auto chopper2Phase = run.getPropertyValueAsType<double>("CH4.phase");
    const auto pOffset = run.getPropertyValueAsType<double>("CollAngle.poff");
    const auto openOffset =
        run.getPropertyValueAsType<double>("CollAngle.openOffset");
    const auto tof0 =
        tofDelay -
        60e6 * (pOffset - 45. + chopper2Phase - chopper1Phase + openOffset) /
            (2. * 360. * chopper1Speed);
    TS_ASSERT_EQUALS(output->blocksize(), channelCount);
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      const auto &xs = output->x(i);
      for (size_t j = 0; j < xs.size(); ++j) {
        const auto tof = tof0 + static_cast<double>(j) * channelWidth;
        TS_ASSERT_DELTA(xs[j], tof, 1e-12)
      }
    }
  }

  void testSampleAndSourcePositionsD17() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("Xunit", "TimeOfFlight");
    getWorkspaceFor(output, m_d17File, m_outWSName, prop);
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

  void testSampleAndSourcePositionsFigaro() {
    MatrixWorkspace_sptr output;
    auto prop = emptyProperties();
    prop.emplace_back("Xunit", "TimeOfFlight");
    getWorkspaceFor(output, m_figaroFile, m_outWSName, prop);
    const auto &run = output->run();
    const auto chopperCentre =
        run.getPropertyValueAsType<double>(
            "ChopperSetting.chopperpair_sample_distance") *
        1e-3;
    const auto incomingDeflectionAngle =
        run.getPropertyValueAsType<double>("CollAngle.actual_coll_angle");
    const auto sampleZOffset =
        run.getPropertyValueAsType<double>("Theta.sampleHorizontalOffset") *
        1e-3;
    const auto sourceSample =
        chopperCentre +
        sampleZOffset / std::cos(incomingDeflectionAngle / 180. * M_PI);
    const auto &spectrumInfo = output->spectrumInfo();
    const auto l1 = spectrumInfo.l1();
    TS_ASSERT_DELTA(sourceSample, l1, 1e-12)
    const auto samplePos = spectrumInfo.samplePosition();
    const auto sourcePos = spectrumInfo.sourcePosition();
    TS_ASSERT_EQUALS(samplePos.X(), 0)
    TS_ASSERT_EQUALS(samplePos.Y(), 0)
    TS_ASSERT_EQUALS(samplePos.Z(), 0)
    TS_ASSERT_EQUALS(sourcePos.X(), 0)
    TS_ASSERT_EQUALS(sourcePos.Y(), 0)
    TS_ASSERT_EQUALS(sourcePos.Z(), -sourceSample)
  }

  void testDetectorPositionAndRotationD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, m_outWSName, emptyProperties());
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
      const auto pixDist = std::hypot(pixOffset, detDist);
      const auto idealX = pixDist * std::sin(pixAngle);
      const auto idealZ = pixDist * std::cos(pixAngle);
      TS_ASSERT_DELTA(p.X(), idealX, 1e-8)
      TS_ASSERT_DELTA(p.Z(), idealZ, 1e-8)
    }
  }

  void testDetectorPositionAndRotationFigaro() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_figaroFile, m_outWSName, emptyProperties());
    const auto &spectrumInfo = output->spectrumInfo();
    const auto &run = output->run();
    const auto detectorRestZ =
        run.getPropertyValueAsType<double>("DTR.value") * 1e-3;
    const auto DH1Y = run.getPropertyValueAsType<double>("DH1.value") * 1e-3;
    const double DH1Z{1.135};
    const auto DH2Y = run.getPropertyValueAsType<double>("DH2.value") * 1e-3;
    const double DH2Z{2.077};
    const auto detAngle = std::atan2(DH2Y - DH1Y, DH2Z - DH1Z);
    const double detectorRestY{0.509};
    const auto detectorY =
        std::sin(detAngle) * (detectorRestZ - DH1Z) + DH1Y - detectorRestY;
    const auto detectorZ = std::cos(detAngle) * (detectorRestZ - DH1Z) + DH1Z;
    const auto pixWidth = run.getPropertyValueAsType<double>("PSD.mppy") * 1e-3;
    const auto pixelOffset = detectorRestY - 0.5 * pixWidth;
    const auto beamY = detectorY + pixelOffset * std::cos(detAngle);
    const auto beamZ = detectorZ - pixelOffset * std::sin(detAngle);
    const auto sht1 = run.getPropertyValueAsType<double>("SHT1.value") * 1e-3;
    const auto sampleZOffset =
        run.getPropertyValueAsType<double>("Theta.sampleHorizontalOffset") *
        1e-3;
    const auto collimationAngle =
        run.getPropertyValueAsType<double>("CollAngle.actual_coll_angle") /
        180. * M_PI;
    const auto detDist = std::hypot(beamY - sht1, beamZ) -
                         sampleZOffset / std::cos(collimationAngle);
    const auto sampleAngle =
        run.getPropertyValueAsType<double>("Theta.actual_theta") / 180. * M_PI;
    for (size_t i = 0; i < spectrumInfo.size(); ++i) {
      if (spectrumInfo.isMonitor(i)) {
        continue;
      }
      const auto p = spectrumInfo.position(i);
      TS_ASSERT_EQUALS(p.X(), 0)
      const auto pixOffset = (static_cast<double>(i) - 127.5) * pixWidth;
      const auto pixAngle = detAngle + collimationAngle + sampleAngle +
                            std::atan2(pixOffset, detDist);
      const auto pixDist = std::hypot(pixOffset, detDist);
      const auto idealY = pixDist * std::sin(pixAngle);
      const auto idealZ = pixDist * std::cos(pixAngle);
      TS_ASSERT_DELTA(p.Y(), idealY, 1e-8)
      TS_ASSERT_DELTA(p.Z(), idealZ, 1e-8)
    }
  }

  void test2ThetaD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, m_outWSName, emptyProperties());
    // Compare angles in rad
    const auto &spectrumInfo = output->spectrumInfo();
    // Check twoTheta between two center detectors.
    const auto dan = output->run().getPropertyValueAsType<double>("dan.value");
    TS_ASSERT_LESS_THAN_EQUALS(spectrumInfo.twoTheta(128) * 180 / M_PI, dan)
    TS_ASSERT_LESS_THAN_EQUALS(dan, spectrumInfo.twoTheta(127) * 180 / M_PI)
  }

  void testUserAngleD17() {
    MatrixWorkspace_sptr output;
    const double angle = 23.23;
    auto prop = emptyProperties();
    prop.emplace_back("BraggAngle", std::to_string(angle));
    getWorkspaceFor(output, m_d17File, m_outWSName, prop);
    const double peakOffsetAngle = -1.64; // Approximately known value.
    const double detectorAngle = 2 * angle - peakOffsetAngle;
    const auto &spectrumInfo = output->spectrumInfo();
    TS_ASSERT_LESS_THAN_EQUALS(spectrumInfo.twoTheta(128) * 180 / M_PI,
                               detectorAngle)
    TS_ASSERT_LESS_THAN_EQUALS(detectorAngle,
                               spectrumInfo.twoTheta(127) * 180 / M_PI)
  }

  void testUserAngleFigaro() {
    MatrixWorkspace_sptr output;
    const double angle = 23.23;
    auto prop = emptyProperties();
    const int detector{0};
    prop.emplace_back("BeamCentre", std::to_string(detector));
    prop.emplace_back("BraggAngle", std::to_string(angle));
    getWorkspaceFor(output, m_figaroFile, m_outWSName, prop);
    const double detectorAngle = 2 * angle;
    const auto &spectrumInfo = output->spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(detector) * 180. / M_PI,
                    detectorAngle, 1e-6)
  }

  void testPropertiesD17() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_d17File, m_outWSName, emptyProperties());
    commonProperties(output, "D17");
  }

  void testPropertiesFigaro() {
    MatrixWorkspace_sptr output;
    getWorkspaceFor(output, m_figaroFile, m_outWSName, emptyProperties());
    commonProperties(output, "Figaro");
  }

  void testDirectBeamOutput() {
    using namespace Mantid::DataObjects;
    MatrixWorkspace_sptr output;
    const std::string beamPosWSName{"LoadILLReflectometryTest_BeapPositionWS"};
    auto prop = emptyProperties();
    prop.emplace_back("OutputBeamPosition", beamPosWSName);
    getWorkspaceFor(output, m_d17DirectBeamFile, m_outWSName, prop);
    TableWorkspace_sptr beamPosWS =
        AnalysisDataService::Instance().retrieveWS<TableWorkspace>(
            beamPosWSName);
    TS_ASSERT(beamPosWS)
    TS_ASSERT_EQUALS(beamPosWS->rowCount(), 1)
    TS_ASSERT_EQUALS(beamPosWS->columnCount(), 3)
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
        std::count(colNames.cbegin(), colNames.cend(), "PeakCentre"), 1)
    const auto peakCentres = beamPosWS->getColVector<double>("PeakCentre");
    TS_ASSERT_DELTA(peakCentres.front(), 202.5, 0.5)
  }

  void testDirectBeamInput() {
    using namespace Mantid::DataObjects;
    MatrixWorkspace_sptr dbOutput;
    const std::string dbBeamPosWSName{
        "LoadILLReflectometryTest_DbBeamPositionWS"};
    auto prop = emptyProperties();
    prop.emplace_back("OutputBeamPosition", dbBeamPosWSName);
    getWorkspaceFor(dbOutput, m_d17DirectBeamFile,
                    "LoadILLReflectometryTest_DirectBeamWS", prop);
    TableWorkspace_sptr dbBeamPosWS =
        AnalysisDataService::Instance().retrieveWS<TableWorkspace>(
            dbBeamPosWSName);
    MatrixWorkspace_sptr refOutput;
    prop.clear();
    prop.emplace_back("DirectBeamPosition", dbBeamPosWSName);
    getWorkspaceFor(refOutput, m_d17File, m_outWSName, prop);
    const auto dbDetAngle =
        dbOutput->run().getPropertyValueAsType<double>("dan.value");
    const auto dbDetDist =
        dbBeamPosWS->cell_cast<double>(0, "DetectorDistance");
    const auto dbPeakPos = dbBeamPosWS->cell_cast<double>(0, "PeakCentre");
    const auto dbPixWidth =
        dbOutput->run().getPropertyValueAsType<double>("PSD.mppx") / 1000;
    const auto dbPeakOffset = (127.5 - dbPeakPos) * dbPixWidth;
    const auto dbOffsetAngle = std::atan2(dbPeakOffset, dbDetDist) * 180 / M_PI;
    const auto refDetAngle =
        refOutput->run().getPropertyValueAsType<double>("dan.value");
    const auto newDetAngle = refDetAngle - dbDetAngle - dbOffsetAngle;
    const auto &spectrumInfo = refOutput->spectrumInfo();
    TS_ASSERT_LESS_THAN_EQUALS(spectrumInfo.twoTheta(128) * 180 / M_PI,
                               newDetAngle)
    TS_ASSERT_LESS_THAN_EQUALS(newDetAngle,
                               spectrumInfo.twoTheta(127) * 180 / M_PI)
  }

  void testDirectBeamIgnoredWhenBraggAngleGiven() {
    using namespace Mantid::DataObjects;
    MatrixWorkspace_sptr dbOutput;
    const std::string dbBeamPosWSName{
        "LoadILLReflectometryTest_DbBeamPositionWS"};
    auto prop = emptyProperties();
    prop.emplace_back("OutputBeamPosition", dbBeamPosWSName);
    getWorkspaceFor(dbOutput, m_d17DirectBeamFile,
                    "LoadILLReflectometryTest_DirectBeamWS", prop);
    MatrixWorkspace_sptr refOutput;
    const double userAngle{23.23};
    const std::string refBeamPosWSName{
        "LoadILLReflectometryTest_RefBeamPositionWS"};
    prop.clear();
    prop.emplace_back("DirectBeamPosition", dbBeamPosWSName);
    prop.emplace_back("BraggAngle", std::to_string(userAngle));
    prop.emplace_back("OutputBeamPosition", refBeamPosWSName);
    getWorkspaceFor(refOutput, m_d17File, m_outWSName, prop);
    TableWorkspace_sptr refBeamPosWS =
        AnalysisDataService::Instance().retrieveWS<TableWorkspace>(
            refBeamPosWSName);
    const auto refDetDist =
        refOutput->run().getPropertyValueAsType<double>("det.value") / 1000;
    const auto refPeakPos = refBeamPosWS->cell_cast<double>(0, "PeakCentre");
    const auto refPixWidth =
        refOutput->run().getPropertyValueAsType<double>("PSD.mppx") / 1000;
    const auto refPeakOffset = (127.5 - refPeakPos) * refPixWidth;
    const auto refOffsetAngle =
        std::atan2(refPeakOffset, refDetDist) * 180 / M_PI;
    const auto userDetectorAngle = 2 * userAngle - refOffsetAngle;
    const auto &spectrumInfo = refOutput->spectrumInfo();
    TS_ASSERT_LESS_THAN_EQUALS(spectrumInfo.twoTheta(128) * 180 / M_PI,
                               userDetectorAngle)
    TS_ASSERT_LESS_THAN_EQUALS(userDetectorAngle,
                               spectrumInfo.twoTheta(127) * 180 / M_PI)
  }

  void testPeakCentre() {
    MatrixWorkspace_sptr output;
    constexpr double peakPosition{42};
    constexpr double angle{23.23};
    auto prop = emptyProperties();
    prop.emplace_back("BeamCentre", std::to_string(peakPosition));
    prop.emplace_back("BraggAngle", std::to_string(angle));
    getWorkspaceFor(output, m_d17File, m_outWSName, prop);
    const auto &spectrumInfo = output->spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(42) * 180 / M_PI, 2 * angle, 1e-6)
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
