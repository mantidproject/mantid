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
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/SCDCalibratePanels2.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Unit.h"

#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/round.hpp>
#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <stdexcept>

using namespace Mantid::API;
using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::Kernel::Strings::randomString;

namespace {
/// static logger
Logger g_log("SCDCalibratePanels2Test");

std::string tempfilename() {
  const auto filename = "testL1_" + randomString(8);
  auto filepath = std::filesystem::temp_directory_path() / filename;

  return filepath.string();
}
} // namespace

class SCDCalibratePanels2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SCDCalibratePanels2Test *createSuite() { return new SCDCalibratePanels2Test(); }
  static void destroySuite(SCDCalibratePanels2Test *suite) { delete suite; }

  // ----------------- //
  // ----- Setup ----- //
  // ----------------- //
  /**
   * @brief Construct a new SCDCalibratePanels2Test object
   *
   */
  SCDCalibratePanels2Test()
      : wsname("wsSCDCalibratePanels2Test"), pwsname("pwsSCDCalibratePanels2Test"),
        tmppwsname("tmppwsSCDCalibratePanels2Test"),            // fixed workspace name
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
    //  The MAGIC PIECE, basically we need to let AlgorithmFactory
    //  to load a non-related algorithm, then somehow AlgorithmFactory
    //  can find the Fit algorithm for the remaining test
    std::shared_ptr<Algorithm> darkmagic = AlgorithmFactory::Instance().create("LoadIsawPeaks", 1);
    darkmagic->initialize();
    darkmagic->setLogging(false); // don't really care its output
    darkmagic->setPropertyValue("Filename", "Peaks5637.integrate");
    darkmagic->setPropertyValue("OutputWorkspace", "TOPAZ_5637");
    darkmagic->executeAsChildAlg();

    // NOTE:
    // PredictPeaks
    //     m_pws = generateSimulatedPeaksWorkspace(m_ws);
    // takes way too long, use the pre-generated one
    std::shared_ptr<Algorithm> loadalg = AlgorithmFactory::Instance().create("Load", 1);
    loadalg->initialize();
    loadalg->setProperty("Filename", "PwsTOPAZIDeal.nxs");
    loadalg->setProperty("OutputWorkspace", "mpws");
    loadalg->execute();
    // NOTE:
    // somehow
    //    loadalg->getProperty("OutputWorkspace")
    // will return a nullptr here, so we need to rely on ADS to retrieve the actual pws.
    m_pws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>("mpws");
  }

  // ---------------------- //
  // ----- Unit Tests ----- //
  // ---------------------- //

  /**
   * @brief test name
   *
   */
  void test_Name() {
    SCDCalibratePanels2 alg;
    TS_ASSERT_EQUALS(alg.name(), "SCDCalibratePanels");
  }

  /**
   * @brief test init
   *
   */
  void test_Init() {
    SCDCalibratePanels2 alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  /**
   * @brief Trivial case where all components are in ideal/starting position
   *        Therefore the calibration results should be close to a zero
   *        vector.
   */
  void run_Null_Case() {
    g_log.notice() << "test: !Null case!\n";
    // Generate unique temp files
    const auto filenamebase = tempfilename();

    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();

    // No need to change anything

    // Run the calibration
    bool calibrateL1 = true;
    bool calibrateBanks = false;
    bool calibrateT0 = false;
    bool tuneSamplePos = false;
    runCalibration(filenamebase, pws, calibrateL1, calibrateBanks, calibrateT0, tuneSamplePos);

    // Apply the calibration results
    MatrixWorkspace_sptr ws = generateSimulatedWorkspace();
    const std::string xmlFileName = filenamebase + ".xml";
    auto lpf_alg = AlgorithmFactory::Instance().create("LoadParameterFile", 1);
    lpf_alg->initialize();
    lpf_alg->setLogging(LOGCHILDALG);
    lpf_alg->setProperty("Workspace", ws);
    lpf_alg->setProperty("Filename", xmlFileName);
    lpf_alg->execute();

    // Checking L1 since it is the only thing we calibrated
    double L1_ref = m_pws->getInstrument()->getSource()->getPos().Z();
    double L1_cali = ws->getInstrument()->getSource()->getPos().Z();
    TS_ASSERT_DELTA(L1_cali, L1_ref, TOLERANCE_L);
    g_log.notice() << "L1_ref = " << L1_ref << "\n"
                   << "L1_cali = " << L1_cali << "\n";
  }

  void run_L1() {
    g_log.notice() << "test_L1() starts.\n";
    // Generate unique temp files
    const auto filenamebase = tempfilename();
    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();

    // Shift L1 to a "wrong" state
    double dL1 = 0.001;
    adjustComponent(0.0, 0.0, dL1, 1.0, 0.0, 0.0, 0.0, pws->getInstrument()->getSource()->getName(), pws);

    // Run the calibration
    // NOTE: this should bring the instrument back to engineering position,
    //       which is the solution
    bool calibrateL1 = true;
    bool calibrateBanks = false;
    bool calibrateT0 = false;
    bool tuneSamplePos = false;
    runCalibration(filenamebase, pws, calibrateL1, calibrateBanks, calibrateT0, tuneSamplePos);

    // Apply the calibration results
    MatrixWorkspace_sptr ws = generateSimulatedWorkspace();
    const std::string xmlFileName = filenamebase + ".xml";
    auto lpf_alg = AlgorithmFactory::Instance().create("LoadParameterFile", 1);
    lpf_alg->initialize();
    lpf_alg->setLogging(LOGCHILDALG);
    lpf_alg->setProperty("Workspace", ws);
    lpf_alg->setProperty("Filename", xmlFileName);
    lpf_alg->execute();

    // Checking L1 since it is the only thing we calibrated
    double L1_wrng = pws->getInstrument()->getSource()->getPos().Z();
    double L1_ref = m_pws->getInstrument()->getSource()->getPos().Z();
    double L1_cali = ws->getInstrument()->getSource()->getPos().Z();
    TS_ASSERT_DELTA(L1_cali, L1_ref, TOLERANCE_L);
    g_log.notice() << "@calibration:\n"
                   << L1_wrng << " --> " << L1_cali << "\n"
                   << "@solution:\n"
                   << "L1_ref = " << L1_ref << "\n";
  }

  void run_Bank() {
    g_log.notice() << "test_Bank() starts.\n";
    // Generate unique temp files
    const auto filenamebase = tempfilename();
    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();

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
    //
    adjustComponent(dx, dy, dz, rvx, rvy, rvz, ang, bankname, pws);

    // Run the calibration
    // NOTE: this should bring the instrument back to engineering position,
    //       which is the solution
    bool calibrateL1 = false;
    bool calibrateBanks = true;
    bool calibrateT0 = false;
    bool tuneSamplePos = false;
    runCalibration(filenamebase, pws, calibrateL1, calibrateBanks, calibrateT0, tuneSamplePos);

    // Apply the calibration results
    MatrixWorkspace_sptr ws = generateSimulatedWorkspace();
    const std::string xmlFileName = filenamebase + ".xml";
    auto lpf_alg = AlgorithmFactory::Instance().create("LoadParameterFile", 1);
    lpf_alg->initialize();
    lpf_alg->setLogging(LOGCHILDALG);
    lpf_alg->setProperty("Workspace", ws);
    lpf_alg->setProperty("Filename", xmlFileName);
    lpf_alg->execute();

    // check translation
    V3D pos_wrng = pws->getInstrument()->getComponentByName(bankname)->getRelativePos();
    V3D pos_ref = m_pws->getInstrument()->getComponentByName(bankname)->getRelativePos();
    V3D pos_cali = ws->getInstrument()->getComponentByName(bankname)->getRelativePos();
    g_log.notice() << "@calibration:\n"
                   << pos_wrng << "\n"
                   << "\t--calibrated to-->\n"
                   << pos_cali << "\n"
                   << "@solution:\n"
                   << "pos_ref = " << pos_ref << "\n";
    TS_ASSERT_DELTA(pos_cali.X(), pos_ref.X(), TOLERANCE_L);
    TS_ASSERT_DELTA(pos_cali.Y(), pos_ref.Y(), TOLERANCE_L);
    TS_ASSERT_DELTA(pos_cali.Z(), pos_ref.Z(), TOLERANCE_L);

    // check bank orientation
    Quat q_wrng = pws->getInstrument()->getComponentByName(bankname)->getRelativeRot();
    Quat q_ref = m_pws->getInstrument()->getComponentByName(bankname)->getRelativeRot();
    Quat q_cali = ws->getInstrument()->getComponentByName(bankname)->getRelativeRot();
    g_log.notice() << "@calibration:\n"
                   << q_wrng << "\n"
                   << "--calibrated to-->\n"
                   << q_cali << "\n"
                   << "@solution:\n"
                   << q_ref << "\n";
    // calculate misorientation
    q_cali.inverse();
    Quat dq = q_ref * q_cali;
    double dang = (2 * acos(dq.real()) / PI * 180);
    dang = dang > 180 ? 360 - dang : dang;
    g_log.notice() << "with\n"
                   << "ang(q_ref, q_cali) = " << dang << " (deg) \n";
    TS_ASSERT_LESS_THAN(dang, TOLERANCE_R);
  }

  void run_T0() {
    g_log.notice() << "test_T0() starts.\n";

    // Generate unique temp files
    const auto filenamebase = tempfilename();
    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();

    // Do nothing regarding T0, see if we can get a zero back

    // Run the calibration
    bool calibrateL1 = false;
    bool calibrateBanks = false;
    bool calibrateT0 = true;
    bool tuneSamplePos = false;

    double t0 = runCalibration(filenamebase, pws, calibrateL1, calibrateBanks, calibrateT0, tuneSamplePos);
    g_log.notice() << "calibrated T0 = " << t0 << "\n";
    // NOTE:
    //  It is recommended to have L1 and T0 calibrated at the same time.
  }

  /**
   * @brief Test on calibrating detector size
   */
  void test_calibrateDetectorSize() {

    // Generate unique temp files
    std::string filenameBase = tempfilename();

    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();
    // Resize
    const std::string compname("bank27");
    const double scalex(1.1), scaley(1.1);
    auto resizeAlg = AlgorithmFactory::Instance().create("ResizeRectangularDetector", 1);
    resizeAlg->initialize();
    resizeAlg->setProperty("Workspace", pws);
    resizeAlg->setProperty("ComponentName", compname);
    resizeAlg->setProperty("ScaleX", scalex);
    resizeAlg->setProperty("ScaleY", scaley);
    resizeAlg->execute();
    // Retrieve
    Mantid::API::IPeaksWorkspace_sptr ipws = std::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(pws);
    // Check
    TS_ASSERT(ipws);
    auto input = std::dynamic_pointer_cast<ExperimentInfo>(pws);
    Mantid::Geometry::ParameterMap &pmap = input->instrumentParameters();
    auto checkscalex = pmap.getDouble(compname, std::string("scalex"));
    auto checkscaley = pmap.getDouble(compname, std::string("scaley"));

    TS_ASSERT_DELTA(scalex, checkscalex[0], 0.00000001);
    TS_ASSERT_DELTA(scaley, checkscaley[0], 0.00000001);

    // Init, config and run Calibration
    const std::string isawFilename = filenameBase + ".DetCal";
    const std::string xmlFilename = filenameBase + ".xml";
    const std::string csvFilename = filenameBase + ".csv";

    // execute the calibration
    SCDCalibratePanels2 alg;
    alg.initialize();
    alg.setProperty("PeakWorkspace", pws);
    alg.setProperty("a", silicon_a);
    alg.setProperty("b", silicon_b);
    alg.setProperty("c", silicon_c);
    alg.setProperty("alpha", silicon_alpha);
    alg.setProperty("beta", silicon_beta);
    alg.setProperty("gamma", silicon_gamma);
    alg.setProperty("RecalculateUB", false);
    alg.setProperty("CalibrateL1", false);
    alg.setProperty("CalibrateT0", false);
    // special about det size calibration
    alg.setProperty("CalibrateBanks", true);
    alg.setProperty("SearchRadiusTransBank", 0.0);
    alg.setProperty("SearchradiusRotXBank", 0.0);
    alg.setProperty("SearchradiusRotYBank", 0.0);
    alg.setProperty("SearchradiusRotZBank", 0.0);

    alg.setProperty("CalibrateSize", true);
    alg.setProperty("SearchRadiusSize", 0.2);
    alg.setProperty("FixAspectRatio", true);

    alg.setProperty("TuneSamplePosition", false);
    alg.setProperty("OutputWorkspace", "caliTableDetSizeTest");
    alg.setProperty("DetCalFilename", isawFilename);
    alg.setProperty("XmlFilename", xmlFilename);
    alg.setProperty("CSVFilename", csvFilename);

    alg.setProperty("MaxFitIterations", 5);
    alg.setProperty("BankName", "bank27");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
  }

  void SaveTime_test_samplePos() {
    g_log.notice() << "test_samplePos() starts.\n";
    // Generate unique temp files
    const auto filenamebase = tempfilename();

    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();

    // shift the sample pos to a "wrong" state
    double dsx = -0.01;
    adjustComponent(dsx, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, "sample-position", pws);

    // Run the calibration
    // NOTE: this should bring the instrument back to engineering position,
    //       which is the solution
    bool calibrateL1 = true;
    bool calibrateBanks = false;
    bool calibrateT0 = true;
    bool tuneSamplePos = true;
    runCalibration(filenamebase, pws, calibrateL1, calibrateBanks, calibrateT0, tuneSamplePos);

    // Apply the calibration results
    MatrixWorkspace_sptr ws = generateSimulatedWorkspace();
    const std::string xmlFileName = filenamebase + ".xml";
    IAlgorithm_sptr lpf_alg = AlgorithmFactory::Instance().create("LoadParameterFile", 1);
    lpf_alg->initialize();
    lpf_alg->setLogging(LOGCHILDALG);
    lpf_alg->setProperty("Workspace", ws);
    lpf_alg->setProperty("Filename", xmlFileName);
    lpf_alg->execute();

    // Checking sample.x since it is the only thing we calibrated
    double dsx_wrng = pws->getInstrument()->getSample()->getPos().X();
    double dsx_ref = m_pws->getInstrument()->getSample()->getPos().X();
    double dsx_cali = ws->getInstrument()->getSample()->getPos().X();
    TS_ASSERT_DELTA(dsx_cali, dsx_ref, TOLERANCE_L);

    g_log.notice() << "@calibration:\n"
                   << dsx_wrng << " --> " << dsx_cali << "\n"
                   << "@solution:\n"
                   << "dsx_ref = " << dsx_ref << "\n";
  }

  // NOTE: skipped to prevent time out on build server
  void run_Exec() {
    g_log.notice() << "test_Exec() starts.\n";
    // Generate unique temp files
    const auto filenamebase = tempfilename();
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

    // Run the calibration
    // NOTE: this should bring the instrument back to engineering position,
    //       which is the solution
    bool calibrateL1 = false;
    bool calibrateBanks = true;
    bool calibrateT0 = false;
    bool tuneSamplePos = false;
    runCalibration(filenamebase, pws, calibrateL1, calibrateBanks, calibrateT0, tuneSamplePos);

    // Apply the calibration results
    MatrixWorkspace_sptr ws = generateSimulatedWorkspace();
    const std::string xmlFileName = filenamebase + ".xml";
    auto lpf_alg = AlgorithmFactory::Instance().create("LoadParameterFile", 1);
    lpf_alg->initialize();
    lpf_alg->setLogging(LOGCHILDALG);
    lpf_alg->setProperty("Workspace", ws);
    lpf_alg->setProperty("Filename", xmlFileName);
    lpf_alg->execute();

    // Check
    // -- L1
    double L1_wrng = pws->getInstrument()->getSource()->getPos().Z();
    double L1_ref = m_pws->getInstrument()->getSource()->getPos().Z();
    double L1_cali = ws->getInstrument()->getSource()->getPos().Z();
    TS_ASSERT_DELTA(L1_cali, L1_ref, TOLERANCE_L);
    g_log.notice() << "<<Source>>\n"
                   << "@calibration:\n"
                   << L1_wrng << " --> " << L1_cali << "\n"
                   << "@solution:\n"
                   << "L1_ref = " << L1_ref << "\n";
    // -- bank27
    g_log.notice() << "<<bank27>>\n";
    V3D pos_wrng = pws->getInstrument()->getComponentByName(bank27)->getRelativePos();
    V3D pos_ref = m_pws->getInstrument()->getComponentByName(bank27)->getRelativePos();
    V3D pos_cali = ws->getInstrument()->getComponentByName(bank27)->getRelativePos();
    g_log.notice() << "@calibration:\n"
                   << pos_wrng << "\n"
                   << "\t--calibrated to-->\n"
                   << pos_cali << "\n"
                   << "@solution:\n"
                   << "pos_ref = " << pos_ref << "\n";
    TS_ASSERT_DELTA(pos_cali.X(), pos_ref.X(), TOLERANCE_L);
    TS_ASSERT_DELTA(pos_cali.Y(), pos_ref.Y(), TOLERANCE_L);
    TS_ASSERT_DELTA(pos_cali.Z(), pos_ref.Z(), TOLERANCE_L);
    Quat q_wrng = pws->getInstrument()->getComponentByName(bank27)->getRelativeRot();
    Quat q_ref = m_pws->getInstrument()->getComponentByName(bank27)->getRelativeRot();
    Quat q_cali = ws->getInstrument()->getComponentByName(bank27)->getRelativeRot();
    g_log.notice() << "@calibration:\n"
                   << q_wrng << "\n"
                   << "--calibrated to-->\n"
                   << q_cali << "\n"
                   << "@solution:\n"
                   << q_ref << "\n";
    // calculate misorientation
    q_cali.inverse();
    Quat dq = q_ref * q_cali;
    double dang = (2 * acos(dq.real()) / PI * 180);
    dang = dang > 180 ? 360 - dang : dang;
    g_log.notice() << "with\n"
                   << "ang(q_ref, q_cali) = " << dang << " (deg) \n";
    TS_ASSERT_LESS_THAN(dang, TOLERANCE_R);
    // -- bank16
    g_log.notice() << "<<bank16>>\n";
    pos_wrng = pws->getInstrument()->getComponentByName(bank16)->getRelativePos();
    pos_ref = m_pws->getInstrument()->getComponentByName(bank16)->getRelativePos();
    pos_cali = ws->getInstrument()->getComponentByName(bank16)->getRelativePos();
    g_log.notice() << "@calibration:\n"
                   << pos_wrng << "\n"
                   << "\t--calibrated to-->\n"
                   << pos_cali << "\n"
                   << "@solution:\n"
                   << "pos_ref = " << pos_ref << "\n";
    TS_ASSERT_DELTA(pos_cali.X(), pos_ref.X(), TOLERANCE_L);
    TS_ASSERT_DELTA(pos_cali.Y(), pos_ref.Y(), TOLERANCE_L);
    TS_ASSERT_DELTA(pos_cali.Z(), pos_ref.Z(), TOLERANCE_L);
    q_wrng = pws->getInstrument()->getComponentByName(bank16)->getRelativeRot();
    q_ref = m_pws->getInstrument()->getComponentByName(bank16)->getRelativeRot();
    q_cali = ws->getInstrument()->getComponentByName(bank16)->getRelativeRot();
    g_log.notice() << "@calibration:\n"
                   << q_wrng << "\n"
                   << "--calibrated to-->\n"
                   << q_cali << "\n"
                   << "@solution:\n"
                   << q_ref << "\n";
    // calculate misorientation
    q_cali.inverse();
    dq = q_ref * q_cali;
    dang = (2 * acos(dq.real()) / PI * 180);
    dang = dang > 180 ? 360 - dang : dang;
    g_log.notice() << "with\n"
                   << "ang(q_ref, q_cali) = " << dang << " (deg) \n";
    TS_ASSERT_LESS_THAN(dang, TOLERANCE_R);
  }

private:
  // ---------------------------- //
  // ----- Helper Functions ----- //
  // -----------------------------//

  /**
   * @brief generate a simulated workspace for each testing case
   *
   * @return MatrixWorkspace_sptr
   */
  MatrixWorkspace_sptr generateSimulatedWorkspace() {

    // create simulated workspace
    auto csws_alg = AlgorithmFactory::Instance().create("CreateSimulationWorkspace", 1);
    csws_alg->initialize();
    csws_alg->setLogging(LOGCHILDALG);
    csws_alg->setProperty("Instrument", "TOPAZ");
    csws_alg->setProperty("BinParams", "1,100,10000");
    csws_alg->setProperty("UnitX", "TOF");
    csws_alg->setProperty("OutputWorkspace", wsname);
    csws_alg->execute();
    TS_ASSERT(csws_alg->isExecuted());

    // set UB
    auto sub_alg = AlgorithmFactory::Instance().create("SetUB", 1);
    sub_alg->initialize();
    sub_alg->setLogging(LOGCHILDALG);
    sub_alg->setProperty("Workspace", wsname);
    sub_alg->setProperty("u", "0.5,0.8660254037844387,0");
    sub_alg->setProperty("v", "-0.8660254037844387,0.5,0");
    // sub_alg->setProperty("u", "1.0,0.0,0.0");
    // sub_alg->setProperty("v", "0.0,1.0,0.0");
    sub_alg->setProperty("a", silicon_a);
    sub_alg->setProperty("b", silicon_b);
    sub_alg->setProperty("c", silicon_c);
    sub_alg->setProperty("alpha", silicon_alpha);
    sub_alg->setProperty("beta", silicon_beta);
    sub_alg->setProperty("gamma", silicon_gamma);
    sub_alg->execute();
    TS_ASSERT(sub_alg->isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsname);

    // auto &sample = ws->mutableSample();
    // sample.setCrystalStructure(silicon_cs);

    return ws;
  }

  /**
   * @brief populate peaks for the post adjustment simulated workspace
   *
   * @return PeaksWorkspace_sptr
   */
  PeaksWorkspace_sptr generateSimulatedPeaksWorkspace(MatrixWorkspace_sptr ws) {
    // prepare the algs pointer
    auto sg_alg = AlgorithmFactory::Instance().create("SetGoniometer", 1);
    auto pp_alg = AlgorithmFactory::Instance().create("PredictPeaks", 1);
    auto cpw_alg = AlgorithmFactory::Instance().create("CombinePeaksWorkspaces", 1);

    // generate peaks for a range of omega values
    for (double omega = 0; omega <= 180; omega = omega + omega_step) {
      std::ostringstream os;
      os << omega << ",0,1,0,1";

      // set the SetGoniometer
      sg_alg->initialize();
      sg_alg->setLogging(LOGCHILDALG);
      sg_alg->setProperty("Workspace", ws);
      sg_alg->setProperty("Axis0", os.str());
      sg_alg->execute();

      // predict peak positions
      pp_alg->initialize();
      pp_alg->setLogging(LOGCHILDALG);
      pp_alg->setProperty("InputWorkspace", ws);
      pp_alg->setProperty("WavelengthMin", wavelength_min);
      pp_alg->setProperty("wavelengthMax", wavelength_max);
      pp_alg->setProperty("MinDSpacing", dspacing_min);
      pp_alg->setProperty("MaxDSpacing", dspacing_max);
      pp_alg->setProperty("ReflectionCondition", "All-face centred");

      if (omega < omega_step) {
        pp_alg->setProperty("OutputWorkspace", pwsname);
        pp_alg->execute();
      } else {
        pp_alg->setProperty("OutputWorkspace", tmppwsname);
        pp_alg->execute();

        // add the peaks to output peaks workspace
        cpw_alg->initialize();
        cpw_alg->setLogging(LOGCHILDALG);
        cpw_alg->setProperty("LHSWorkspace", tmppwsname);
        cpw_alg->setProperty("RHSWorkspace", pwsname);
        cpw_alg->setProperty("OutputWorkspace", pwsname);
        cpw_alg->execute();
      }
    }

    return AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(pwsname);
  }

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
   * @brief Run the calibration algorithm
   *
   * @param filenameBase
   * @param pws
   * @param calibrateL1
   * @param calibrateBanks
   * @param calibrateT0
   * @param tuneSamplePosition
   */
  double runCalibration(const std::string filenameBase, PeaksWorkspace_sptr pws, bool calibrateL1, bool calibrateBanks,
                        bool calibrateT0, bool tuneSamplePosition) {
    // generate isaw, xml, and csv filename
    const std::string isawFilename = filenameBase + ".DetCal";
    const std::string xmlFilename = filenameBase + ".xml";
    const std::string csvFilename = filenameBase + ".csv";

    // execute the calibration
    SCDCalibratePanels2 alg;
    alg.initialize();
    alg.setProperty("PeakWorkspace", pws);
    alg.setProperty("a", silicon_a);
    alg.setProperty("b", silicon_b);
    alg.setProperty("c", silicon_c);
    alg.setProperty("alpha", silicon_alpha);
    alg.setProperty("beta", silicon_beta);
    alg.setProperty("gamma", silicon_gamma);
    alg.setProperty("RecalculateUB", false);
    alg.setProperty("CalibrateL1", calibrateL1);
    alg.setProperty("CalibrateBanks", calibrateBanks);
    alg.setProperty("CalibrateT0", calibrateT0);
    alg.setProperty("TuneSamplePosition", tuneSamplePosition);
    alg.setProperty("OutputWorkspace", "caliTableTest");
    alg.setProperty("DetCalFilename", isawFilename);
    alg.setProperty("XmlFilename", xmlFilename);
    alg.setProperty("CSVFilename", csvFilename);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    double t0 = alg.getProperty("T0");

    return t0;
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
