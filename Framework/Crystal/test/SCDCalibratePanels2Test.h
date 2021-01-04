// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/SCDCalibratePanels2.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidKernel/Logger.h"

#include <cxxtest/TestSuite.h>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/math/constants/constants.hpp>

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
  static void destroySuite( SCDCalibratePanels2Test *suite ) { delete suite; }

  // ----------------- //
  // ----- Setup ----- //
  // ----------------- //
  /**
   * @brief Construct a new SCDCalibratePanels2Test object
   *
   */
  SCDCalibratePanels2Test()
      : wsname("wsSCDCalibratePanels2Test"),
        pwsname("pwsSCDCalibratePanels2Test"),
        tmppwsname("tmppwsSCDCalibratePanels2Test"), // fixed workspace name
        bank_xtop("bank73/sixteenpack"),             //
        bank_xcenter("bank12/sixteenpack"),          //
        bank_xbottom("bank11/sixteenpack"),          //
        bank_yright("bank59/sixteenpack"),           //
        bank_yleft("bank58/sixteenpack"),            //
        bank_ytop("bank88/sixteenpack"),             //
        bank_ybottom("bank26/sixteenpack"),          //
        silicon_a(5.431), silicon_b(5.431), silicon_c(5.431),   // angstrom
        silicon_alpha(90), silicon_beta(90), silicon_gamma(90), // degree
        silicon_cs(CrystalStructure("5.431 5.431 5.431", "F d -3 m",
                                    "Si 0 0 0 1.0 0.02")),
        dspacing_min(1.0), dspacing_max(10.0),    //
        wavelength_min(0.8), wavelength_max(2.9), //
        omega_step(3.0),                          //
        TOLERANCE_L(1e-2), // this calibration has intrinsic accuracy limit of
                           // 0.01m for translation
        TOLERANCE_R(1e-4), // this calibration has intrinsic accuracy limit of
                           // 1e-4 deg for rotation
        LOGCHILDALG(false) {
    // NOTE:
    //  The MAGIC PIECE, basically we need to let AlgorithmFactory
    //  to load a non-related algorithm, then somehow AlgorithmFactory
    //  can find the Fit algorithm for the remaining test
    std::shared_ptr<Algorithm> darkmagic =
        AlgorithmFactory::Instance().create("LoadIsawPeaks", 1);
    darkmagic->initialize();
    darkmagic->setLogging(false);  // don't really care its output
    darkmagic->setPropertyValue("Filename", "Peaks5637.integrate");
    darkmagic->setPropertyValue("OutputWorkspace", "TOPAZ_5637");
    TS_ASSERT(darkmagic->execute());

    m_ws = generateSimulatedWorkspace();
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
    TS_ASSERT_EQUALS(alg.name(), "SCDCalibratePanels2");
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

  // /**
  //  * @brief Trivial case where all components are in ideal/starting position
  //  *        Therefore the calibration results should be close to a zero
  //  *        vector.
  //  */
  // void test_Null_Case() {
  //   g_log.notice() << "test: !Null case!\n";

  //   // Generate unique temp files
  //   auto isawFile = boost::filesystem::temp_directory_path();
  //   isawFile /= boost::filesystem::unique_path("nullcase_%%%%%%%%.DetCal");
  //   auto xmlFile = boost::filesystem::temp_directory_path();
  //   xmlFile /= boost::filesystem::unique_path("nullcase_%%%%%%%%.xml");

  //   g_log.notice() << "-- generate simulated workspace\n";
  //   MatrixWorkspace_sptr ws = m_ws->clone();
  //   MatrixWorkspace_sptr wsraw = ws->clone();

  //   // Trivial case, no component undergoes any affine transformation
  //   g_log.notice() << "-- trivial case, no components moved\n";

  //   g_log.notice() << "-- generate peaks\n";
  //   PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
  //   PeaksWorkspace_sptr pwsref = pws->clone();

  //   // Pretend we don't know the answer
  //   g_log.notice() << "-- reset instrument positions&orientations\n";
  //   pws->setInstrument(wsraw->getInstrument());

  //   // Perform the calibration
  //   g_log.notice() << "-- start calibration\n";
  //   runCalibration(isawFile.string(), xmlFile.string(), pws, false, true,
  //   true);

  //   // Check if the calibration returns the same instrument as we put in
  //   g_log.notice() << "-- validate calibration output\n";
  //   TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

  //   // Cleanup
  //   doCleanup();
  // }

  // /**
  //  * @brief Single variant case where only global var is adjusted.
  //  *
  //  * NOTE: technically we should also check T0, but the client, CORELLI
  //  *       team does not seem to care about using T0, therefore we are
  //  *       not implmenting T0 calibration here.
  //  *
  //  */
  // void test_L1_Shift() {
  //   g_log.notice() << "test: !Source Shift (L1 change)!\n";

  //   // prescribed shift
  //   const double dL1 = boost::math::constants::e<double>();

  //   // Generate unique temp files
  //   auto isawFile = boost::filesystem::temp_directory_path();
  //   isawFile /= boost::filesystem::unique_path("changeL1_%%%%%%%%.DetCal");
  //   auto xmlFile = boost::filesystem::temp_directory_path();
  //   xmlFile /= boost::filesystem::unique_path("changeL1_%%%%%%%%.xml");

  //   g_log.notice() << "-- generate simulated workspace\n";
  //   MatrixWorkspace_sptr ws = m_ws->clone();
  //   MatrixWorkspace_sptr wsraw = ws->clone();

  //   // move source
  //   adjustComponent(0.0, 0.0, dL1, 0.0, 0.0, 0.0,
  //                   ws->getInstrument()->getSource()->getName(), ws);

  //   g_log.notice() << "-- generate peaks\n";
  //   PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
  //   PeaksWorkspace_sptr pwsref = pws->clone();

  //   // Pretend we don't know the answer
  //   g_log.notice() << "-- reset instrument positions&orientations\n";
  //   g_log.notice() << "    * before reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";
  //   pws->setInstrument(wsraw->getInstrument());
  //   g_log.notice() << "    * after reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";

  //   // Perform the calibration
  //   g_log.notice() << "-- start calibration\n";
  //   runCalibration(isawFile.string(), xmlFile.string(), pws, false, true,
  //                  false);

  //   // Check if the calibration returns the same instrument as we put in
  //   g_log.notice() << "-- validate calibration output\n";
  //   TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

  //   // Cleanup
  //   doCleanup();
  // }

  // /**
  //  * @brief Single panel translated by (dx, dy, dz)
  //  *
  //  */
  // void test_Single_Panel_Shift() {
  //   g_log.notice() << "test: !Single Panel Shift!\n";

  //   //prescribed shift
  //   double dx = boost::math::constants::euler<double>();
  //   double dy = boost::math::constants::ln_ln_two<double>();
  //   double dz = boost::math::constants::pi_minus_three<double>();

  //   // Generate unique temp files
  //   auto isawFile = boost::filesystem::temp_directory_path();
  //   isawFile /=
  //   boost::filesystem::unique_path("unoPanelShift_%%%%%%%%.DetCal"); auto
  //   xmlFile = boost::filesystem::temp_directory_path(); xmlFile /=
  //   boost::filesystem::unique_path("unoPanelShift_%%%%%%%%.xml");

  //   g_log.notice() << "-- generate simulated workspace\n";
  //   MatrixWorkspace_sptr ws = m_ws->clone();
  //   MatrixWorkspace_sptr wsraw = ws->clone();

  //   // adjust xcenter
  //   g_log.notice() << "-- translate bank12 (x center) by (" << dx << "," <<
  //   dy
  //                  << "," << dz << ")\n";
  //   adjustComponent(dx, dy, dz, 0.0, 0.0, 0.0, bank_xcenter, ws);

  //   g_log.notice() << "-- generate peaks\n";
  //   PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
  //   PeaksWorkspace_sptr pwsref = pws->clone();

  //   // Pretend we don't know the answer
  //   g_log.notice() << "-- reset instrument positions&orientations\n";
  //   g_log.notice() << "    * before reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";
  //   pws->setInstrument(wsraw->getInstrument());
  //   g_log.notice() << "    * after reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";

  //   // Perform the calibration
  //   g_log.notice() << "-- start calibration\n";
  //   runCalibration(isawFile.string(), xmlFile.string(), pws, false, false,
  //                  true);

  //   // Check if the calibration returns the same instrument as we put in
  //   g_log.notice() << "-- validate calibration output\n";
  //   TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

  //   // Cleanup
  //   doCleanup();
  // }

  // /**
  //  * @brief rotate a single panel
  //  *
  //  */
  // void test_Single_Panel_Rotate() {
  //   g_log.notice() << "test: !Single Panel Rotate!\n";

  //   // prescribed rotate
  //   double drotx = boost::math::constants::euler<double>() / 3;
  //   double droty = boost::math::constants::ln_ln_two<double>() / 3;
  //   double drotz = boost::math::constants::pi_minus_three<double>() / 3;

  //   // Generate unique temp files
  //   auto isawFile = boost::filesystem::temp_directory_path();
  //   isawFile /=
  //       boost::filesystem::unique_path("unoPanelRotate_%%%%%%%%.DetCal");
  //   auto xmlFile = boost::filesystem::temp_directory_path();
  //   xmlFile /= boost::filesystem::unique_path("unoPanelRotate_%%%%%%%%.xml");

  //   g_log.notice() << "-- generate simulated workspace\n";
  //   MatrixWorkspace_sptr ws = m_ws->clone();
  //   MatrixWorkspace_sptr wsraw = ws->clone();

  //   // adjust xcenter
  //   g_log.notice() << "-- rotate bank12 (x center) by\n"
  //                  << "    drotx@(100) = " << drotx << "\n"
  //                  << "    droty@(010) = " << droty << "\n"
  //                  << "    drotz@(001) = " << drotz << "\n";
  //   adjustComponent(0.0, 0.0, 0.0, drotx, droty, drotz, bank_xcenter, ws);

  //   g_log.notice() << "-- generate peaks\n";
  //   PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
  //   PeaksWorkspace_sptr pwsref = pws->clone();

  //   // Pretend we don't know the answer
  //   g_log.notice() << "-- reset instrument positions&orientations\n";
  //   g_log.notice() << "    * before reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";
  //   pws->setInstrument(wsraw->getInstrument());
  //   g_log.notice() << "    * after reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";

  //   // Perform the calibration
  //   g_log.notice() << "-- start calibration\n";
  //   runCalibration(isawFile.string(), xmlFile.string(), pws, false, false,
  //                  true);

  //   // Check if the calibration returns the same instrument as we put in
  //   g_log.notice() << "-- validate calibration output\n";
  //   TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

  //   // Cleanup
  //   doCleanup();
  // }

  // /**
  //  * @brief Test compound movement of panel
  //  *
  //  */
  // void test_Single_Panel_Move() {
  //   g_log.notice() << "test: !Single Panel move (translation and
  //   rotation)!\n";

  //   // prescribed shift
  //   double dx = boost::math::constants::euler<double>();
  //   double dy = boost::math::constants::ln_ln_two<double>();
  //   double dz = boost::math::constants::pi_minus_three<double>();

  //   // prescribed rotate
  //   double drotx = boost::math::constants::euler<double>() / 3;
  //   double droty = boost::math::constants::ln_ln_two<double>() / 3;
  //   double drotz = boost::math::constants::pi_minus_three<double>() / 3;

  //   // Generate unique temp files
  //   auto isawFile = boost::filesystem::temp_directory_path();
  //   isawFile /=
  //   boost::filesystem::unique_path("unoPanelMove_%%%%%%%%.DetCal"); auto
  //   xmlFile = boost::filesystem::temp_directory_path(); xmlFile /=
  //   boost::filesystem::unique_path("unoPanelMove_%%%%%%%%.xml");

  //   g_log.notice() << "-- generate simulated workspace\n";
  //   MatrixWorkspace_sptr ws = m_ws->clone();
  //   MatrixWorkspace_sptr wsraw = ws->clone();

  //   // Adjust panel
  //   g_log.notice() << "-- translate bank12 (x center) by (" << dx << "," <<
  //   dy
  //                  << "," << dz << ")\n";
  //   g_log.notice() << "-- rotate bank12 (x center) by\n"
  //                  << "    drotx@(100) = " << drotx << "\n"
  //                  << "    droty@(010) = " << droty << "\n"
  //                  << "    drotz@(001) = " << drotz << "\n";
  //   adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_xcenter, ws);

  //   g_log.notice() << "-- generate peaks\n";
  //   PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
  //   PeaksWorkspace_sptr pwsref = pws->clone();

  //   // Pretend we don't know the answer
  //   g_log.notice() << "-- reset instrument positions&orientations\n";
  //   g_log.notice() << "    * before reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";
  //   pws->setInstrument(wsraw->getInstrument());
  //   g_log.notice() << "    * after reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";

  //   // Perform the calibration
  //   g_log.notice() << "-- start calibration\n";
  //   runCalibration(isawFile.string(), xmlFile.string(), pws, false, false,
  //                  true);

  //   // Check if the calibration returns the same instrument as we put in
  //   g_log.notice() << "-- validate calibration output\n";
  //   TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

  //   // Cleanup
  //   doCleanup();
  // }

  // void test_Source_Single_Panel_Move() {
  //   g_log.notice() << "test: !Source (z translation) and Single Panel move "
  //                     "(translation and rotation)!\n";

  //   // prescribed shift of source
  //   const double dL1 = boost::math::constants::e<double>();

  //   // prescribed shift
  //   double dx = boost::math::constants::euler<double>();
  //   double dy = boost::math::constants::ln_ln_two<double>();
  //   double dz = boost::math::constants::pi_minus_three<double>();

  //   // prescribed rotate
  //   double drotx = boost::math::constants::euler<double>() / 3;
  //   double droty = boost::math::constants::ln_ln_two<double>() / 3;
  //   double drotz = boost::math::constants::pi_minus_three<double>() / 3;

  //   // Generate unique temp files
  //   auto isawFile = boost::filesystem::temp_directory_path();
  //   isawFile /=
  //       boost::filesystem::unique_path("sourceAndUnoPanelMove_%%%%%%%%.DetCal");
  //   auto xmlFile = boost::filesystem::temp_directory_path();
  //   xmlFile /=
  //       boost::filesystem::unique_path("sourceAndUnoPanelMove_%%%%%%%%.xml");

  //   g_log.notice() << "-- generate simulated workspace\n";
  //   MatrixWorkspace_sptr ws = m_ws->clone();
  //   MatrixWorkspace_sptr wsraw = ws->clone();

  //   g_log.notice() << "-- translate source by " << dL1 << "\n"
  //                  << "-- translate bank12 (x center) by (" << dx << "," <<
  //                  dy
  //                  << "," << dz << ")\n"
  //                  << "-- rotate bank12 (x center) by\n"
  //                  << "    drotx@(100) = " << drotx << "\n"
  //                  << "    droty@(010) = " << droty << "\n"
  //                  << "    drotz@(001) = " << drotz << "\n";
  //   adjustComponent(0.0, 0.0, dL1, 0.0, 0.0, 0.0,
  //                   ws->getInstrument()->getSource()->getName(), ws);
  //   adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_xcenter, ws);

  //   g_log.notice() << "-- generate peaks\n";
  //   PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
  //   PeaksWorkspace_sptr pwsref = pws->clone();

  //   // Pretend we don't know the answer
  //   g_log.notice() << "-- reset instrument positions&orientations\n";
  //   g_log.notice() << "    * before reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";
  //   pws->setInstrument(wsraw->getInstrument());
  //   g_log.notice() << "    * after reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";

  //   // Perform the calibration
  //   g_log.notice() << "-- start calibration\n";
  //   runCalibration(isawFile.string(), xmlFile.string(), pws, false, true,
  //   true);

  //   // Check if the calibration returns the same instrument as we put in
  //   g_log.notice() << "-- validate calibration output\n";
  //   TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

  //   // Cleanup
  //   doCleanup();
  // }

  // /**
  //  * @brief Moving two panels at the same time
  //  *
  //  */
  // void test_Duo_Panels_Move() {
  //   g_log.notice() << "test: !Duo Panel move (translation and rotation)!\n";

  //   // NOTE: using the same move for both panels
  //   // prescribed shift
  //   double dx = boost::math::constants::euler<double>();
  //   double dy = boost::math::constants::ln_ln_two<double>();
  //   double dz = boost::math::constants::pi_minus_three<double>();

  //   // prescribed rotate
  //   double drotx = boost::math::constants::euler<double>() / 3;
  //   double droty = boost::math::constants::ln_ln_two<double>() / 3;
  //   double drotz = boost::math::constants::pi_minus_three<double>() / 3;

  //   // Generate unique temp files
  //   auto isawFile = boost::filesystem::temp_directory_path();
  //   isawFile /=
  //   boost::filesystem::unique_path("duoPanelMove_%%%%%%%%.DetCal"); auto
  //   xmlFile = boost::filesystem::temp_directory_path(); xmlFile /=
  //   boost::filesystem::unique_path("duoPanelMove_%%%%%%%%.xml");

  //   g_log.notice() << "-- generate simulated workspace\n";
  //   MatrixWorkspace_sptr ws = m_ws->clone();
  //   MatrixWorkspace_sptr wsraw = ws->clone();

  //   g_log.notice() << "-- translate bank12 (x center) by (" << dx << "," <<
  //   dy
  //                  << "," << dz << ")\n"
  //                  << "-- rotate bank12 (x center) by\n"
  //                  << "    drotx@(100) = " << drotx << "\n"
  //                  << "    droty@(010) = " << droty << "\n"
  //                  << "    drotz@(001) = " << drotz << "\n"
  //                  << "-- translate bank88 (y top) by (" << dx << "," << dy
  //                  << "," << dz << ")\n"
  //                  << "-- rotate bank88 (y top) by\n"
  //                  << "    drotx@(100) = " << drotx << "\n"
  //                  << "    droty@(010) = " << droty << "\n"
  //                  << "    drotz@(001) = " << drotz << "\n";

  //   adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_xcenter, ws);
  //   adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_ytop, ws);

  //   g_log.notice() << "-- generate peaks\n";
  //   PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
  //   PeaksWorkspace_sptr pwsref = pws->clone();

  //   // Pretend we don't know the answer
  //   g_log.notice() << "-- reset instrument positions&orientations\n";
  //   g_log.notice() << "    * before reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";
  //   pws->setInstrument(wsraw->getInstrument());
  //   g_log.notice() << "    * after reset L1 = "
  //                  << pws->getInstrument()->getSource()->getPos().Z() <<
  //                  "\n";

  //   // Perform the calibration
  //   g_log.notice() << "-- start calibration\n";
  //   runCalibration(isawFile.string(), xmlFile.string(), pws, false, true,
  //   true);

  //   // Check if the calibration returns the same instrument as we put in
  //   g_log.notice() << "-- validate calibration output\n";
  //   TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

  //   // Cleanup
  //   doCleanup();
  // }

  /**
   * @brief Everything goes
   *
   */
  void test_Exec() {
    g_log.notice()
        << "test: !multi components move (translation and rotation)!\n";

    // prescribed shift of source
    const double dL1 = boost::math::constants::e<double>();

    // prescribed shift
    double dx = boost::math::constants::euler<double>();
    double dy = boost::math::constants::ln_ln_two<double>();
    double dz = boost::math::constants::pi_minus_three<double>();

    // prescribed rotate
    double drotx = boost::math::constants::euler<double>() / 3;
    double droty = boost::math::constants::ln_ln_two<double>() / 3;
    double drotz = boost::math::constants::pi_minus_three<double>() / 3;

    // Generate unique temp files
    auto isawFile = boost::filesystem::temp_directory_path();
    isawFile /= boost::filesystem::unique_path("duoPanelMove_%%%%%%%%.DetCal");
    auto xmlFile = boost::filesystem::temp_directory_path();
    xmlFile /= boost::filesystem::unique_path("duoPanelMove_%%%%%%%%.xml");

    g_log.notice() << "-- generate simulated workspace\n";
    MatrixWorkspace_sptr ws = m_ws->clone();
    MatrixWorkspace_sptr wsraw = ws->clone();

    g_log.notice() << "-- translate source by " << dL1 << "\n"
                   << "-- for x(top,center,bottom) - bank(73,12,11)\n"
                   << "       y(right,left,top,bottom) - bank(59,58,88,26)\n"
                   << "   translate by (" << dx << "," << dy << "," << dz
                   << ")\n"
                   << "   rotate by\n"
                   << "    drotx@(100) = " << drotx << "\n"
                   << "    droty@(010) = " << droty << "\n"
                   << "    drotz@(001) = " << drotz << "\n";

    adjustComponent(0.0, 0.0, dL1, 0.0, 0.0, 0.0,
                    ws->getInstrument()->getSource()->getName(), ws);
    adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_xtop, ws);
    adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_xcenter, ws);
    adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_xbottom, ws);
    adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_yright, ws);
    adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_yleft, ws);
    adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_ytop, ws);
    adjustComponent(dx, dy, dz, drotx, droty, drotz, bank_ybottom, ws);

    g_log.notice() << "-- generate peaks\n";
    PeaksWorkspace_sptr pws = generateSimulatedPeaksWorkspace(ws);
    PeaksWorkspace_sptr pwsref = pws->clone();

    // Pretend we don't know the answer
    g_log.notice() << "-- reset instrument positions&orientations\n";
    g_log.notice() << "    * before reset L1 = "
                   << pws->getInstrument()->getSource()->getPos().Z() << "\n";
    pws->setInstrument(wsraw->getInstrument());
    g_log.notice() << "    * after reset L1 = "
                   << pws->getInstrument()->getSource()->getPos().Z() << "\n";

    // Perform the calibration
    g_log.notice() << "-- start calibration\n";
    runCalibration(isawFile.string(), xmlFile.string(), pws, false, true, true);

    // Check if the calibration returns the same instrument as we put in
    g_log.notice() << "-- validate calibration output\n";
    TS_ASSERT(validateCalibrationResults(pwsref, wsraw, xmlFile.string()));

    // Cleanup
    doCleanup();
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
    IAlgorithm_sptr csws_alg =
        AlgorithmFactory::Instance().create("CreateSimulationWorkspace", 1);
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
    sub_alg->setProperty("u", "1,0,0");
    sub_alg->setProperty("v", "0,1,0");
    sub_alg->setProperty("a", silicon_a);
    sub_alg->setProperty("b", silicon_b);
    sub_alg->setProperty("c", silicon_c);
    sub_alg->setProperty("alpha", silicon_alpha);
    sub_alg->setProperty("beta", silicon_beta);
    sub_alg->setProperty("gamma", silicon_gamma);
    sub_alg->execute();
    TS_ASSERT(sub_alg->isExecuted());

    MatrixWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsname);

    auto &sample = ws->mutableSample();
    sample.setCrystalStructure(silicon_cs);

    return ws;
  }

  /**
   * @brief populate peaks for the post adjustment simulated workspace
   * 
   * @return PeaksWorkspace_sptr 
   */
  PeaksWorkspace_sptr generateSimulatedPeaksWorkspace(MatrixWorkspace_sptr ws) {
    // prepare the algs pointer
    IAlgorithm_sptr sg_alg =
        AlgorithmFactory::Instance().create("SetGoniometer", 1);
    IAlgorithm_sptr pp_alg =
        AlgorithmFactory::Instance().create("PredictPeaks", 1);
    IAlgorithm_sptr cpw_alg =
        AlgorithmFactory::Instance().create("CombinePeaksWorkspaces", 1);

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

  void adjustComponent(double dx, double dy, double dz, double drotx,
                       double droty, double drotz, std::string cmptName,
                       MatrixWorkspace_sptr ws) {
    // translation
    IAlgorithm_sptr mv_alg = Mantid::API::AlgorithmFactory::Instance().create(
        "MoveInstrumentComponent", -1);
    mv_alg->initialize();
    mv_alg->setLogging(LOGCHILDALG);
    mv_alg->setProperty("Workspace", ws);
    mv_alg->setProperty("ComponentName", cmptName);
    mv_alg->setProperty("X", dx);
    mv_alg->setProperty("Y", dy);
    mv_alg->setProperty("Z", dz);
    mv_alg->setProperty("RelativePosition", true);
    mv_alg->executeAsChildAlg();

    // orientation
    IAlgorithm_sptr rot_alg = Mantid::API::AlgorithmFactory::Instance().create(
        "RotateInstrumentComponent", -1);
    //-- rotAngX@(1,0,0)
    rot_alg->initialize();
    rot_alg->setLogging(LOGCHILDALG);
    rot_alg->setProperty("Workspace", ws);
    rot_alg->setProperty("ComponentName", cmptName);
    rot_alg->setProperty("X", 1.0);
    rot_alg->setProperty("Y", 0.0);
    rot_alg->setProperty("Z", 0.0);
    rot_alg->setProperty("Angle", drotx);
    rot_alg->setProperty("RelativeRotation", true);
    rot_alg->executeAsChildAlg();
    //-- rotAngY@(0,1,0)
    rot_alg->initialize();
    rot_alg->setLogging(LOGCHILDALG);
    rot_alg->setProperty("Workspace", ws);
    rot_alg->setProperty("ComponentName", cmptName);
    rot_alg->setProperty("X", 0.0);
    rot_alg->setProperty("Y", 1.0);
    rot_alg->setProperty("Z", 0.0);
    rot_alg->setProperty("Angle", droty);
    rot_alg->setProperty("RelativeRotation", true);
    rot_alg->executeAsChildAlg();
    //-- rotAngZ@(0,0,1)
    rot_alg->initialize();
    rot_alg->setLogging(LOGCHILDALG);
    rot_alg->setProperty("Workspace", ws);
    rot_alg->setProperty("ComponentName", cmptName);
    rot_alg->setProperty("X", 0.0);
    rot_alg->setProperty("Y", 0.0);
    rot_alg->setProperty("Z", 1.0);
    rot_alg->setProperty("Angle", drotz);
    rot_alg->setProperty("RelativeRotation", true);
    rot_alg->executeAsChildAlg();
  }

  /**
   * @brief Run the calibration algorithm
   *
   * @param isawFilename
   * @param xmlFilename
   */
  void runCalibration(const std::string &isawFilename,
                      const std::string &xmlFilename, PeaksWorkspace_sptr pws,
                      bool calibrateT0, bool calibrateL1, bool calibrateBanks) {
    SCDCalibratePanels2 alg;
    alg.initialize();
    alg.setProperty("PeakWorkspace", pws);
    alg.setProperty("a", silicon_a);
    alg.setProperty("b", silicon_b);
    alg.setProperty("c", silicon_c);
    alg.setProperty("alpha", silicon_alpha);
    alg.setProperty("beta", silicon_beta);
    alg.setProperty("gamma", silicon_gamma);
    alg.setProperty("CalibrateT0", calibrateT0);
    alg.setProperty("CalibrateL1", calibrateL1);
    alg.setProperty("CalibrateBanks", calibrateBanks);
    alg.setProperty("DetCalFilename", isawFilename);
    alg.setProperty("XmlFilename", xmlFilename);
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
  bool validateCalibrationResults(PeaksWorkspace_sptr refpws,
                                  MatrixWorkspace_sptr refws,
                                  const std::string &xmlFileName) {
    // Adjust components in reference workspace using calibration results
    IAlgorithm_sptr lpf_alg =
        AlgorithmFactory::Instance().create("LoadParameterFile", 1);
    lpf_alg->initialize();
    lpf_alg->setLogging(LOGCHILDALG);
    lpf_alg->setProperty("Workspace", refws);
    lpf_alg->setProperty("Filename", xmlFileName);
    TS_ASSERT(lpf_alg->execute());

    // compare each bank
    // -- get the names
    boost::container::flat_set<std::string> BankNames;
    for (int i = 0; i < refpws->getNumberPeaks(); ++i) {
      std::string bname = refpws->getPeak(i).getBankName();
      if (bname != "None")
        BankNames.insert(bname);
    }
    // -- perform per bank comparison
    Instrument_sptr inst1 = std::const_pointer_cast<Instrument>(
        refws->getInstrument()); // based on calibration
    Instrument_sptr inst2 = std::const_pointer_cast<Instrument>(
        refpws->getInstrument()); // reference one

    for (auto bankname : BankNames) {
      if (!compareComponent(inst1, inst2, bankname)) {
        g_log.error() << "--" << bankname << " mismatch\n";
        return false;
      }
    }

    // all banks are the same, now the source check will make the call
    if (!compareComponent(inst1, inst2, inst1->getSource()->getName())) {
        g_log.error() << "-- " << inst1->getSource()->getName()
                      << " mismatch\n";
        return false;
      }

    return true;
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
  bool compareComponent(std::shared_ptr<Instrument> & instr1,
                        std::shared_ptr<Instrument> & instr2,
                        std::string componentName) {

    std::shared_ptr<const IComponent> cmpt1 =
        instr1->getComponentByName(componentName);
    std::shared_ptr<const IComponent> cmpt2 =
        instr2->getComponentByName(componentName);

    V3D p1 = cmpt1->getRelativePos();
    V3D p2 = cmpt2->getRelativePos();

    Quat q1 = cmpt1->getRelativeRot();
    Quat q2 = cmpt2->getRelativeRot();
    std::vector<double> r1 = q1.getEulerAngles("XYZ");
    std::vector<double> r2 = q2.getEulerAngles("XYZ");

    return  (std::abs(p1.X() - p2.X()) < TOLERANCE_L) &&
            (std::abs(p1.Y() - p2.Y()) < TOLERANCE_L) &&
            (std::abs(p1.Z() - p2.Z()) < TOLERANCE_L) &&
            (std::abs(r1[0] - r2[0]) < TOLERANCE_R) &&
            (std::abs(r1[1] - r2[1]) < TOLERANCE_R) &&
            (std::abs(r1[2] - r2[2]) < TOLERANCE_R);
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
  const bool LOGCHILDALG; // whether to show individual alg log
};
