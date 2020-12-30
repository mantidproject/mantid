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

  void testName() {
    SCDCalibratePanels2 alg;
    TS_ASSERT_EQUALS(alg.name(), "SCDCalibratePanels2");
  }

  void testInit(){
    SCDCalibratePanels2 alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  /**
   * @brief Trivial case where all components are in ideal/starting position
   *        Therefore the calibration results should be close to a zero
   *        vector.
   * 
   * Note: doubled as a quick test for two writters
   * 
   */
  void testNullCase(){
    g_log.notice() << "testNullCase() Start \n";

    SCDCalibratePanels2 alg;
    const std::string wsname("ws_nullcase");
    const std::string pwsname("pws_nullcase");
    auto isawFilename = boost::filesystem::temp_directory_path();
    isawFilename /= boost::filesystem::unique_path("nullcase_%%%%%%%%.DetCal");
    auto xmlFilename = boost::filesystem::temp_directory_path();
    xmlFilename /= boost::filesystem::unique_path("nullcase_%%%%%%%%.xml");

    generateSimulatedworkspace(wsname);
    MatrixWorkspace_sptr ws =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsname);
    MatrixWorkspace_sptr wsraw = ws->clone();

    // trivial case, no component undergoes any affine transformation
    generateSimulatedPeaks(wsname, pwsname);

    // we need to reset the instrument def for the peakworkspace as it
    // already contains the correct answer, therefore the optimization
    // will always return at 0.
    // set the pws instrument to the one
    PeaksWorkspace_sptr pws =
      AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(pwsname);
    PeaksWorkspace_sptr pwsref = pws->clone();
    // resetInstrument(pwsname);
    pws->setInstrument(wsraw->getInstrument());
    // Perform the calibration
    alg.initialize();
    alg.setProperty("PeakWorkspace", pwsname);
    alg.setProperty("a", silicon_a);
    alg.setProperty("b", silicon_b);
    alg.setProperty("c", silicon_c);
    alg.setProperty("alpha", silicon_alpha);
    alg.setProperty("beta", silicon_beta);
    alg.setProperty("gamma", silicon_gamma);
    alg.setProperty("CalibrateT0", false);
    alg.setProperty("CalibrateL1", true);
    alg.setProperty("CalibrateBanks", true);
    alg.setProperty("DetCalFilename", isawFilename.string());
    alg.setProperty("XmlFilename", xmlFilename.string());
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    // Check if the calibration returns the same instrument as we put in
    g_log.notice() << "-- validate calibration output\n";
    TS_ASSERT(CompareInstrument(pwsref, xmlFilename.string()));
  }

  /**
   * @brief Single variant case where only global var is adjusted.
   * 
   * NOTE: technically we should also check T0, but the client, CORELLI
   *       team does not seem to care about using T0, therefore we are
   *       not implmenting T0 calibration here.
   * 
   */
  void testGlobalShiftOnly(){
    g_log.notice() << "testGlobalShiftOnly() start \n";

    SCDCalibratePanels2 alg;
    const std::string wsname("ws_changeL1");
    const std::string pwsname("pws_changeL1");
    const double dL1 = boost::math::constants::e<double>();

    // prepare a workspace
    generateSimulatedworkspace(wsname);

    // move moderator along z by dL1
    moveModerator(wsname, dL1);

    // generate the peak workspace from shifted configuration
    generateSimulatedPeaks(wsname, pwsname);

    //TODO: run the calibration and check if we can backout the
    //      the correct L1
  }

  /**
   * @brief Move a single bank in the high order zone to see if the
   *        calibration can backout the correct shift vector.
   * 
   */
  void testSinglePanelMoved(){
    g_log.notice() << "testSinglePanelMoved() start\n";

    SCDCalibratePanels2 alg;
    const std::string wsname("ws_moveBank");
    const std::string pwsname("pws_moveBank");
    const double dx = boost::math::constants::euler<double>();
    const double dy = boost::math::constants::ln_ln_two<double>();
    const double dz = boost::math::constants::pi_minus_three<double>();

    // prepare a workspace 
    generateSimulatedworkspace(wsname);

    // move x center bank
    moveBank(wsname, bank_xcenter, dx, dy, dz);

    // generate the peak workspace from shifted configuration
    generateSimulatedPeaks(wsname, pwsname);

    //TODO: run the calibration and check if we can dv back

  }

  /**
   * @brief Test moving/translating two panels at the same time
   * 
   */
  void testDualPanelMoved(){
    g_log.notice() << "testDualPanelMoved() start\n";

    SCDCalibratePanels2 alg;
    const std::string wsname("ws_moveBanks");
    const std::string pwsname("pws_moveBanks");
    // for bank_xcenter
    const double dx1 = boost::math::constants::euler<double>();
    const double dy1 = boost::math::constants::ln_ln_two<double>();
    const double dz1 = boost::math::constants::pi_minus_three<double>();
    // for bank_yright
    const double dx2 = boost::math::constants::ln_ln_two<double>();
    const double dy2 = boost::math::constants::pi_minus_three<double>();
    const double dz2 = boost::math::constants::euler<double>();

    // prepare a workspace 
    generateSimulatedworkspace(wsname);

    // move banks
    moveBank(wsname, bank_xcenter, dx1, dy1, dz1);
    moveBank(wsname, bank_yright, dx2, dy2, dz2);

    // generate the peak workspace from shifted configuration
    generateSimulatedPeaks(wsname, pwsname);

    // TODO: run through calibrator and validate

  }

  /**
   * @brief Test using case with all seven banks twiddled plus
   *        the moderator shifted
   * 
   */
  void testExec(){
    g_log.notice() << "testExec() start\n";

    SCDCalibratePanels2 alg;
    const std::string wsname("ws_moveAll");
    const std::string pwsname("pws_moveAll");

    // prepare a workspace 
    generateSimulatedworkspace(wsname);

    // specify the movement of banks

    // generate the peak workspace from shifted configuration
    generateSimulatedPeaks(wsname, pwsname);

    // TODO: run through calibrator and validate

  }

