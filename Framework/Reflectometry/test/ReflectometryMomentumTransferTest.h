// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidReflectometry/ReflectometryMomentumTransfer.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/math/special_functions/pow.hpp>

using namespace Mantid;

namespace {
constexpr double CHOPPER_GAP{0.23};
constexpr double CHOPPER_OPENING_ANGLE{33.}; // degrees
constexpr double CHOPPER_RADIUS{0.3};
constexpr double CHOPPER_SPEED{990.};
constexpr double DET_DIST{4.};
constexpr double DET_RESOLUTION{0.002};
constexpr double L1{8.};
constexpr double PIXEL_SIZE{0.0015};
// h / NeutronMass
constexpr double PLANCK_PER_KG{3.9560340102631226e-7};
constexpr double SLIT1_SIZE{0.03};
constexpr double SLIT1_DIST{1.2};
constexpr double SLIT2_DIST{0.3};
constexpr double SLIT2_SIZE{0.02};
constexpr double TOF_BIN_WIDTH{70.}; // microseconds
} // namespace

class ReflectometryMomentumTransferTest : public CxxTest::TestSuite {
public:
  struct LogValues {
    const double om_fwhm;
    const double s2_fwhm;
    const double s3_fwhm;
    const double da;
    LogValues(const double om_fwhm_, const double s2_fwhm_,
              const double s3_fwhm_, const double da_)
        : om_fwhm(om_fwhm_), s2_fwhm(s2_fwhm_), s3_fwhm(s3_fwhm_), da(da_) {}
  };

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryMomentumTransferTest *createSuite() {
    return new ReflectometryMomentumTransferTest();
  }
  static void destroySuite(ReflectometryMomentumTransferTest *suite) {
    delete suite;
  }

  template <typename LogValues>
  static API::MatrixWorkspace_sptr make_ws(const double braggAngle,
                                           const int nBins,
                                           const LogValues &logValues) {
    using namespace WorkspaceCreationHelper;
    constexpr double startX{1000.};
    const Kernel::V3D sourcePos{0., 0., -L1};
    const Kernel::V3D &monitorPos = sourcePos;
    const Kernel::V3D samplePos{
        0.,
        0.,
        0.,
    };
    const auto detZ = DET_DIST * std::cos(2 * braggAngle);
    const auto detY = DET_DIST * std::sin(2 * braggAngle);
    const Kernel::V3D detectorPos{0., detY, detZ};
    const Kernel::V3D slit1Pos{0., 0., -SLIT1_DIST};
    const Kernel::V3D slit2Pos{0., 0., -SLIT2_DIST};
    auto ws = create2DWorkspaceWithReflectometryInstrument(
        startX, slit1Pos, slit2Pos, SLIT1_SIZE, SLIT2_SIZE, sourcePos,
        monitorPos, samplePos, detectorPos, nBins, TOF_BIN_WIDTH);
    ws = extractNonMonitorSpectrum(ws);
    addSlitSampleLogs(*ws);
    addBeamStatisticsSampleLogs(*ws, logValues);
    ws = convertToWavelength(ws);
    return ws;
  }

  ReflectometryMomentumTransferTest() { API::FrameworkManager::Instance(); }

  void test_Init() {
    Reflectometry::ReflectometryMomentumTransfer alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_XYEFromInputUnchanged() {
    using Geometry::deg2rad;
    const LogValues logValues(0.1, 0.1, 0.1, 0.1);
    constexpr int nBins{10};
    auto inputWS = make_ws(0.5 * deg2rad, nBins, logValues);
    const std::vector<int> foreground(2, 0);
    auto alg = make_alg(inputWS, "SumInLambda", foreground);
    TS_ASSERT_THROWS_NOTHING(alg->execute())
    TS_ASSERT(alg->isExecuted())

    API::MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    const auto axis = outputWS->getAxis(0);
    TS_ASSERT_EQUALS(axis->unit()->unitID(), "MomentumTransfer")
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(),
                     inputWS->getNumberHistograms())
    const auto &inXs = inputWS->x(0);
    const auto &outXs = outputWS->x(0);
    TS_ASSERT_EQUALS(outXs.size(), inXs.size())
    TS_ASSERT(outputWS->hasDx(0))
    const auto &inYs = inputWS->y(0);
    TS_ASSERT_EQUALS(outputWS->y(0).rawData(), inYs.rawData())
    const auto &inEs = inputWS->e(0);
    const auto &outEs = outputWS->e(0);
    TS_ASSERT_EQUALS(outEs.rawData(), inEs.rawData())
  }

