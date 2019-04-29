// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REFLECTOMETRYBEAMSTATISTICSTEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYBEAMSTATISTICSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometryBeamStatistics.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/math/special_functions/pow.hpp>

using namespace Mantid;

namespace {
constexpr double DET_DIST{4.};
constexpr double DET_RESOLUTION{0.002};
constexpr double L1{8.};
constexpr int N_DET{64};
constexpr int BEAM_CENTRE{N_DET / 2};
constexpr int FGD_FIRST{BEAM_CENTRE - 2};
constexpr int FGD_LAST{BEAM_CENTRE + 2};
constexpr double PIXEL_SIZE{0.0015};
// h / NeutronMass
constexpr double PLANCK_PER_KG{3.9560340102631226e-7};
constexpr double SLIT1_DIST{1.2};
constexpr double SLIT1_SIZE{0.03};
constexpr double SLIT2_DIST{0.3};
constexpr double SLIT2_SIZE{0.02};
constexpr double INTERSLIT{SLIT1_DIST - SLIT2_DIST};
constexpr double S2_FWHM{0.68 * SLIT1_SIZE / INTERSLIT};
constexpr double TOF_BIN_WIDTH{70.}; // microseconds
} // namespace

class ReflectometryBeamStatisticsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryBeamStatisticsTest *createSuite() {
    return new ReflectometryBeamStatisticsTest();
  }
  static void destroySuite(ReflectometryBeamStatisticsTest *suite) {
    delete suite;
  }

  void test_Init() {
    Algorithms::ReflectometryBeamStatistics alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_LogsGetAdded() {
    auto reflectedWS = makeWS(0.7);
    const std::vector<int> reflectedForeground{FGD_FIRST, BEAM_CENTRE,
                                               FGD_LAST};
    auto directWS = makeWS(0.);
    const std::vector<int> directForeground{FGD_FIRST, BEAM_CENTRE, FGD_LAST};
    Algorithms::ReflectometryBeamStatistics alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("ReflectedBeamWorkspace", reflectedWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("ReflectedForeground", reflectedForeground))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DirectLineWorkspace", directWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DirectForeground", directForeground))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PixelSize", PIXEL_SIZE))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorResolution", DET_RESOLUTION))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FirstSlitName", "slit1"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("FirstSlitSizeSampleLog", "slit1.size"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SecondSlitName", "slit2"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SecondSlitSizeSampleLog", "slit2.size"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    const auto &reflectedRun = reflectedWS->run();
    checkReflectedStatisticsContainedInSampleLogs(reflectedRun);
    const auto &directRun = directWS->run();
    checkDirectStatisticsContainedInSampleLogs(directRun);
    const auto reflectedDetFwhm = det_fwhm(*reflectedWS, FGD_FIRST, FGD_LAST);
    const auto directDetFwhm = det_fwhm(*directWS, FGD_FIRST, FGD_LAST);
    const auto omFwhm = om_fwhm(DET_DIST, DET_DIST, SLIT1_SIZE, SLIT2_SIZE,
                                reflectedDetFwhm, directDetFwhm);
    TS_ASSERT_EQUALS(reflectedRun.getPropertyValueAsType<double>(
                         "beam_stats.beam_rms_variation"),
                     reflectedDetFwhm)
    TS_ASSERT_EQUALS(directRun.getPropertyValueAsType<double>(
                         "beam_stats.beam_rms_variation"),
                     directDetFwhm)
    const auto bentSample =
        omFwhm > 0. && DET_RESOLUTION / DET_DIST > S2_FWHM ? 1 : 0;
    TS_ASSERT_EQUALS(
        reflectedRun.getPropertyValueAsType<int>("beam_stats.bent_sample"),
        bentSample)
    TS_ASSERT_EQUALS(reflectedRun.getPropertyValueAsType<double>(
                         "beam_stats.first_slit_angular_spread"),
                     S2_FWHM)
    const auto firstAngularSpread = da();
    TS_ASSERT_EQUALS(reflectedRun.getPropertyValueAsType<double>(
                         "beam_stats.incident_angular_spread"),
                     firstAngularSpread)
    TS_ASSERT_EQUALS(reflectedRun.getPropertyValueAsType<double>(
                         "beam_stats.sample_waviness"),
                     omFwhm)
    const auto secondAngularSpread = s3_fwhm(DET_DIST);
    TS_ASSERT_EQUALS(reflectedRun.getPropertyValueAsType<double>(
                         "beam_stats.second_slit_angular_spread"),
                     secondAngularSpread)
  }

  void test_failsGracefullyWhenSlitsNotFound() {
    constexpr int firstSlit{1};
    checkWrongSlitsThrows(firstSlit);
    constexpr int secondSlit{2};
    checkWrongSlitsThrows(secondSlit);
  }