private:
  // bank&panel names selected for testing
  // batch_1: high order zone selection
  const std::string bank_xtop    {"bank73/sixteenpack"};
  const std::string bank_xcenter {"bank12/sixteenpack"};
  const std::string bank_ybotoom {"bank11/sixteenpack"};
  // batch_2: low order zone selection
  // NOTE: limited reflections from experiment, often
  //       considered as a chanllegening case
  const std::string bank_yright  {"bank59/sixteenpack"};
  const std::string bank_yleft   {"bank58/sixteenpack"};
  const std::string bank_ytop    {"bank88/sixteenpack"};
  const std::string bank_ybottom {"bank26/sixteenpack"};

  // lattice constants of silicon
  const double silicon_a = 5.431;
  const double silicon_b = 5.431;
  const double silicon_c = 5.431;
  const double silicon_alpha = 90;
  const double silicon_beta = 90;
  const double silicon_gamma = 90;

  // silicon crystal structure
  const CrystalStructure silicon_cs = CrystalStructure(
    "5.431 5.431 5.431", 
    "F d -3 m",
    "Si 0 0 0 1.0 0.02");

  // constants that select the recriprocal space
  const double dspacing_min = 1.0;
  const double dspacing_max = 10.0;
  const double wavelength_min = 0.8;
  const double wavelength_max = 2.9;
  const double omega_step = 3.0;

  // check praramerter
  const double TOLERANCE_L = 0.001;  // distance
  const double TOLERANCE_R = 0.1;    // rotation angle

  /**
   * @brief Generate a generic workspace using silicon as the single
   *        crystal sample
   * 
   * @param WSName 
   */
  void generateSimulatedworkspace(const std::string &WSName){
    // create simulated workspace
    IAlgorithm_sptr csws_alg = 
      AlgorithmFactory::Instance().create("CreateSimulationWorkspace", 1);
    csws_alg->initialize();
    csws_alg->setProperty("Instrument", "CORELLI");
    csws_alg->setProperty("BinParams", "1,100,10000");
    csws_alg->setProperty("UnitX", "TOF");
    csws_alg->setProperty("OutputWorkspace", WSName);
    csws_alg->execute();
    TS_ASSERT(csws_alg->isExecuted());

    MatrixWorkspace_sptr ws = 
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(WSName);

    // set UB
    IAlgorithm_sptr sub_alg = 
      AlgorithmFactory::Instance().create("SetUB", 1);
    sub_alg->initialize();
    sub_alg->setProperty("Workspace", WSName);
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

    auto& sample = ws->mutableSample();
    sample.setCrystalStructure(silicon_cs);

  }

  /**
   * @brief Move the source ("moderator") of given workspace by
   *        deltaL1
   * 
   * @param WSName 
   * @param deltaL1 
   */
  void moveModerator(const std::string &WSName, double deltaL1){
    // NOTE:
    // We could reuse the moveBank func here, but it is better
    // to make this a separate func due to the significant difference
    // between a moderator and banks (in reality)
    IAlgorithm_sptr mv_alg = 
      AlgorithmFactory::Instance().create("MoveInstrumentComponent", 1);
    mv_alg->initialize();
    mv_alg->setProperty("Workspace", WSName);
    mv_alg->setProperty("ComponentName", "moderator");
    mv_alg->setProperty("X", 0.0);
    mv_alg->setProperty("Y", 0.0);
    mv_alg->setProperty("Z", deltaL1);
    mv_alg->setProperty("RelativePosition", true);
    mv_alg->execute();
    TS_ASSERT(mv_alg->isExecuted());
  }

  /**
   * @brief Move the selected Bank by given delta along x, y, z
   * 
   * @param WSName 
   * @param BankName 
   * @param deltaX 
   * @param deltaY 
   * @param deltaZ 
   */
  void moveBank(
    const std::string &WSName, 
    const std::string &BankName,
    double deltaX, double deltaY, double deltaZ){
    IAlgorithm_sptr mv_alg = 
      AlgorithmFactory::Instance().create("MoveInstrumentComponent", 1);
    mv_alg->initialize();
    mv_alg->setProperty("Workspace", WSName);
    mv_alg->setProperty("ComponentName", BankName);
    mv_alg->setProperty("X", deltaX);
    mv_alg->setProperty("Y", deltaY);
    mv_alg->setProperty("Z", deltaZ);
    mv_alg->setProperty("RelativePosition", true);
    mv_alg->execute();
    TS_ASSERT(mv_alg->isExecuted());
  }

  /**
   * @brief Rotate the selected bank by
   *  deltaRX around (1,0,0)_lab in degree
   *  deltaRY around (0,1,0)_lab in degree
   *  deltaRZ around (0,0,1)_lab in degree
   * 
   * @param WSName 
   * @param BankName 
   * @param deltaRX 
   * @param deltaRY 
   * @param deltaRZ 
   */
  void rotBank(
    const std::string &WSName,
    const std::string &BankName,
    double deltaRX, double deltaRY, double deltaRZ){
    IAlgorithm_sptr rot_alg = 
      AlgorithmFactory::Instance().create("RotateInstrumentComponent", 1);
    rot_alg->initialize();
    rot_alg->setProperty("Workspace", WSName);
    rot_alg->setProperty("ComponentName", BankName);

    // rotate around lab x by deltaRX
    rot_alg->setProperty("X", 1);
    rot_alg->setProperty("Y", 0);
    rot_alg->setProperty("Z", 0);
    rot_alg->setProperty("Angle", deltaRX);
    rot_alg->setProperty("RelativeRotation", true);
    rot_alg->execute();

    // rotate around lab y by deltaRY
    rot_alg->setProperty("X", 0);
    rot_alg->setProperty("Y", 1);
    rot_alg->setProperty("Z", 0);
    rot_alg->setProperty("Angle", deltaRY);
    rot_alg->setProperty("RelativeRotation", true);
    rot_alg->execute();

    // rotate around lab z by deltaRZ
    rot_alg->setProperty("X", 0);
    rot_alg->setProperty("Y", 0);
    rot_alg->setProperty("Z", 1);
    rot_alg->setProperty("Angle", deltaRZ);
    rot_alg->setProperty("RelativeRotation", true);
    rot_alg->execute();
  }

  /**
   * @brief Generate peak workspace with provided workspace
   * 
   * @param WSName 
   * @param PWSName
   */
  void generateSimulatedPeaks(const std::string &WSName, const std::string &PWSName){
    const std::string tmpPWSName{"_tmpPWS"};

    // NOTE:
    // AlgorithmFactory cannot found CreatePeaksWorkspace when doing CXX Unittest.
    // But its Python counterpart can, which is why the unittest here looks slightly
    // different from the corresponding Python demo.

    // prepare the algs pointer
    IAlgorithm_sptr sg_alg = 
        AlgorithmFactory::Instance().create("SetGoniometer", 1);
    IAlgorithm_sptr pp_alg = 
        AlgorithmFactory::Instance().create("PredictPeaks", 1);
    IAlgorithm_sptr cpw_alg = 
        AlgorithmFactory::Instance().create("CombinePeaksWorkspaces", 1);

    // generate peaks for a range of omega values
    for(double omega=0; omega<=180; omega=omega+omega_step){
      std::ostringstream os;
      os << omega << ",0,1,0,1";

      // set the SetGoniometer
      sg_alg->initialize();
      sg_alg->setProperty("Workspace", WSName);
      sg_alg->setProperty("Axis0", os.str());
      sg_alg->execute();

      // predict peak positions
      pp_alg->initialize();
      pp_alg->setProperty("InputWorkspace", WSName);
      pp_alg->setProperty("WavelengthMin", wavelength_min);
      pp_alg->setProperty("wavelengthMax", wavelength_max);
      pp_alg->setProperty("MinDSpacing", dspacing_min);
      pp_alg->setProperty("MaxDSpacing", dspacing_max);
      pp_alg->setProperty("ReflectionCondition", "All-face centred");

      if (omega < omega_step){
        pp_alg->setProperty("OutputWorkspace", PWSName);
        pp_alg->execute();
      } else {
        pp_alg->setProperty("OutputWorkspace", tmpPWSName);
        pp_alg->execute();

        // add the peaks to output peaks workspace
        cpw_alg->initialize();
        cpw_alg->setProperty("LHSWorkspace", tmpPWSName);
        cpw_alg->setProperty("RHSWorkspace", PWSName);
        cpw_alg->setProperty("OutputWorkspace", PWSName);
        cpw_alg->execute();
      }
      
    }
  }

  /**
   * @brief Check if the calibrated instrument is the same as the input reference
   * 
   * @param WSName
   * @param xmlFileName
   * 
   * NOTE: There is no easy way to extract the relative affine transformation with
   *       respect to the ideal position for each instruments, therefore we can not
   *       use the delta input values to check instrument.
   */
  bool CompareInstrument(
    std::shared_ptr<PeaksWorkspace> pws,
    const std::string &xmlFileName) {
    // blank innocent temp simulation workspace adjusted using calibration results
    const std::string wstmp("ws_tmp");
    generateSimulatedworkspace(wstmp);
    IAlgorithm_sptr lpf_alg = 
        AlgorithmFactory::Instance().create("LoadParameterFile", 1);
    lpf_alg->initialize();
    lpf_alg->setProperty("Workspace", wstmp);
    lpf_alg->setProperty("Filename", xmlFileName);
    TS_ASSERT(lpf_alg->execute());

    // compare each bank
    // -- get the names
    MatrixWorkspace_sptr ws = 
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wstmp);
    boost::container::flat_set<std::string> BankNames;
    int npeaks = static_cast<int>(pws->getNumberPeaks());
    for (int i=0; i<npeaks; ++i){
        std::string bname = pws->getPeak(i).getBankName();
        if (bname != "None")
            BankNames.insert(bname);
    }
    // -- perform per bank comparison
    Instrument_sptr inst1 = 
      std::const_pointer_cast<Instrument>(ws->getInstrument());   // based on calibration
    Instrument_sptr inst2 =
      std::const_pointer_cast<Instrument>(pws->getInstrument());  // reference one

    for (auto bankname : BankNames) {
      if (!compareBank(inst1, inst2, bankname))
        return false;
    }

    // all banks are the same, mark it as the same
    return true;
  }

  /**
   * @brief compare if two banks have similar translation and rotation
   * 
   * @param instr1 
   * @param instr2 
   * @param bn 
   * @return true 
   * @return false 
   */
  bool compareBank(
    std::shared_ptr<Instrument> &instr1,
    std::shared_ptr<Instrument> &instr2,
    std::string bn) {
      std::shared_ptr<const IComponent> bnk1 = instr1->getComponentByName(bn);
      std::shared_ptr<const IComponent> bnk2 = instr2->getComponentByName(bn);
      V3D p1 = bnk1->getRelativePos();
      V3D p2 = bnk2->getRelativePos();
      Quat q1 = bnk1->getRelativeRot();
      Quat q2 = bnk2->getRelativeRot();
      std::vector<double> r1 = q1.getEulerAngles("XYZ");
      std::vector<double> r2 = q2.getEulerAngles("XYZ");

      return (std::abs(p1.X() - p2.X()) < TOLERANCE_L) &&
             (std::abs(p1.Y() - p2.Y()) < TOLERANCE_L) &&
             (std::abs(p1.Z() - p2.Z()) < TOLERANCE_L) &&
             (std::abs(r1[0] - r2[0]) < TOLERANCE_R) &&
             (std::abs(r1[1] - r2[1]) < TOLERANCE_R) &&
             (std::abs(r1[2] - r2[2]) < TOLERANCE_R);
  }
};
