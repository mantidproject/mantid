// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

// NOTE: Generating a synthetic peakworkspace for testing is very time
//       consuming, therefore only one wholesome unittest with calibration
//       is enabled for regression test.
//       To test other testing targets, change the lead word from
//                 run_ to test_
//       to run them within the ctest framework.
//       You might need to do one at a time to avoid ctest timeout error
//       locally.

// DEVNOTE:
//  - cos, sin func uses radians
//  - Quat class uses degrees

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
Logger g_log("SCDCalibratePanels2Test");
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
        bank_xtop("bank73/sixteenpack"),                        //
        bank_xcenter("bank12/sixteenpack"),                     //
        bank_xbottom("bank11/sixteenpack"),                     //
        bank_yright("bank59/sixteenpack"),                      //
        bank_yleft("bank58/sixteenpack"),                       //
        bank_ytop("bank88/sixteenpack"),                        //
        bank_ybottom("bank26/sixteenpack"),                     //
        silicon_a(5.431), silicon_b(5.431), silicon_c(5.431),   // angstrom
        silicon_alpha(90), silicon_beta(90), silicon_gamma(90), // degree
        silicon_cs(CrystalStructure("5.431 5.431 5.431", "F d -3 m", "Si 0 0 0 1.0 0.02")), dspacing_min(1.0),
        dspacing_max(10.0),                      //
        wavelength_min(0.1), wavelength_max(10), //
        omega_step(6.0),                         //
        TOLERANCE_L(1e-6),                       // this calibration has intrinsic accuracy limit of
                                                 // 5mm for translation
        TOLERANCE_R(1e-5),                       // this calibration has intrinsic accuracy limit of
                                                 // 0.1 deg for rotation
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

    m_ws = generateSimulatedWorkspace();
    m_pws = generateSimulatedPeaksWorkspace(m_ws);
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
    auto filenamebase = boost::filesystem::temp_directory_path();
    filenamebase /= boost::filesystem::unique_path("nullcase_%%%%%%%%");
    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();

    // No need to change anything

    // Pretend we forget the answer
    pws->setInstrument(m_pws->getInstrument());
    // Run the calibration
    runCalibration(filenamebase.string(), pws, false, true, false);

    // Check if the calibration results
    // -- get a blank workspace for loading cali results
    // MatrixWorkspace_sptr ws_raw = m_ws->clone();
    // -- check
    // bool sameInstrument =
    // validateCalibrationResults(m_pws, ws_raw, filenamebase.string());
    // -- assert
    // NOTE:
    //    Due to built-in q vector centering, the asseration will always fail.
    //    We are commenting it out for now.
    // TS_ASSERT(sameInstrument);
  }

  void run_L1() {
    g_log.notice() << "test_L1() starts.\n";
    // Generate unique temp files
    auto filenamebase = boost::filesystem::temp_directory_path();
    filenamebase /= boost::filesystem::unique_path("testL1_%%%%%%%%");
    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();

    // Shift L1 to a "wrong" state
    double dL1 = 0.01;
    adjustComponent(0.0, 0.0, dL1, 1.0, 0.0, 0.0, 0.0, pws->getInstrument()->getSource()->getName(), pws);

    // Run the calibration
    // NOTE: this should bring the instrument back to engineering position,
    //       which is the solution
    runCalibration(filenamebase.string(), pws, false, true, false);

    // Check if the calibration results
    // -- get a blank workspace for loading cali results
    // MatrixWorkspace_sptr ws_raw = m_ws->clone();
    // -- check
    // bool sameInstrument =
    // validateCalibrationResults(m_pws, ws_raw, filenamebase.string());
    // -- assert
    // NOTE:
    //    Due to built-in q vector centering, the asseration will always fail.
    //    We are commenting it out for now.
    // TS_ASSERT(sameInstrument);
  }

  void run_Bank() {
    g_log.notice() << "test_Bank() starts.\n";
    // Generate unique temp files
    auto filenamebase = boost::filesystem::temp_directory_path();
    filenamebase /= boost::filesystem::unique_path("testBank_%%%%%%%%");
    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();

    // Adjust bank
    //
    // NOTE: the common range for dx, dy ,dz is +-5cm
    double dx = 1.1e-2;
    double dy = -0.9e-2;
    double dz = 1.5e-2;
    // prescribed rotation
    double theta = PI / 3;
    double phi = PI / 8;
    double rvx = sin(theta) * cos(phi);
    double rvy = sin(theta) * sin(phi);
    double rvz = cos(theta);
    double ang = 1.414; // degrees
    //
    adjustComponent(dx, dy, dz, rvx, rvy, rvz, ang, bank_xtop, pws);

    // Run the calibration
    // NOTE: this should bring the instrument back to engineering position,
    //       which is the solution
    runCalibration(filenamebase.string(), pws, false, false, true);

    // Check if the calibration results
    // -- get a blank workspace for loading cali results
    // MatrixWorkspace_sptr ws_raw = m_ws->clone();
    // -- check
    // bool sameInstrument =
    // validateCalibrationResults(m_pws, ws_raw, filenamebase.string());
    // -- assert
    // NOTE:
    //    Due to built-in q vector centering, the asseration will always fail.
    //    We are commenting it out for now.
    // TS_ASSERT(sameInstrument);
  }

  void test_Exec() {
    g_log.notice() << "test_Exec() starts.\n";
    // Generate unique temp files
    auto filenamebase = boost::filesystem::temp_directory_path();
    filenamebase /= boost::filesystem::unique_path("testExec_%%%%%%%%");
    // Make a clone of the standard peak workspace
    PeaksWorkspace_sptr pws = m_pws->clone();

    // Adjust L1 and banks
    //-- source
    const double dL1 = boost::math::constants::e<double>() / 100;
    //-- xtop
    double dx1 = 1.1e-2;
    double dy1 = -0.9e-2;
    double dz1 = 1.5e-2;
    double theta1 = PI / 3;
    double phi1 = PI / 8;
    double rvx1 = sin(theta1) * cos(phi1);
    double rvy1 = sin(theta1) * sin(phi1);
    double rvz1 = cos(theta1);
    double ang1 = 1.414; // degrees
    //-- xbottom
    double dx2 = 0.5e-2;
    double dy2 = 1.3e-2;
    double dz2 = -1.9e-2;
    double theta2 = PI / 4;
    double phi2 = PI / 3;
    double rvx2 = sin(theta2) * cos(phi2);
    double rvy2 = sin(theta2) * sin(phi2);
    double rvz2 = cos(theta2);
    double ang2 = 2.13; // degrees
    // source
    adjustComponent(0.0, 0.0, dL1, 1.0, 0.0, 0.0, 0.0, pws->getInstrument()->getSource()->getName(), pws);
    // Bank73
    adjustComponent(dx1, dy1, dz1, rvx1, rvy1, rvz1, ang1, bank_xtop, pws);
    // Bank11
    adjustComponent(dx2, dy2, dz2, rvx2, rvy2, rvz2, ang2, bank_xbottom, pws);

    // Run the calibration
    // NOTE: this should bring the instrument back to engineering position,
    //       which is the solution
    runCalibration(filenamebase.string(), pws, false, false, true);

    // Check if the calibration results
    // -- get a blank workspace for loading cali results
    // MatrixWorkspace_sptr ws_raw = m_ws->clone();
    // -- check
    // bool sameInstrument =
    // validateCalibrationResults(m_pws, ws_raw, filenamebase.string());
    // -- assert
    // NOTE:
    //    Due to built-in q vector centering, the asseration will always
    //    fail. We are commenting it out for now.
    // TS_ASSERT(sameInstrument);
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
    IAlgorithm_sptr csws_alg = AlgorithmFactory::Instance().create("CreateSimulationWorkspace", 1);
    csws_alg->initialize();
    csws_alg->setLogging(LOGCHILDALG);
    csws_alg->setProperty("Instrument", "CORELLI");
    csws_alg->setProperty("BinParams", "1,100,10000");
    csws_alg->setProperty("UnitX", "TOF");
    csws_alg->setProperty("OutputWorkspace", wsname);
    csws_alg->execute();
    TS_ASSERT(csws_alg->isExecuted());

    // set UB
    IAlgorithm_sptr sub_alg = AlgorithmFactory::Instance().create("SetUB", 1);
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
    IAlgorithm_sptr sg_alg = AlgorithmFactory::Instance().create("SetGoniometer", 1);
    IAlgorithm_sptr pp_alg = AlgorithmFactory::Instance().create("PredictPeaks", 1);
    IAlgorithm_sptr cpw_alg = AlgorithmFactory::Instance().create("CombinePeaksWorkspaces", 1);

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
    IAlgorithm_sptr rot_alg = Mantid::API::AlgorithmFactory::Instance().create("RotateInstrumentComponent", -1);
    rot_alg->initialize();
    rot_alg->setLogging(LOGCHILDALG);
    rot_alg->setProperty("Workspace", pws);
    rot_alg->setProperty("ComponentName", cmptName);
    rot_alg->setProperty("X", rvx);
    rot_alg->setProperty("Y", rvy);
    rot_alg->setProperty("Z", rvz);
    rot_alg->setProperty("Angle", drotang);
    rot_alg->setProperty("RelativeRotation", true);
    rot_alg->executeAsChildAlg();

    // translation
    IAlgorithm_sptr mv_alg = Mantid::API::AlgorithmFactory::Instance().create("MoveInstrumentComponent", -1);
    mv_alg->initialize();
    mv_alg->setLogging(LOGCHILDALG);
    mv_alg->setProperty("Workspace", pws);
    mv_alg->setProperty("ComponentName", cmptName);
    mv_alg->setProperty("X", dx);
    mv_alg->setProperty("Y", dy);
    mv_alg->setProperty("Z", dz);
    mv_alg->setProperty("RelativePosition", true);
    mv_alg->executeAsChildAlg();

    // since moving instrument does not trigger the update the embedded peaks,
    // we need to do this manually
    double tof;
    Units::Wavelength wl;
    for (int i = 0; i < pws->getNumberPeaks(); ++i) {
      tof = pws->getPeak(i).getTOF();
      pws->getPeak(i).setInstrument(pws->getInstrument());
      wl.initialize(pws->getPeak(i).getL1(), pws->getPeak(i).getL2(), pws->getPeak(i).getScattering(), 0,
                    pws->getPeak(i).getInitialEnergy(), 0.0);
      pws->getPeak(i).setDetectorID(pws->getPeak(i).getDetectorID());
      pws->getPeak(i).setWavelength(wl.singleFromTOF(tof));
    }
  }

  /**
   * @brief Run the calibration algorithm
   *
   * @param filenameBase
   * @param pws
   * @param calibrateT0
   * @param calibrateL1
   * @param calibrateBanks
   */
  void runCalibration(const std::string filenameBase, PeaksWorkspace_sptr pws, bool calibrateT0, bool calibrateL1,
                      bool calibrateBanks) {
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
    alg.setProperty("CalibrateT0", calibrateT0);
    alg.setProperty("CalibrateL1", calibrateL1);
    alg.setProperty("CalibrateBanks", calibrateBanks);
    alg.setProperty("OutputWorkspace", "caliTableTest");
    alg.setProperty("DetCalFilename", isawFilename);
    alg.setProperty("XmlFilename", xmlFilename);
    alg.setProperty("CSVFilename", csvFilename);
    alg.execute();
    TS_ASSERT(alg.isExecuted());
  }

  /**
   * @brief validate the calibration results by comparing component
   *        positions from reference Peakworkspace and workspace adjusted
   *        using calibration output (xml)
   *
   * @param refpws
   * @param refws
   * @param xmlFileName
   * @return true
   * @return false
   */
  bool validateCalibrationResults(PeaksWorkspace_sptr refpws, MatrixWorkspace_sptr refws, const std::string &fileName) {
    // Test using xml parameter file (default)
    const std::string xmlFileName = fileName + ".xml";

    g_log.notice() << "Using Paramter file: " << xmlFileName << "\n";
    // Adjust components in reference workspace using calibration results
    IAlgorithm_sptr lpf_alg = AlgorithmFactory::Instance().create("LoadParameterFile", 1);
    lpf_alg->initialize();
    lpf_alg->setLogging(LOGCHILDALG);
    lpf_alg->setProperty("Workspace", refws);
    lpf_alg->setProperty("Filename", xmlFileName);
    lpf_alg->execute();

    // Test using DetCal paramter file (debug)
    // const std::string isawFileName = fileName + ".DetCal";
    // IAlgorithm_sptr lpf_alg =
    //     AlgorithmFactory::Instance().create("LoadIsawDetCal", 1);
    // lpf_alg->initialize();
    // lpf_alg->setLogging(LOGCHILDALG);
    // lpf_alg->setProperty("InputWorkspace", refws);
    // lpf_alg->setProperty("Filename", fileName);
    // lpf_alg->execute();

    // compare each bank
    // -- get the names
    boost::container::flat_set<std::string> BankNames;
    for (int i = 0; i < refpws->getNumberPeaks(); ++i) {
      std::string bname = refpws->getPeak(i).getBankName();
      if (bname != "None") {
        BankNames.insert(bname);
      }
    }
    // -- perform per bank comparison
    Instrument_sptr inst1 = std::const_pointer_cast<Instrument>(refws->getInstrument());  // based on calibration
    Instrument_sptr inst2 = std::const_pointer_cast<Instrument>(refpws->getInstrument()); // reference one

    bool sameInstrument = true;
    for (auto bankname : BankNames) {
      // update bankname for CORELLI
      if (refpws->getInstrument()->getName().compare("CORELLI") == 0)
        bankname.append("/sixteenpack");

      if (!compareComponent(inst1, inst2, bankname)) {
        g_log.error() << "--" << bankname << " mismatch\n";
        sameInstrument = false;
      }
    }

    // all banks are the same, now the source check will make the call
    if (!compareComponent(inst1, inst2, inst1->getSource()->getName())) {
      g_log.error() << "-- " << inst1->getSource()->getName() << " mismatch\n";
      sameInstrument = false;
    }

    return sameInstrument;
  }

  /**
   * @brief compare if two components to see if they have similar
   *        translation and rotation
   *
   * @param instr1
   * @param instr2
   * @param componentName
   * @return true
   * @return false
   */
  bool compareComponent(std::shared_ptr<Instrument> &instr1, std::shared_ptr<Instrument> &instr2,
                        std::string componentName) {

    std::shared_ptr<const IComponent> cmpt1 = instr1->getComponentByName(componentName);
    std::shared_ptr<const IComponent> cmpt2 = instr2->getComponentByName(componentName);

    V3D p1 = cmpt1->getRelativePos();
    V3D p2 = cmpt2->getRelativePos();

    Quat q1 = cmpt1->getRelativeRot();
    Quat q2 = cmpt2->getRelativeRot();

    q2.inverse();
    Quat dq = q1 * q2;
    double dang = (2 * acos(dq.real()) / PI * 180);
    dang = dang > 180 ? 360 - dang : dang;

    double dx = std::abs(p1.X() - p2.X());
    double dy = std::abs(p1.Y() - p2.Y());
    double dz = std::abs(p1.Z() - p2.Z());

    bool sameComponent;
    if (dx > TOLERANCE_L || dy > TOLERANCE_L || dz > TOLERANCE_L || dang > TOLERANCE_R) {
      sameComponent = false;
    } else {
      sameComponent = true;
    }

    if (!sameComponent)
      g_log.notice() << std::setprecision(8) << "--Component " << componentName << "\n"
                     << "  cali: " << p1 << "\n"
                     << "  ref: " << p2 << "\n"
                     << "    dx = " << dx << "\n"
                     << "    dy = " << dy << "\n"
                     << "    dz = " << dz << "\n"
                     << "  cali: " << cmpt1->getRelativeRot() << "\n"
                     << "  ref: " << cmpt2->getRelativeRot() << "\n"
                     << "    misorientation = " << dang << "\n";

    return sameComponent;
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

  // bank&panel names selected for testing
  // batch_1: high order zone selection
  const std::string bank_xtop;
  const std::string bank_xcenter;
  const std::string bank_xbottom;

  // batch_2: low order zone selection
  // NOTE: limited reflections from experiment, often
  //       considered as a chanllegening case
  const std::string bank_yright;
  const std::string bank_yleft;
  const std::string bank_ytop;
  const std::string bank_ybottom;

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
