#ifndef MANTID_ALGORITHMS_REFLECTOMETRYMOMENTUMTRANSFERTEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYMOMENTUMTRANSFERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometryMomentumTransfer.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"
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
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryMomentumTransferTest *createSuite() {
    return new ReflectometryMomentumTransferTest();
  }
  static void destroySuite(ReflectometryMomentumTransferTest *suite) {
    delete suite;
  }

  ReflectometryMomentumTransferTest() { API::FrameworkManager::Instance(); }

  void test_Init() {
    Algorithms::ReflectometryMomentumTransfer alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_XYEFromInputUnchangedAndMonitorDXSetToZero() {
    auto inputWS = make_ws(0.5 / 180. * M_PI);
    API::MatrixWorkspace_sptr directWS = inputWS->clone();
    auto alg = make_alg(inputWS, directWS, "SumInLambda", false);
    TS_ASSERT_THROWS_NOTHING(alg->execute();)
    TS_ASSERT(alg->isExecuted())

    API::MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    const auto axis = outputWS->getAxis(0);
    TS_ASSERT_EQUALS(axis->unit()->unitID(), "MomentumTransfer")
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(),
                     inputWS->getNumberHistograms())
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      const auto &inXs = inputWS->x(i);
      const auto &outXs = outputWS->x(i);
      TS_ASSERT_EQUALS(outXs.size(), inXs.size())
      TS_ASSERT(outputWS->hasDx(i))
      if (i == 1) {
        // Monitor should have Dx = 0
        TS_ASSERT(outputWS->spectrumInfo().isMonitor(i))
        const auto &outDx = outputWS->dx(i);
        for (size_t j = 0; j < outDx.size(); ++j) {
          TS_ASSERT_EQUALS(outDx[j], 0.)
        }
      }
      const auto &inYs = inputWS->y(i);
      const auto &outYs = outputWS->y(i);
      TS_ASSERT_EQUALS(outYs.rawData(), inYs.rawData())
      const auto &inEs = inputWS->e(i);
      const auto &outEs = outputWS->e(i);
      TS_ASSERT_EQUALS(outEs.rawData(), inEs.rawData())
    }
  }

  void test_nonpolarizedSumInLambdaResultsAreValid() {
    const bool polarized(false);
    const std::string sumType{"SumInLambda"};
    sameReflectedAndDirectSlitSizes(polarized, sumType);
  }

  void test_polarizedSumInLambdaResultsAreValid() {
    const bool polarized(true);
    const std::string sumType{"SumInLambda"};
    sameReflectedAndDirectSlitSizes(polarized, sumType);
  }

  void test_nonpolarizedSumInQResultsAreValid() {
    const bool polarized(false);
    const std::string sumType{"SumInQ"};
    sameReflectedAndDirectSlitSizes(polarized, sumType);
  }

  void test_polarizedSumInQResultsAreValid() {
    const bool polarized(true);
    const std::string sumType{"SumInQ"};
    sameReflectedAndDirectSlitSizes(polarized, sumType);
  }

  void test_differentReflectedAndDirectSlitSizes() {
    using namespace boost::math;
    const bool polarized{false};
    const std::string sumType{"SumInLambda"};
    auto inputWS = make_ws(0.5 / 180. * M_PI);
    inputWS->mutableY(0) = 1. / static_cast<double>(inputWS->y(0).size());
    API::MatrixWorkspace_sptr directWS = inputWS->clone();
    auto &run = directWS->mutableRun();
    constexpr bool overwrite{true};
    const std::string meters{"m"};
    run.addProperty("slit1.size", 1.5 * SLIT1_SIZE, meters, overwrite);
    run.addProperty("slit2.size", 1.5 * SLIT2_SIZE, meters, overwrite);
    auto alg = make_alg(inputWS, directWS, sumType, polarized);
    TS_ASSERT_THROWS_NOTHING(alg->execute();)
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
    const auto &dirSpectrumInfo = directWS->spectrumInfo();
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      const auto &inQs = qWS->points(i);
      const auto &outPoints = outputWS->points(i);
      TS_ASSERT_EQUALS(outPoints.size(), inQs.size())
      TS_ASSERT(outputWS->hasDx(i))
      if (i != 1) {
        TS_ASSERT(!outputWS->spectrumInfo().isMonitor(i))
        const auto &outDx = outputWS->dx(i);
        TS_ASSERT_EQUALS(outDx.size(), inQs.size())
        const auto &lambdas = inputWS->points(i);
        const auto l2 = spectrumInfo.l2(i);
        const auto dirL2 = dirSpectrumInfo.l2(i);
        const auto angle_bragg = spectrumInfo.twoTheta(i) / 2.;
        for (size_t j = 0; j < lambdas.size(); ++j) {
          const auto lambda = lambdas[j] * 1e-10;
          const size_t qIndex = inQs.size() - j - 1;
          const auto q = inQs[qIndex];
          const auto resE = std::sqrt(pow<2>(err_res(lambda, l2)) +
                                      pow<2>(width_res(lambda, l2)));
          const auto detFwhm = det_fwhm(*inputWS, 0, 0);
          const auto dirDetFwhm = det_fwhm(*directWS, 0, 0);
          const auto omFwhm =
              om_fwhm(l2, dirL2, SLIT1_SIZE, SLIT2_SIZE, detFwhm, dirDetFwhm);
          const auto rayE =
              err_ray(l2, angle_bragg, sumType, polarized, omFwhm);
          const auto fractionalResolution =
              std::sqrt(pow<2>(resE) + pow<2>(rayE));
          TS_ASSERT_EQUALS(outPoints[qIndex], q)
          TS_ASSERT_DELTA(outDx[qIndex], q * fractionalResolution, 1e-7)
        }
      } else {
        // Monitor should have Dx = 0
        TS_ASSERT(outputWS->spectrumInfo().isMonitor(i))
        const auto &outDx = outputWS->dx(i);
        for (size_t j = 0; j < outDx.size(); ++j) {
          TS_ASSERT_EQUALS(outDx[j], 0.)
        }
      }
    }
  }

private:
  void sameReflectedAndDirectSlitSizes(const bool polarized,
                                       const std::string &sumType) {
    using namespace boost::math;
    auto inputWS = make_ws(0.5 / 180. * M_PI);
    inputWS->mutableY(0) = 1. / static_cast<double>(inputWS->y(0).size());
    API::MatrixWorkspace_sptr directWS = inputWS->clone();
    auto &run = directWS->mutableRun();
    constexpr bool overwrite{true};
    const std::string meters{"m"};
    run.addProperty("slit1.size", SLIT1_SIZE, meters, overwrite);
    run.addProperty("slit2.size", SLIT2_SIZE, meters, overwrite);
    auto alg = make_alg(inputWS, directWS, sumType, polarized);
    TS_ASSERT_THROWS_NOTHING(alg->execute();)
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
    const auto &dirSpectrumInfo = directWS->spectrumInfo();
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      const auto &inQs = qWS->points(i);
      const auto &outPoints = outputWS->points(i);
      TS_ASSERT_EQUALS(outPoints.size(), inQs.size())
      TS_ASSERT(outputWS->hasDx(i))
      if (i != 1) {
        TS_ASSERT(!outputWS->spectrumInfo().isMonitor(i))
        const auto &outDx = outputWS->dx(i);
        TS_ASSERT_EQUALS(outDx.size(), inQs.size())
        const auto &lambdas = inputWS->points(i);
        const auto l2 = spectrumInfo.l2(i);
        const auto dirL2 = dirSpectrumInfo.l2(i);
        const auto angle_bragg = spectrumInfo.twoTheta(i) / 2.;
        for (size_t j = 0; j < lambdas.size(); ++j) {
          const auto lambda = lambdas[j] * 1e-10;
          const size_t qIndex = inQs.size() - j - 1;
          const auto q = inQs[qIndex];
          const auto resE = std::sqrt(pow<2>(err_res(lambda, l2)) +
                                      pow<2>(width_res(lambda, l2)));
          const auto detFwhm = det_fwhm(*inputWS, 0, 0);
          const auto dirDetFwhm = det_fwhm(*directWS, 0, 0);
          const auto omFwhm =
              om_fwhm(l2, dirL2, SLIT1_SIZE, SLIT2_SIZE, detFwhm, dirDetFwhm);
          const auto rayE =
              err_ray(l2, angle_bragg, sumType, polarized, omFwhm);
          const auto fractionalResolution =
              std::sqrt(pow<2>(resE) + pow<2>(rayE));
          TS_ASSERT_EQUALS(outPoints[qIndex], q)
          TS_ASSERT_DELTA(outDx[qIndex], q * fractionalResolution, 1e-7)
        }
      } else {
        // Monitor should have Dx = 0
        TS_ASSERT(outputWS->spectrumInfo().isMonitor(i))
        const auto &outDx = outputWS->dx(i);
        for (size_t j = 0; j < outDx.size(); ++j) {
          TS_ASSERT_EQUALS(outDx[j], 0.)
        }
      }
    }
  }

  API::Algorithm_sptr make_alg(API::MatrixWorkspace_sptr inputWS,
                               API::MatrixWorkspace_sptr directWS,
                               const std::string &sumType,
                               const bool polarized) {
    std::vector<int> foreground(2);
    foreground.front() = 0;
    foreground.back() = 0;
    auto alg = boost::make_shared<Algorithms::ReflectometryMomentumTransfer>();
    alg->setChild(true);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->initialize())
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ReflectedBeamWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ReflectedForeground", foreground))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("DirectBeamWorkspace", directWS))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("DirectForeground", foreground))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SummationType", sumType))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Polarized", polarized))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("PixelSize", PIXEL_SIZE))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("DetectorResolution", DET_RESOLUTION))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("ChopperSpeed", CHOPPER_SPEED))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ChopperOpening", CHOPPER_OPENING_ANGLE))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("ChopperRadius", CHOPPER_RADIUS))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ChopperpairDistance", CHOPPER_GAP))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Slit1Name", "slit1"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("Slit1SizeSampleLog", "slit1.size"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Slit2Name", "slit2"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("Slit2SizeSampleLog", "slit2.size"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("TOFChannelWidth", TOF_BIN_WIDTH))
    return alg;
  }

  API::MatrixWorkspace_sptr make_ws(const double braggAngle) {
    using namespace WorkspaceCreationHelper;
    constexpr double startX{1000.};
    const Kernel::V3D sourcePos{0., 0., -L1};
    const Kernel::V3D &monitorPos = sourcePos;
    const Kernel::V3D samplePos{
        0., 0., 0.,
    };
    const auto detZ = DET_DIST * std::cos(2 * braggAngle);
    const auto detY = DET_DIST * std::sin(2 * braggAngle);
    const Kernel::V3D detectorPos{0., detY, detZ};
    const Kernel::V3D slit1Pos{0., 0., -SLIT1_DIST};
    const Kernel::V3D slit2Pos{0., 0., -SLIT2_DIST};
    constexpr int nHisto{2};
    constexpr int nBins{100};
    auto ws = create2DWorkspaceWithReflectometryInstrument(
        startX, slit1Pos, slit2Pos, SLIT1_SIZE, SLIT2_SIZE, sourcePos,
        monitorPos, samplePos, detectorPos, nHisto, nBins, TOF_BIN_WIDTH);
    // Add slit sizes to sample logs, too.
    auto &run = ws->mutableRun();
    constexpr bool overwrite{true};
    const std::string meters{"m"};
    run.addProperty("slit1.size", SLIT1_SIZE, meters, overwrite);
    run.addProperty("slit2.size", SLIT2_SIZE, meters, overwrite);
    auto alg =
        API::AlgorithmManager::Instance().createUnmanaged("ConvertUnits");
    alg->initialize();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", "_unused_for_child");
    alg->setProperty("Target", "Wavelength");
    alg->setProperty("EMode", "Elastic");
    alg->execute();
    return alg->getProperty("OutputWorkspace");
  }

  double det_fwhm(const API::MatrixWorkspace &ws, const size_t fgd_first,
                  const size_t fgd_last) {
    using namespace boost::math;
    std::vector<double> angd;
    const auto &spectrumInfo = ws.spectrumInfo();
    for (size_t i = fgd_first; i <= fgd_last; ++i) {
      if (spectrumInfo.isMonitor(i)) {
        continue;
      }
      const auto &ys = ws.y(i);
      const auto sum = std::accumulate(ys.cbegin(), ys.cend(), 0.0);
      angd.emplace_back(sum);
    }
    const auto temp = [&angd]() {
      double sum{0.0};
      for (size_t i = 0; i < angd.size(); ++i) {
        sum += static_cast<double>(i) * angd[i];
      }
      return sum;
    }();
    const auto total_angd = std::accumulate(angd.cbegin(), angd.cend(), 0.0);
    const auto pref = temp / total_angd + static_cast<double>(fgd_first);
    const auto angd_cen = pref - static_cast<double>(fgd_first);
    const auto tt = [&angd, &angd_cen]() {
      double sum{0.0};
      for (size_t i = 0; i < angd.size(); ++i) {
        sum += angd[i] * pow<2>(angd_cen - static_cast<double>(i));
      }
      return sum;
    }();
    return 2. * std::sqrt(2. * std::log(2.)) * PIXEL_SIZE *
           std::sqrt(tt / total_angd);
  }

  double err_ray(const double l2, const double angle_bragg,
                 const std::string &sumType, const bool polarized,
                 const double om_fwhm) {
    using namespace boost::math;
    const auto interslit = SLIT1_DIST - SLIT2_DIST;
    const auto da = 0.68 * std::sqrt((pow<2>(SLIT1_SIZE) + pow<2>(SLIT2_SIZE)) /
                                     pow<2>(interslit));
    const auto s2_fwhm = (0.68 * SLIT1_SIZE) / interslit;
    const auto s3_fwhm = (0.68 * SLIT2_SIZE) / (SLIT2_DIST + l2);
    double err_ray1;
    if (sumType == "SumInQ") {
      if (om_fwhm > 0) {
        if (s2_fwhm >= 2 * om_fwhm) {
          err_ray1 = std::sqrt(pow<2>(DET_RESOLUTION / l2) + pow<2>(s3_fwhm) +
                               pow<2>(om_fwhm)) /
                     angle_bragg;
        } else {
          err_ray1 = std::sqrt(pow<2>(DET_RESOLUTION / (2. * l2)) +
                               pow<2>(s3_fwhm) + pow<2>(s2_fwhm)) /
                     angle_bragg;
        }
      } else {
        if (s2_fwhm > DET_RESOLUTION / l2) {
          err_ray1 = std::sqrt(pow<2>(DET_RESOLUTION / l2) + pow<2>(s3_fwhm)) /
                     angle_bragg;
        } else {
          err_ray1 =
              std::sqrt(pow<2>(da) + pow<2>(DET_RESOLUTION / l2)) / angle_bragg;
        }
      }
    } else {
      if (polarized) {
        err_ray1 = std::sqrt(pow<2>(da)) / angle_bragg;
      } else {
        err_ray1 = std::sqrt(pow<2>(da) + pow<2>(om_fwhm)) / angle_bragg;
      }
    }
    const auto err_ray_temp =
        0.68 *
        std::sqrt((pow<2>(PIXEL_SIZE) + pow<2>(SLIT2_SIZE)) / pow<2>(l2)) /
        angle_bragg;
    return std::min(err_ray1, err_ray_temp);
  }

  double err_res(const double lambda, const double l2) {
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

  double om_fwhm(const double l2, const double dirl2, const double dirs2w,
                 const double dirs3w, const double det_fwhm,
                 const double detdb_fwhm) {
    using namespace boost::math;
    const double sdr = SLIT2_DIST + l2;
    const double ratio = SLIT2_SIZE / SLIT1_SIZE;
    const double interslit = SLIT1_DIST - SLIT2_DIST;
    const double vs = sdr + (ratio * interslit) / (1 + ratio);
    const double da = 0.68 * std::sqrt(pow<2>(SLIT1_SIZE) +
                                       pow<2>(SLIT2_SIZE) / pow<2>(interslit));
    const double da_det = std::sqrt(pow<2>(da * vs) + pow<2>(DET_RESOLUTION));
    double om_fwhm{0};
    if (std::abs(SLIT1_SIZE - dirs2w) >= 0.00004 ||
        std::abs(SLIT2_SIZE - dirs3w) >= 0.00004) {
      if ((det_fwhm - da_det) >= 0.) {
        if (std::sqrt(pow<2>(det_fwhm) - pow<2>(da_det)) >= PIXEL_SIZE) {
          om_fwhm = 0.5 * std::sqrt(pow<2>(det_fwhm) - pow<2>(da_det)) / dirl2;
        } else {
          om_fwhm = 0;
        }
      }
    } else {
      if (pow<2>(det_fwhm) - pow<2>(detdb_fwhm) >= 0.) {
        if (std::sqrt(pow<2>(det_fwhm) - pow<2>(detdb_fwhm)) >= PIXEL_SIZE) {
          om_fwhm =
              0.5 * std::sqrt(pow<2>(det_fwhm) - pow<2>(detdb_fwhm)) / dirl2;
        } else {
          om_fwhm = 0.;
        }
      } else {
        om_fwhm = 0.;
      }
    }
    return om_fwhm;
  }

  double width_res(const double lambda, const double l2) {
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
    m_reflectedWS = makeWS();
    m_directWS = m_reflectedWS->clone();
    m_algorithm = makeAlgorithm(m_reflectedWS, m_directWS);
  }

  void test_performance() {
    for (int i = 0; i < 1000; ++i)
      m_algorithm->execute();
  }

private:
  static API::IAlgorithm_sptr
  makeAlgorithm(API::MatrixWorkspace_sptr &reflectedWS,
                API::MatrixWorkspace_sptr &directWS) {
    std::vector<int> foreground(2);
    foreground.front() = 0;
    foreground.back() = 0;
    auto alg = boost::make_shared<Algorithms::ReflectometryMomentumTransfer>();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    alg->isInitialized();
    alg->setProperty("InputWorkspace", reflectedWS);
    alg->setPropertyValue("OutputWorkspace", "_unused_for_child");
    alg->setProperty("ReflectedBeamWorkspace", reflectedWS);
    alg->setProperty("ReflectedForeground", foreground);
    alg->setProperty("DirectBeamWorkspace", directWS);
    alg->setProperty("DirectForeground", foreground);
    alg->setProperty("SummationType", "SumInLambda");
    alg->setProperty("Polarized", false);
    alg->setProperty("PixelSize", PIXEL_SIZE);
    alg->setProperty("DetectorResolution", DET_RESOLUTION);
    alg->setProperty("ChopperSpeed", CHOPPER_SPEED);
    alg->setProperty("ChopperOpening", CHOPPER_OPENING_ANGLE);
    alg->setProperty("ChopperRadius", CHOPPER_RADIUS);
    alg->setProperty("ChopperpairDistance", CHOPPER_GAP);
    alg->setProperty("Slit1Name", "slit1");
    alg->setProperty("Slit1SizeSampleLog", "slit1.size");
    alg->setProperty("Slit2Name", "slit2");
    alg->setProperty("Slit2SizeSampleLog", "slit2.size");
    alg->setProperty("TOFChannelWidth", TOF_BIN_WIDTH);
    return alg;
  }

  static API::MatrixWorkspace_sptr makeWS() {
    using namespace WorkspaceCreationHelper;
    constexpr double startX{1000.};
    const Kernel::V3D sourcePos{0., 0., -L1};
    const Kernel::V3D &monitorPos = sourcePos;
    const Kernel::V3D samplePos{
        0., 0., 0.,
    };
    const double braggAngle{0.7};
    const auto detZ = DET_DIST * std::cos(2 * braggAngle);
    const auto detY = DET_DIST * std::sin(2 * braggAngle);
    const Kernel::V3D detectorPos{0., detY, detZ};
    const Kernel::V3D slit1Pos{0., 0., -SLIT1_DIST};
    const Kernel::V3D slit2Pos{0., 0., -SLIT2_DIST};
    constexpr int nHisto{2};
    constexpr int nBins{10000};
    auto ws = create2DWorkspaceWithReflectometryInstrument(
        startX, slit1Pos, slit2Pos, SLIT1_SIZE, SLIT2_SIZE, sourcePos,
        monitorPos, samplePos, detectorPos, nHisto, nBins, TOF_BIN_WIDTH);
    // Add slit sizes to sample logs, too.
    auto &run = ws->mutableRun();
    constexpr bool overwrite{true};
    const std::string meters{"m"};
    run.addProperty("slit1.size", SLIT1_SIZE, meters, overwrite);
    run.addProperty("slit2.size", SLIT2_SIZE, meters, overwrite);
    auto convertUnits =
        API::AlgorithmManager::Instance().createUnmanaged("ConvertUnits");
    convertUnits->initialize();
    convertUnits->setChild(true);
    convertUnits->setRethrows(true);
    convertUnits->setProperty("InputWorkspace", ws);
    convertUnits->setPropertyValue("OutputWorkspace", "_unused_for_child");
    convertUnits->setProperty("Target", "Wavelength");
    convertUnits->setProperty("EMode", "Elastic");
    convertUnits->execute();
    API::MatrixWorkspace_sptr outWS =
        convertUnits->getProperty("OutputWorkspace");
    return outWS;
  }

private:
  API::IAlgorithm_sptr m_algorithm;
  API::MatrixWorkspace_sptr m_directWS;
  API::MatrixWorkspace_sptr m_reflectedWS;
};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYMOMENTUMTRANSFERTEST_H_ */
