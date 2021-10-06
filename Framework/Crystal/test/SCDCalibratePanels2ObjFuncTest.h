// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

// DEVNOTE:
//  - cos, sin func uses radians
//  - Quat class uses degrees
//  - The overall strategy here is that the correct answer is always the engineering position,
//    and we are moving the insturment to the wrong location (i.e. needs calibration) so that
//    the calibration can move it back to the correct position

#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/SCDCalibratePanels2.h"
#include "MantidCrystal/SCDCalibratePanels2ObjFunc.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Unit.h"

#include <boost/filesystem.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/round.hpp>
#include <cxxtest/TestSuite.h>
#include <stdexcept>

using namespace Mantid::API;
using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
/// static logger
Logger g_log("SCDCalibratePanels2ObjFuncTest");
} // namespace

class SCDCalibratePanels2ObjFuncTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SCDCalibratePanels2ObjFuncTest *createSuite() { return new SCDCalibratePanels2ObjFuncTest(); }
  static void destroySuite(SCDCalibratePanels2ObjFuncTest *suite) { delete suite; }

  // ----------------- //
  // ----- Setup ----- //
  // ----------------- //
  /**
   * @brief Construct a new SCDCalibratePanels2Test object
   *
   */
  SCDCalibratePanels2ObjFuncTest()
      : wsname("wsSCDCalibratePanels2ObjFuncTest"), pwsname("pwsSCDCalibratePanels2ObjFuncTest"),
        tmppwsname("tmppwsSCDCalibratePanels2ObjFuncTest"),     // fixed workspace name
        silicon_a(5.431), silicon_b(5.431), silicon_c(5.431),   // angstrom
        silicon_alpha(90), silicon_beta(90), silicon_gamma(90), // degree
        silicon_cs(CrystalStructure("5.431 5.431 5.431", "F d -3 m", "Si 0 0 0 1.0 0.02")), dspacing_min(1.0),
        dspacing_max(10.0),                      //
        wavelength_min(0.1), wavelength_max(10), //
        omega_step(6.0),                         //
        TOLERANCE_L(1e-3),                       // this calibration has intrinsic accuracy limit of
                                                 // 1 mm for translation on a panel detector
        TOLERANCE_R(1e-1),                       // this calibration has intrinsic accuracy limit of
                                                 // 0.1 deg for rotation on a panel detector
        LOGCHILDALG(false) {

    // NOTE:
    // PredictPeaks
    //     m_pws = generateSimulatedPeaksWorkspace(m_ws);
    // takes way too long, use the pre-generated one
    std::shared_ptr<Algorithm> loadalg = AlgorithmFactory::Instance().create("Load", 1);
    loadalg->initialize();
    loadalg->setProperty("Filename", "PwsTOPAZIDeal.nxs");
    loadalg->setProperty("OutputWorkspace", "mpws");
    loadalg->execute();
    m_pws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>("mpws");
  }

  /**
   * @brief test object function with rotation and shift
   */
  void test_rot_shift() {
    g_log.notice() << "test_rot_shift() starts.\n";

    // Generate unique temp files
    auto filenamebase = boost::filesystem::temp_directory_path();
    filenamebase /= boost::filesystem::unique_path("testBank_%%%%%%%%");
    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();
    Mantid::API::IPeaksWorkspace_sptr ipws = std::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(pws);

    // Move one bank to the wrong location
    // NOTE: the common range for dx, dy ,dz is +-5cm
    const std::string bankname = "bank27";
    double dx = 1.1e-3;
    double dy = -0.9e-3;
    double dz = 1.5e-3;
    // prescribed rotation
    double theta = PI / 3;
    double phi = PI / 8;
    double rvx = sin(theta) * cos(phi);
    double rvy = sin(theta) * sin(phi);
    double rvz = cos(theta);
    double ang = 0.02; // degrees

    // Adjust bank but really doing nothing
    adjustComponent(dx, dy, dz, rvx, rvy, rvz, ang, bankname, pws);

    // capture TOF
    std::vector<double> tofs;
    for (int i = 0; i < pws->getNumberPeaks(); ++i) {
      tofs.emplace_back(pws->getPeak(i).getTOF());
    }

    // Init and set up function
    SCDCalibratePanels2ObjFunc testfunc;
    testfunc.initialize();
    testfunc.setPeakWorkspace(ipws, bankname, tofs);

    std::cout << "Name: " << testfunc.name() << "\n";

    const int n_peaks = pws->getNumberPeaks();
    TS_ASSERT_EQUALS(n_peaks, 11076);

    std::unique_ptr<double[]> out(new double[n_peaks * 3]);

    // useless parameters
    double useless[5];
    size_t order(1000);

    // calcualte function value
    testfunc.function1D(out.get(), useless, order);

    std::vector<double> golderrorstart{2.69624,  1.91549, -4.81046, 3.37157, 1.91529,
                                       -4.80595, 2.02136, 1.91584,  -4.12417};
    for (size_t i = 0; i < 9; ++i)
      TS_ASSERT_DELTA(out[i], golderrorstart[i], 0.00001);
    std::vector<double> golderrorend{3.36926, -3.83431, 2.06252,  3.71041, -2.87334,
                                     2.40752, 4.04829,  -3.82838, 2.74896};
    for (size_t i = 33219; i < 33228; ++i)
      TS_ASSERT_DELTA(out[i], golderrorend[i - 33219], 0.00001);

    //    // Peak indexes
    //    std::vector<int> peakindexes{64,    65,    66,    67,    254,   255,   256,   257,   10955, 10956, 10957,
    //    10958}; std::vector<long int> detids{1780254, 1800379, 1814619, 1790397, 1811588, 1825313, 1788132, 1801093,
    //    1803923, 1788915, 1771352, 1824577}; for (size_t i = 0; i < peakindexes.size(); ++i) {
    //        // Assert detector IDs
    //        int ipeak = peakindexes[i];
    //        long int det_i = pws->getPeak(ipeak).getDetectorID();
    //        TS_ASSERT_EQUALS(det_i, detids[i]);

    //        std::cout << ipeak << ": ";
    //        for (size_t d = 0; d < 3; ++d) {
    //            std::cout << out[ipeak * 3 + d] << "  ";
    //        }
    //        std::cout << "\n";
    //    }
    //    TS_ASSERT_EQUALS(121, 212);
    return;
  }

  void test_detector_resize() {
    g_log.notice() << "test_Bank() starts.\n";

    // Generate unique temp files
    auto filenamebase = boost::filesystem::temp_directory_path();
    filenamebase /= boost::filesystem::unique_path("testBank_%%%%%%%%");
    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();
    Mantid::API::IPeaksWorkspace_sptr ipws = std::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(pws);

    // Move one bank to the wrong location
    // NOTE: the common range for dx, dy ,dz is +-5cm
    const std::string bankname = "bank27";
    double dx = 0.;
    double dy = 0.;
    double dz = 0.;
    // prescribed rotation
    double theta = PI / 3;
    double phi = PI / 8;
    double rvx = sin(theta) * cos(phi);
    double rvy = sin(theta) * sin(phi);
    double rvz = cos(theta);
    double ang = 0.02; // degrees
    //
    adjustComponent(dx, dy, dz, rvx, rvy, rvz, ang, bankname, pws);
    // capture TOF
    std::vector<double> tofs;
    for (int i = 0; i < pws->getNumberPeaks(); ++i) {
      tofs.emplace_back(pws->getPeak(i).getTOF());
    }

    // Init and set up function
    SCDCalibratePanels2ObjFunc testfunc;
    testfunc.initialize();
    testfunc.setPeakWorkspace(ipws, bankname, tofs);

    std::cout << "Name: " << testfunc.name() << "\n";

    const int n_peaks = pws->getNumberPeaks();
    TS_ASSERT_EQUALS(n_peaks, 11076);

    std::unique_ptr<double[]> out(new double[n_peaks * 3]);

    // useless parameters
    double useless[5];
    size_t order(1000);

    // calcualte function value
    testfunc.function1D(out.get(), useless, order);

    // Peak indexes
    std::vector<int> peakindexes{64, 65, 66, 67, 254, 255, 256, 257, 10955, 10956, 10957, 10958};
    std::vector<long int> detids{1780254, 1800379, 1814619, 1790397, 1811588, 1825313,
                                 1788132, 1801093, 1803923, 1788915, 1771352, 1824577};
    for (size_t i = 0; i < peakindexes.size(); ++i) {
      // Assert detector IDs
      int ipeak = peakindexes[i];
      long int det_i = pws->getPeak(ipeak).getDetectorID();
      TS_ASSERT_EQUALS(det_i, detids[i]);

      std::cout << ipeak << ": ";
      for (size_t d = 0; d < 3; ++d) {
        std::cout << out[ipeak * 3 + d] << "  ";
      }
      std::cout << "\n";
    }

    // Set up function parameters values
    testfunc.setParameter("ScaleX", 1.1);
    testfunc.setParameter("ScaleY", 0.9);
    // calcualte function value for bank27 (related to the object function)
    testfunc.function1D(out.get(), useless, order);

    std::cout << "Scale (1.1, 0.9)\n";
    for (size_t i = 0; i < peakindexes.size(); ++i) {
      // Assert detector IDs
      int ipeak = peakindexes[i];
      long int det_i = pws->getPeak(ipeak).getDetectorID();
      TS_ASSERT_EQUALS(det_i, detids[i]);

      std::cout << ipeak << ": ";
      for (size_t d = 0; d < 3; ++d) {
        std::cout << out[ipeak * 3 + d] << "  ";
      }
      std::cout << "\n";
    }

    std::cout << "Scale (0.9, 1.1)\n";
    testfunc.setParameter("ScaleX", 0.9);
    testfunc.setParameter("ScaleY", 1.1);
    // calcualte function value for bank27 (related to the object function)
    testfunc.function1D(out.get(), useless, order);

    // show result
    // Peak indexes
    //    std::vector<int> peakindexes{64, 65, 66, 67, 254, 255, 256, 257, 10955, 10956, 10957, 10958};
    //    std::vector<long int> detids{1780254, 1800379, 1814619, 1790397, 1811588, 1825313,
    //                                 1788132, 1801093, 1803923, 1788915, 1771352, 1824577};
    for (size_t i = 0; i < peakindexes.size(); ++i) {
      // Assert detector IDs
      int ipeak = peakindexes[i];
      long int det_i = pws->getPeak(ipeak).getDetectorID();
      TS_ASSERT_EQUALS(det_i, detids[i]);

      std::cout << ipeak << ": ";
      for (size_t d = 0; d < 3; ++d) {
        std::cout << out[ipeak * 3 + d] << "  ";
      }
      std::cout << "\n";
    }
    TS_ASSERT_EQUALS(135, 246);

    // TODO - write these into unit tests!

    /*
     * Scale (1.1, 0.9)
64: -4.04172  1.91418  -4.04384
65: -4.37725  0.988898  -3.80048
66: -3.71589  0.927614  -3.78026
67: -3.69851  0.996146  -3.09986
254: -4.38616  0.944574  -3.79285
255: -3.72545  0.896009  -3.77312
256: -4.37156  1.02298  -3.11404
257: -3.70605  0.964197  -3.09218
10955: 3.70692  0.961802  3.78894
10956: 4.36969  1.03149  3.80559
10957: 4.02371  1.94577  4.05188
10958: 3.72025  0.903004  4.4747
Scale (0.9, 1.1)
64: -4.04381  1.92244  -4.20213
65: -4.38515  0.930843  -3.75479
66: -3.69941  0.989064  -3.77225
67: -3.71802  0.91721  -3.08782
254: -4.37735  0.970865  -3.75927
255: -3.68901  1.02111  -3.77964
256: -4.39288  0.892533  -3.0754
257: -3.70793  0.957278  -3.09125
10955: 3.70922  0.951834  3.76849
10956: 4.39379  0.878827  3.7513
10957: 4.06492  1.88495  4.1947
10958: 3.6933  1.01796  4.45579
     *
     *
     */

    //      for (int i = 0; i < n_peaks; ++i) {
    //          // search bank27
    //          if (1769472 <= pws->getPeak(i).getDetectorID() && pws->getPeak(i).getDetectorID() < 1835008) {
    //             std::cout << i << ": " << pws->getPeak(i).getDetectorID() << "\n";
    //          }
    //      }

    // TS_ASSERT_EQUALS(1, 123321123);
  }

  // NOTE: skipped to prevent time out on build server
  void NotUsed_run_Exec() {
    // Shall not run this now
    TS_ASSERT_EQUALS(1, 12321);

    // Generate unique temp files
    auto filenamebase = boost::filesystem::temp_directory_path();
    filenamebase /= boost::filesystem::unique_path("testExec_%%%%%%%%");
    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();

    // Adjust L1 and banks
    //-- source
    const double dL1 = boost::math::constants::e<double>() / 100;
    //-- bank27
    const std::string bank27 = "bank27";
    double dx1 = 1.1e-3;
    double dy1 = -0.9e-3;
    double dz1 = 1.5e-3;
    double theta1 = PI / 3;
    double phi1 = PI / 8;
    double rvx1 = sin(theta1) * cos(phi1);
    double rvy1 = sin(theta1) * sin(phi1);
    double rvz1 = cos(theta1);
    double ang1 = 0.01; // degrees
    //-- bank16
    const std::string bank16 = "bank16";
    double dx2 = 0.5e-3;
    double dy2 = 1.3e-3;
    double dz2 = -1.9e-3;
    double theta2 = PI / 4;
    double phi2 = PI / 3;
    double rvx2 = sin(theta2) * cos(phi2);
    double rvy2 = sin(theta2) * sin(phi2);
    double rvz2 = cos(theta2);
    double ang2 = 0.01; // degrees

    // source
    adjustComponent(0.0, 0.0, dL1, 1.0, 0.0, 0.0, 0.0, pws->getInstrument()->getSource()->getName(), pws);
    // bank27
    adjustComponent(dx1, dy1, dz1, rvx1, rvy1, rvz1, ang1, bank27, pws);
    // bank16
    adjustComponent(dx2, dy2, dz2, rvx2, rvy2, rvz2, ang2, bank16, pws);
  }

private:
  /**
   * @brief Adjust the position of a component through translation and rotation
   *
   * @param dx
   * @param dy
   * @param dz
   * @param rvx  x-component of rotation axis
   * @param rvy  y-component of rotation axis
   * @param rvz  z-component of rotation axis
   * @param drotang  rotation angle
   * @param cmptName
   * @param ws
   */
  void adjustComponent(double dx, double dy, double dz, double rvx, double rvy, double rvz, double drotang,
                       std::string cmptName, PeaksWorkspace_sptr &pws) {

    // rotation
    auto rot_alg = Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
    rot_alg->initialize();
    rot_alg->setLogging(LOGCHILDALG);
    rot_alg->setProperty("Workspace", pws);
    rot_alg->setProperty("ComponentName", cmptName);
    rot_alg->setProperty("X", rvx);
    rot_alg->setProperty("Y", rvy);
    rot_alg->setProperty("Z", rvz);
    rot_alg->setProperty("Angle", drotang);
    rot_alg->setProperty("RelativeRotation", true);
    rot_alg->execute();

    // translation
    auto mv_alg = Mantid::API::AlgorithmFactory::Instance().create("MoveInstrumentComponent", -1);
    mv_alg->initialize();
    mv_alg->setLogging(LOGCHILDALG);
    mv_alg->setProperty("Workspace", pws);
    mv_alg->setProperty("ComponentName", cmptName);
    mv_alg->setProperty("X", dx);
    mv_alg->setProperty("Y", dy);
    mv_alg->setProperty("Z", dz);
    mv_alg->setProperty("RelativePosition", true);
    mv_alg->execute();
  }

  /**
   * @brief remove all workspace memory after one test is done
   *
   */
  void doCleanup() {
    AnalysisDataService::Instance().remove(pwsname);
    AnalysisDataService::Instance().remove(tmppwsname);
  }

  // ------------------- //
  // ----- members ----- //
  // ------------------- //
  // workspace names
  const std::string wsname;
  const std::string pwsname;
  const std::string tmppwsname;

  MatrixWorkspace_sptr m_ws;
  PeaksWorkspace_sptr m_pws;

  // lattice constants of silicon
  const double silicon_a;
  const double silicon_b;
  const double silicon_c;
  const double silicon_alpha;
  const double silicon_beta;
  const double silicon_gamma;

  // silicon crystal structure
  CrystalStructure silicon_cs;

  // constants that select the recriprocal space
  const double dspacing_min;
  const double dspacing_max;
  const double wavelength_min;
  const double wavelength_max;
  const double omega_step;

  // check praramerter
  const double TOLERANCE_L; // distance
  const double TOLERANCE_R; // rotation angle
  const bool LOGCHILDALG;   // whether to show individual alg log
  const double PI{3.1415926535897932384626433832795028841971693993751058209};
};