  void test_sumInQResultsAreValid() {
    using Geometry::deg2rad;
    const std::string sumType{"SumInQ"};
    constexpr double s3_fwhm{0.1};
    constexpr double da{0.1};
    constexpr double angleBragg{1.5 * deg2rad};
    const std::vector<int> foreground(2, 0);
    sameReflectedAndDirectSlitSizes(sumType, angleBragg, foreground,
                                    sumInQBeamDivergenceDominated(s3_fwhm, da));
    sameReflectedAndDirectSlitSizes(
        sumType, angleBragg, foreground,
        sumInQBentSampleDominateSmallSecondSlitAngularSpread(s3_fwhm, da));
    sameReflectedAndDirectSlitSizes(
        sumType, angleBragg, foreground,
        sumInQBentSampleDominatedLargeSecondSlitAngularSpread(s3_fwhm, da));
    sameReflectedAndDirectSlitSizes(
        sumType, angleBragg, foreground,
        sumInQDetectorResolutionDominated(s3_fwhm, da));
  }

  void test_sumInLambdaAngularResolutionDominatesResultsAreValid() {
    using Geometry::deg2rad;
    const std::string sumType{"SumInLambda"};
    constexpr double om_fwhm{0.001};
    constexpr double s2_fwhm{0.1};
    constexpr double s3_fwhm{0.1};
    constexpr double da{0.001};
    constexpr double angleBragg{1.23 * deg2rad};
    const LogValues angularResolutionDominatedLogValues(om_fwhm, s2_fwhm,
                                                        s3_fwhm, da);
    std::vector<int> foreground(2);
    foreground.front() = 0;
    foreground.back() = 40;
    TS_ASSERT(
        err_ray_SumInLambda(angleBragg, angularResolutionDominatedLogValues) <
        err_ray_temp(foreground, DET_DIST, angleBragg))
    sameReflectedAndDirectSlitSizes(sumType, angleBragg, foreground,
                                    angularResolutionDominatedLogValues);
  }

  void test_sumInLambdaForegroundWidthDominatesResultsAreValid() {
    using Geometry::deg2rad;
    const std::string sumType{"SumInLambda"};
    constexpr double om_fwhm{0.1};
    constexpr double s2_fwhm{0.1};
    constexpr double s3_fwhm{0.1};
    constexpr double da{0.1};
    constexpr double angleBragg{1.23 * deg2rad};
    const LogValues foregroundWidthDominatedLogValues(om_fwhm, s2_fwhm, s3_fwhm,
                                                      da);
    std::vector<int> foreground(2);
    foreground.front() = 0;
    foreground.back() = 10;
    TS_ASSERT(
        err_ray_SumInLambda(angleBragg, foregroundWidthDominatedLogValues) >
        err_ray_temp(foreground, DET_DIST, angleBragg))
    sameReflectedAndDirectSlitSizes(sumType, angleBragg, foreground,
                                    foregroundWidthDominatedLogValues);
  }