private:
  static void checkDirectStatisticsContainedInSampleLogs(const API::Run &run) {
    TS_ASSERT(run.hasProperty("beam_stats.beam_rms_variation"))
    TS_ASSERT_EQUALS(run.getProperty("beam_stats.beam_rms_variation")->units(),
                     "m")
    TS_ASSERT(!run.hasProperty("beam_stats.bent_sample"))
    TS_ASSERT(!run.hasProperty("beam_stats.first_slit_angular_spread"))
    TS_ASSERT(!run.hasProperty("beam_stats.incident_angular_spread"))
    TS_ASSERT(!run.hasProperty("beam_stats.sample_waviness"))
    TS_ASSERT(!run.hasProperty("beam_stats.second_slit_angular_spread"))
  }
  static void
  checkReflectedStatisticsContainedInSampleLogs(const API::Run &run) {
    TS_ASSERT(run.hasProperty("beam_stats.beam_rms_variation"))
    TS_ASSERT_EQUALS(run.getProperty("beam_stats.beam_rms_variation")->units(),
                     "m")
    TS_ASSERT(run.hasProperty("beam_stats.bent_sample"))
    TS_ASSERT_EQUALS(run.getProperty("beam_stats.bent_sample")->units(), "")
    TS_ASSERT(run.hasProperty("beam_stats.first_slit_angular_spread"))
    TS_ASSERT_EQUALS(
        run.getProperty("beam_stats.first_slit_angular_spread")->units(),
        "radians")
    TS_ASSERT(run.hasProperty("beam_stats.incident_angular_spread"))
    TS_ASSERT_EQUALS(
        run.getProperty("beam_stats.incident_angular_spread")->units(),
        "radians")
    TS_ASSERT(run.hasProperty("beam_stats.sample_waviness"))
    TS_ASSERT_EQUALS(run.getProperty("beam_stats.sample_waviness")->units(),
                     "radians")
    TS_ASSERT(run.hasProperty("beam_stats.second_slit_angular_spread"))
    TS_ASSERT_EQUALS(
        run.getProperty("beam_stats.second_slit_angular_spread")->units(),
        "radians")
  }

  void checkWrongSlitsThrows(const int slit) {
    const std::string slit1 = slit == 1 ? "non-existent" : "slit1";
    const std::string slit2 = slit == 2 ? "non-existent" : "slit2";
    auto reflectedWS = makeWS(0.7);
    const std::vector<int> reflectedForeground{FGD_FIRST, BEAM_CENTRE,
                                               FGD_LAST};
    auto directWS = makeWS(0.);
    const std::vector<int> directForeground{FGD_FIRST, BEAM_CENTRE, FGD_LAST};
    Algorithms::ReflectometryBeamStatistics alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("ReflectedBeamWorkspace", reflectedWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("ReflectedForeground", reflectedForeground))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DirectLineWorkspace", directWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DirectForeground", directForeground))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PixelSize", PIXEL_SIZE))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorResolution", DET_RESOLUTION))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FirstSlitName", slit1))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("FirstSlitSizeSampleLog", "slit1.size"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SecondSlitName", slit2))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SecondSlitSizeSampleLog", "slit2.size"))
    TS_ASSERT_THROWS_EQUALS(alg.execute(), std::runtime_error & e, e.what(),
                            std::string("Some invalid Properties found"))
    TS_ASSERT(!alg.isExecuted())
  }

  static API::MatrixWorkspace_sptr makeWS(const double braggAngle) {
    using namespace WorkspaceCreationHelper;
    using Geometry::deg2rad;
    constexpr double startX{1000.};
    const Kernel::V3D sourcePos{0., 0., -L1};
    const Kernel::V3D &monitorPos = sourcePos;
    const Kernel::V3D samplePos{0., 0., 0.};
    const auto detZ = DET_DIST * std::cos(2 * braggAngle * deg2rad);
    const auto detY = DET_DIST * std::sin(2 * braggAngle * deg2rad);
    const Kernel::V3D detectorPos{0., detY, detZ};
    const Kernel::V3D slit1Pos{0., 0., -SLIT1_DIST};
    const Kernel::V3D slit2Pos{0., 0., -SLIT2_DIST};
    constexpr int nBins{100};
    auto ws = create2DWorkspaceWithReflectometryInstrumentMultiDetector(
        startX, PIXEL_SIZE, slit1Pos, slit2Pos, SLIT1_SIZE, SLIT2_SIZE,
        sourcePos, monitorPos, samplePos, detectorPos, N_DET, nBins,
        TOF_BIN_WIDTH);
    // Add slit sizes to sample logs, too.
    auto &run = ws->mutableRun();
    constexpr bool overwrite{true};
    const std::string meters{"m"};
    run.addProperty("slit1.size", SLIT1_SIZE, meters, overwrite);
    run.addProperty("slit2.size", SLIT2_SIZE, meters, overwrite);
    // Build a step-like peak in the middle of the detector.
    auto zeros = Kernel::make_cow<HistogramData::HistogramY>(nBins, 0.);
    auto zeroErrors = Kernel::make_cow<HistogramData::HistogramE>(nBins, 0.);
    auto peak = Kernel::make_cow<HistogramData::HistogramY>(nBins, 10.);
    auto peakErrors =
        Kernel::make_cow<HistogramData::HistogramE>(nBins, std::sqrt(10.));
    for (size_t i = 0; i < N_DET; ++i) {
      if (i >= FGD_FIRST && i <= FGD_LAST) {
        ws->setSharedY(i, peak);
        ws->setSharedE(i, peakErrors);
      } else {
        ws->setSharedY(i, zeros);
        ws->setSharedE(i, zeroErrors);
      }
    }
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

  static double det_fwhm(const API::MatrixWorkspace &ws, const size_t fgd_first,
                         const size_t fgd_last) {
    // This function comes from COSMOS.
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

  static double s3_fwhm(const double l2) {
    // This function comes from COSMOS.
    return 0.68 * SLIT2_SIZE / (SLIT2_DIST + l2);
  }

  static double err_ray(const double l2, const double angle_bragg,
                        const std::string &sumType, const bool polarized,
                        const double om_fwhm) {
    // This function comes from COSMOS.
    using namespace boost::math;
    double err_ray1;
    if (sumType == "SumInQ") {
      if (om_fwhm > 0) {
        if (S2_FWHM >= 2 * om_fwhm) {
          err_ray1 = std::sqrt(pow<2>(DET_RESOLUTION / l2) +
                               pow<2>(s3_fwhm(l2)) + pow<2>(om_fwhm)) /
                     angle_bragg;
        } else {
          err_ray1 = std::sqrt(pow<2>(DET_RESOLUTION / (2. * l2)) +
                               pow<2>(s3_fwhm(l2)) + pow<2>(S2_FWHM)) /
                     angle_bragg;
        }
      } else {
        if (S2_FWHM > DET_RESOLUTION / l2) {
          err_ray1 =
              std::sqrt(pow<2>(DET_RESOLUTION / l2) + pow<2>(s3_fwhm(l2))) /
              angle_bragg;
        } else {
          err_ray1 = std::sqrt(pow<2>(da()) + pow<2>(DET_RESOLUTION / l2)) /
                     angle_bragg;
        }
      }
    } else {
      if (polarized) {
        err_ray1 = std::sqrt(pow<2>(da())) / angle_bragg;
      } else {
        err_ray1 = std::sqrt(pow<2>(da()) + pow<2>(om_fwhm)) / angle_bragg;
      }
    }
    const auto err_ray_temp =
        0.68 *
        std::sqrt((pow<2>(PIXEL_SIZE) + pow<2>(SLIT2_SIZE)) / pow<2>(l2)) /
        angle_bragg;
    return std::min(err_ray1, err_ray_temp);
  }

  static double da() {
    // This function comes from COSMOS.
    using namespace boost::math;
    return 0.68 * std::sqrt((pow<2>(SLIT1_SIZE) + pow<2>(SLIT2_SIZE)) /
                            pow<2>(INTERSLIT));
  }

  static double om_fwhm(const double l2, const double dirl2,
                        const double dirs2w, const double dirs3w,
                        const double det_fwhm, const double detdb_fwhm) {
    // This function comes from COSMOS.
    using namespace boost::math;
    const double sdr = SLIT2_DIST + l2;
    const double ratio = SLIT2_SIZE / SLIT1_SIZE;
    const double vs = sdr + (ratio * INTERSLIT) / (1 + ratio);
    const double da_det = std::sqrt(pow<2>(da() * vs) + pow<2>(DET_RESOLUTION));
    double om_fwhm{0.};
    if ((std::abs(SLIT1_SIZE - dirs2w) >= 0.00004 ||
         std::abs(SLIT2_SIZE - dirs3w) >= 0.00004) &&
        (det_fwhm - da_det >= 0.) &&
        (std::sqrt(pow<2>(det_fwhm) - pow<2>(da_det)) >= PIXEL_SIZE)) {
      om_fwhm = 0.5 * std::sqrt(pow<2>(det_fwhm) - pow<2>(da_det)) / dirl2;
    } else if ((pow<2>(det_fwhm) - pow<2>(detdb_fwhm) >= 0.) &&
               (std::sqrt(pow<2>(det_fwhm) - pow<2>(detdb_fwhm)) >=
                PIXEL_SIZE)) {
      om_fwhm = 0.5 * std::sqrt(pow<2>(det_fwhm) - pow<2>(detdb_fwhm)) / dirl2;
    }
    return om_fwhm;
  }
};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYBEAMSTATISTICSTEST_H_ */
