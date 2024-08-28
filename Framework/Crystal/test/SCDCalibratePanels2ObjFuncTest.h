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
        tmppwsname("tmppwsSCDCalibratePanels2ObjFuncTest"), // fixed workspace name
        silicon_cs(CrystalStructure("5.431 5.431 5.431", "F d -3 m", "Si 0 0 0 1.0 0.02")), LOGCHILDALG(false) {

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
  }

  void test_detector_resize() {
    g_log.notice() << "test_Bank() starts.\n";

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
    }

    // Calculate value with scaling (1.1, 0.9) on  bank27
    testfunc.setParameter("ScaleX", 1.1);
    testfunc.setParameter("ScaleY", 0.9);
    testfunc.function1D(out.get(), useless, order);
    // verify values
    std::vector<double> goldvalue1{-4.04172, 1.914180, -4.04384, -4.37725, 0.988898, -3.80048, -3.71589, 0.927614,
                                   -3.78026, -3.69851, 0.996146, -3.09986, -4.38616, 0.944574, -3.79285, -3.72545,
                                   0.896009, -3.77312, -4.37156, 1.022980, -3.11404, -3.70605, 0.964197, -3.09218,
                                   3.70692,  0.961802, 3.78894,  4.36969,  1.031490, 3.80559,  4.02371,  1.945770,
                                   4.05188,  3.72025,  0.903004, 4.47470};
    for (size_t i = 0; i < peakindexes.size(); ++i) {
      // Assert detector IDs
      int ipeak = peakindexes[i];
      long int det_i = pws->getPeak(ipeak).getDetectorID();
      TS_ASSERT_EQUALS(det_i, detids[i]);

      for (size_t d = 0; d < 3; ++d) {
        TS_ASSERT_DELTA(out[ipeak * 3 + d], goldvalue1[i * 3 + d], 0.00001);
      }
    }

    // Evalulate with detector of bank27 scaled to (0.9, 1.1)
    testfunc.setParameter("ScaleX", 0.9);
    testfunc.setParameter("ScaleY", 1.1);
    testfunc.function1D(out.get(), useless, order);
    // verify values
    std::vector<double> goldvalue2{-4.04381, 1.922440, -4.20213, -4.38515, 0.930843, -3.75479, -3.69941, 0.989064,
                                   -3.77225, -3.71802, 0.917210, -3.08782, -4.37735, 0.970865, -3.75927, -3.68901,
                                   1.021110, -3.77964, -4.39288, 0.892533, -3.07540, -3.70793, 0.957278, -3.09125,
                                   3.70922,  0.951834, 3.76849,  4.39379,  0.878827, 3.75130,  4.06492,  1.884950,
                                   4.19470,  3.69330,  1.017960, 4.45579};
    for (size_t i = 0; i < peakindexes.size(); ++i) {
      // Assert detector IDs
      int ipeak = peakindexes[i];
      long int det_i = pws->getPeak(ipeak).getDetectorID();
      TS_ASSERT_EQUALS(det_i, detids[i]);

      for (size_t d = 0; d < 3; ++d) {
        TS_ASSERT_DELTA(out[ipeak * 3 + d], goldvalue2[i * 3 + d], 0.00001);
      }
    }
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

  // silicon crystal structure
  CrystalStructure silicon_cs;

  // check praramerter
  const bool LOGCHILDALG; // whether to show individual alg log
  const double PI{3.1415926535897932384626433832795028841971693993751058209};
};