  void test_failsGracefullyWhenSlitsNotFound() {
    constexpr int firstSlit{1};
    wrongSlitNames(firstSlit);
    constexpr int secondSlit{2};
    wrongSlitNames(secondSlit);
  }

private:
  static void sameReflectedAndDirectSlitSizes(
      const std::string &sumType, const double angleBragg,
      const std::vector<int> &foreground, const LogValues &logValues) {
    using namespace boost::math;
    constexpr int nBins{10};
    auto inputWS = make_ws(angleBragg, nBins, logValues);
    inputWS->mutableY(0) = 1. / static_cast<double>(inputWS->y(0).size());
    auto alg = make_alg(inputWS, sumType, foreground);
    TS_ASSERT_THROWS_NOTHING(alg->execute())
    TS_ASSERT(alg->isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    alg = API::AlgorithmManager::Instance().createUnmanaged("ConvertUnits");
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("OutputWorkspace", "unused_for_child");
    alg->setProperty("Target", "MomentumTransfer");
    alg->execute();
    API::MatrixWorkspace_sptr qWS = alg->getProperty("OutputWorkspace");
    const auto axis = outputWS->getAxis(0);
    TS_ASSERT_EQUALS(axis->unit()->unitID(), "MomentumTransfer")
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(),
                     inputWS->getNumberHistograms())
    const auto &spectrumInfo = outputWS->spectrumInfo();
    const auto &inQs = qWS->points(0);
    const auto &outPoints = outputWS->points(0);
    TS_ASSERT_EQUALS(outPoints.size(), inQs.size())
    TS_ASSERT(outputWS->hasDx(0))
    TS_ASSERT(!outputWS->spectrumInfo().isMonitor(0))
    const auto &outDx = outputWS->dx(0);
    TS_ASSERT_EQUALS(outDx.size(), inQs.size())
    const auto &lambdas = inputWS->points(0);
    const auto l2 = spectrumInfo.l2(0);
    const auto angle_bragg = spectrumInfo.twoTheta(0) / 2.;
    const auto rayE = err_ray(foreground, l2, angle_bragg, sumType, logValues);
    for (size_t j = 0; j < lambdas.size(); ++j) {
      const auto lambda = lambdas[j] * 1e-10;
      const size_t qIndex = inQs.size() - j - 1;
      const auto q = inQs[qIndex];
      const auto resE = std::sqrt(pow<2>(err_res(lambda, l2)) +
                                  pow<2>(width_res(lambda, l2)));
      const auto fractionalResolution = std::sqrt(pow<2>(resE) + pow<2>(rayE));
      TS_ASSERT_EQUALS(outPoints[qIndex], q)
      TS_ASSERT_DELTA(outDx[qIndex], q * fractionalResolution, 1e-7)
    }
  }

  void wrongSlitNames(const int nonexistentSlit) {
    using Geometry::deg2rad;
    const std::string slit1 = nonexistentSlit == 1 ? "non-existent" : "slit1";
    const std::string slit2 = nonexistentSlit == 2 ? "non-existent" : "slit2";
    const LogValues logValues(0.1, 0.1, 0.1, 0.1);
    constexpr int nBins{10};
    auto inputWS = make_ws(0.5 * deg2rad, nBins, logValues);
    const std::vector<int> foreground(2, 0);
    auto alg = std::make_shared<Reflectometry::ReflectometryMomentumTransfer>();
    alg->setChild(true);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->initialize())
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ReflectedForeground", foreground))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SummationType", "SumInLambda"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("PixelSize", PIXEL_SIZE))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("DetectorResolution", DET_RESOLUTION))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("ChopperSpeed", CHOPPER_SPEED))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ChopperOpening", CHOPPER_OPENING_ANGLE))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("ChopperRadius", CHOPPER_RADIUS))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ChopperpairDistance", CHOPPER_GAP))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("FirstSlitName", "non-existent"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("FirstSlitSizeSampleLog", "slit1.size"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SecondSlitName", slit1))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SecondSlitSizeSampleLog", slit2))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TOFChannelWidth", TOF_BIN_WIDTH))
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::runtime_error &e,
                            e.what(),
                            std::string("Some invalid Properties found"))
    TS_ASSERT(!alg->isExecuted())
  }

  static API::Algorithm_sptr make_alg(const API::MatrixWorkspace_sptr &inputWS,
                                      const std::string &sumType,
                                      const std::vector<int> &foreground) {
    auto alg = std::make_shared<Reflectometry::ReflectometryMomentumTransfer>();
    alg->setChild(true);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->initialize())
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ReflectedForeground", foreground))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SummationType", sumType))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("PixelSize", PIXEL_SIZE))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("DetectorResolution", DET_RESOLUTION))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("ChopperSpeed", CHOPPER_SPEED))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ChopperOpening", CHOPPER_OPENING_ANGLE))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("ChopperRadius", CHOPPER_RADIUS))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ChopperpairDistance", CHOPPER_GAP))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("FirstSlitName", "slit1"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("FirstSlitSizeSampleLog", "slit1.size"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SecondSlitName", "slit2"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("SecondSlitSizeSampleLog", "slit2.size"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TOFChannelWidth", TOF_BIN_WIDTH))
    return alg;
  }

  static void addBeamStatisticsSampleLogs(API::MatrixWorkspace &ws,
                                          const LogValues &values) {
    auto &run = ws.mutableRun();
    constexpr bool overwrite{true};
    run.addProperty("beam_stats.incident_angular_spread", values.da, overwrite);
    run.addProperty("beam_stats.first_slit_angular_spread", values.s2_fwhm,
                    overwrite);
    run.addProperty("beam_stats.second_slit_angular_spread", values.s3_fwhm,
                    overwrite);
    run.addProperty("beam_stats.sample_waviness", values.om_fwhm, overwrite);
  }

  static void addSlitSampleLogs(API::MatrixWorkspace &ws) {
    auto &run = ws.mutableRun();
    constexpr bool overwrite{true};
    const std::string meters{"m"};
    run.addProperty("slit1.size", SLIT1_SIZE, meters, overwrite);
    run.addProperty("slit2.size", SLIT2_SIZE, meters, overwrite);
  }

  static API::MatrixWorkspace_sptr
  convertToWavelength(API::MatrixWorkspace_sptr &ws) {
    auto alg =
        API::AlgorithmManager::Instance().createUnmanaged("ConvertUnits");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", "_unused_for_child");
    alg->setProperty("Target", "Wavelength");
    alg->setProperty("EMode", "Elastic");
    alg->execute();
    return alg->getProperty("OutputWorkspace");
  }

  static API::MatrixWorkspace_sptr
  extractNonMonitorSpectrum(API::MatrixWorkspace_sptr &ws) {
    auto alg = API::AlgorithmManager::Instance().createUnmanaged(
        "ExtractSingleSpectrum");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", "_unused_for_child");
    alg->setProperty("WorkspaceIndex", 0);
    alg->execute();
    return alg->getProperty("OutputWorkspace");
  }

  static const LogValues
  sumInQBentSampleDominatedLargeSecondSlitAngularSpread(const double s3_fwhm,
                                                        const double da) {
    constexpr double om_fwhm{0.1};
    constexpr double s2_fwhm{2.1 * om_fwhm};
    return LogValues(om_fwhm, s2_fwhm, s3_fwhm, da);
  }

  static const LogValues
  sumInQBentSampleDominateSmallSecondSlitAngularSpread(const double s3_fwhm,
                                                       const double da) {
    constexpr double om_fwhm{0.1};
    constexpr double s2_fwhm{1.9 * om_fwhm};
    return LogValues(om_fwhm, s2_fwhm, s3_fwhm, da);
  }

  static const LogValues sumInQBeamDivergenceDominated(const double s3_fwhm,
                                                       const double da) {
    constexpr double om_fwhm{-0.1};
    constexpr double s2_fwhm{1.1 * DET_RESOLUTION / DET_DIST};
    return LogValues(om_fwhm, s2_fwhm, s3_fwhm, da);
  }

  static const LogValues sumInQDetectorResolutionDominated(const double s3_fwhm,
                                                           const double da) {
    constexpr double om_fwhm{-0.1};
    constexpr double s2_fwhm{0.9 * DET_RESOLUTION / DET_DIST};
    return LogValues(om_fwhm, s2_fwhm, s3_fwhm, da);
  }

  static double err_ray_temp(const std::vector<int> &foreground,
                             const double l2, const double angle_bragg) {
    using namespace boost::math;
    const auto width = foreground.back() - foreground.front() + 1;
    return 0.68 *
           std::sqrt((pow<2>(width * PIXEL_SIZE) + pow<2>(SLIT2_SIZE)) /
                     pow<2>(l2)) /
           angle_bragg;
  }

  static double err_ray_SumInLambda(const double angle_bragg,
                                    const LogValues &values) {
    using namespace boost::math;
    return std::sqrt(pow<2>(values.da) + pow<2>(values.om_fwhm)) / angle_bragg;
  }

  static double err_ray(const std::vector<int> &foreground, const double l2,
                        const double angle_bragg, const std::string &sumType,
                        const LogValues &values) {
    using namespace boost::math;
    double err;
    if (sumType == "SumInQ") {
      if (values.om_fwhm > 0) {
        if (values.s2_fwhm >= 2 * values.om_fwhm) {
          err = std::sqrt(pow<2>(DET_RESOLUTION / (SLIT2_DIST + l2)) +
                          pow<2>(values.s3_fwhm) + pow<2>(values.om_fwhm)) /
                angle_bragg;
        } else {
          err = std::sqrt(pow<2>(DET_RESOLUTION / (2. * (SLIT2_DIST + l2))) +
                          pow<2>(values.s3_fwhm) + pow<2>(values.s2_fwhm)) /
                angle_bragg;
        }
      } else {
        if (values.s2_fwhm > DET_RESOLUTION / l2) {
          err = std::sqrt(pow<2>(DET_RESOLUTION / (SLIT2_DIST + l2)) +
                          pow<2>(values.s3_fwhm)) /
                angle_bragg;
        } else {
          err = std::sqrt(pow<2>(values.da) +
                          pow<2>(DET_RESOLUTION / (SLIT2_DIST + l2))) /
                angle_bragg;
        }
      }
    } else {
      err = err_ray_SumInLambda(angle_bragg, values);
      const auto temp = err_ray_temp(foreground, l2, angle_bragg);
      err = std::min(err, temp);
    }
    return err;
  }

  static double err_res(const double lambda, const double l2) {
    using namespace boost::math;
    const auto tofd = L1 + l2;
    const auto period = 60. / CHOPPER_SPEED;
    const auto det_res =
        PLANCK_PER_KG * TOF_BIN_WIDTH * 1e-6 / lambda / (2 * tofd);
    const auto chop_res =
        (CHOPPER_GAP +
         (PLANCK_PER_KG * CHOPPER_OPENING_ANGLE * period / (360 * lambda))) /
        (2 * tofd);
    return 0.98 *
           (3 * pow<2>(chop_res) + pow<2>(det_res) + 3 * chop_res * det_res) /
           (2 * chop_res + det_res);
  }

  static double width_res(const double lambda, const double l2) {
    using namespace boost::math;
    const auto tofd = L1 + l2;
    const auto period = 60. / CHOPPER_SPEED;
    const auto sdr = SLIT2_DIST + l2;
    const auto interslit = SLIT1_DIST - SLIT2_DIST;
    const auto tempratio = (tofd - sdr) / interslit;
    const auto tempa =
        tempratio * std::abs(SLIT1_SIZE - SLIT2_SIZE) + SLIT1_SIZE;
    const auto tempb = tempratio * (SLIT1_SIZE + SLIT2_SIZE) + SLIT1_SIZE;
    const auto tempwidthfwhm = 0.49 * (pow<3>(tempb) - pow<3>(tempa)) /
                               (pow<2>(tempb) - pow<2>(tempa));
    return tempwidthfwhm * period / (2 * M_PI * CHOPPER_RADIUS) *
           PLANCK_PER_KG / lambda / tofd;
  }
};

class ReflectometryMomentumTransferTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    using Geometry::deg2rad;
    constexpr int nBins{10000};
    ReflectometryMomentumTransferTest::LogValues logValues(0.1, 0.1, 0.1, 0.1);
    m_reflectedWS = ReflectometryMomentumTransferTest::make_ws(
        0.7 * deg2rad, nBins, logValues);
    m_algorithm = makeAlgorithm(m_reflectedWS);
  }

  void test_performance() {
    for (int i = 0; i < 1000; ++i)
      m_algorithm->execute();
  }

private:
  static API::IAlgorithm_sptr
  makeAlgorithm(API::MatrixWorkspace_sptr &reflectedWS) {
    std::vector<int> foreground(2, 0);
    auto alg = std::make_shared<Reflectometry::ReflectometryMomentumTransfer>();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", reflectedWS);
    alg->setPropertyValue("OutputWorkspace", "_unused_for_child");
    alg->setProperty("ReflectedForeground", foreground);
    alg->setProperty("SummationType", "SumInLambda");
    alg->setProperty("PixelSize", PIXEL_SIZE);
    alg->setProperty("DetectorResolution", DET_RESOLUTION);
    alg->setProperty("ChopperSpeed", CHOPPER_SPEED);
    alg->setProperty("ChopperOpening", CHOPPER_OPENING_ANGLE);
    alg->setProperty("ChopperRadius", CHOPPER_RADIUS);
    alg->setProperty("ChopperpairDistance", CHOPPER_GAP);
    alg->setProperty("FirstSlitName", "slit1");
    alg->setProperty("FirstSlitSizeSampleLog", "slit1.size");
    alg->setProperty("SecondSlitName", "slit2");
    alg->setProperty("SecondSlitSizeSampleLog", "slit2.size");
    alg->setProperty("TOFChannelWidth", TOF_BIN_WIDTH);
    return alg;
  }

private:
  API::IAlgorithm_sptr m_algorithm;
  API::MatrixWorkspace_sptr m_reflectedWS;
};
