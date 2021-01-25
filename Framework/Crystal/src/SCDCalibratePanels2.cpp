// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidCrystal/SCDCalibratePanels2.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCrystal/SCDCalibratePanels2ObjFunc.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Logger.h"
#include <boost/container/flat_set.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace Mantid {
namespace Crystal {

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

/// Config logger
namespace {
Logger logger("SCDCalibratePanels2");
}

DECLARE_ALGORITHM(SCDCalibratePanels2)

/**
 * @brief Initialization
 *
 */
void SCDCalibratePanels2::init() {
  // Input peakworkspace
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                      "PeakWorkspace", "", Kernel::Direction::Input),
                  "Workspace of Indexed Peaks");

  // Lattice constant group
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  // NOTE:
  // Build serve does not allow a,b,c,alpha,beta,gamma as names, hence
  // the awkward naming here.
  //
  // Stacktrace
  // ConfigService-[Information] Unable to locate directory at:
  // /etc/mantid/instrument ConfigService-[Information] This is Mantid
  // version 5.1.1-2-rc.1 revision ge9b2c62 ConfigService-[Information] running
  // on ndw1457 starting 2021-01-18T01:51Z ConfigService-[Information]
  // Properties file(s) loaded: /opt/mantidunstable/bin/Mantid.properties,
  // /home/abc/.mantid/Mantid.user.properties ConfigService-[Information] Unable
  // to locate directory at: /etc/mantid/instrument FrameworkManager-[Notice]
  // Welcome to Mantid 5.1.1-2-rc.1 FrameworkManager-[Notice] Please cite:
  // http://dx.doi.org/10.1016/j.nima.2014.07.029 and this release:
  // http://dx.doi.org/10.5286/Software/Mantid5.1.1
  // FrameworkManager-[Information] Instrument updates disabled - cannot update
  // instrument definitions. FrameworkManager-[Information] Version check
  // disabled. SCDCalibratePanels(v2) property (a) violates conventions
  // SCDCalibratePanels(v2) property (b) violates conventions
  // SCDCalibratePanels(v2) property (c) violates conventions
  // SCDCalibratePanels(v2) property (alpha) violates conventions
  // SCDCalibratePanels(v2) property (beta) violates conventions
  // SCDCalibratePanels(v2) property (gamma) violates conventions
  // RESULT|iteration time_taken|1 0.15
  // RESULT|time_taken|0.15
  // Found 6 errors. Coding conventions found at
  // http://www.mantidproject.org/Mantid_Standards RESULT|memory footprint
  // increase|8.5703125
  //
  declareProperty("a", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter a (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("b", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter b (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("c", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter c (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("alpha", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter alpha in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  declareProperty("beta", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter beta in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  declareProperty("gamma", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter gamma in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  const std::string LATTICE("Lattice Constants");
  setPropertyGroup("a", LATTICE);
  setPropertyGroup("b", LATTICE);
  setPropertyGroup("c", LATTICE);
  setPropertyGroup("alpha", LATTICE);
  setPropertyGroup("beta", LATTICE);
  setPropertyGroup("gamma", LATTICE);

  // Calibration options group
  declareProperty("CalibrateT0", false, "Calibrate the T0 (initial TOF)");
  declareProperty("CalibrateL1", true,
                  "Change the L1(source to sample) distance");
  declareProperty("CalibrateBanks", true,
                  "Calibrate position and orientation of each bank.");
  // TODO:
  //  - add support to ignore edge pixels (EdgePixels)
  //  - add support for composite panels like SNAP (CalibrateSNAPPanels)
  //  - add support for calibration panels with non-standard size
  //  (ChangePanelSize)
  //     Once the core functionality of calibration is done, we can consider
  //     adding the following control calibration parameters.
  const std::string PARAMETERS("Calibration Parameters");
  setPropertyGroup("CalibrateT0", PARAMETERS);
  setPropertyGroup("CalibrateL1", PARAMETERS);
  setPropertyGroup("CalibrateBanks", PARAMETERS);

  // Output options group
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The workspace containing the calibration table.");
  const std::vector<std::string> detcalExts{".DetCal", ".Det_Cal"};
  declareProperty(
      std::make_unique<FileProperty>("DetCalFilename", "SCDCalibrate2.DetCal",
                                     FileProperty::OptionalSave, detcalExts),
      "Path to an ISAW-style .detcal file to save.");
  declareProperty(
      std::make_unique<FileProperty>("XmlFilename", "SCDCalibrate2.xml",
                                     FileProperty::OptionalSave, ".xml"),
      "Path to an Mantid .xml description(for LoadParameterFile) file to "
      "save.");
  declareProperty(
      std::make_unique<FileProperty>("CSVFilename", "SCDCalibrate2.csv",
                                     FileProperty::OptionalSave, ".csv"),
      "Path to an .csv file which contains the Calibration Table");
  // TODO:
  //  - add option to store intermedia calibration results for additional
  //    analysis if needed
  const std::string OUTPUT("Output");
  setPropertyGroup("OutputWorkspace", OUTPUT);
  setPropertyGroup("DetCalFilename", OUTPUT);
  setPropertyGroup("XmlFilename", OUTPUT);
  setPropertyGroup("CSVFilename", OUTPUT);

  // Add new section for advanced control of the calibration/optimization
  declareProperty("ToleranceOfTranslation", 5e-4, mustBePositive,
                  "Translations in meters found below this value will be set "
                  "to 0");
  declareProperty("ToleranceOfReorientation", 5e-2, mustBePositive,
                  "Reorientation (rotation) angles in degress found below "
                  "this value will be set to 0");
  declareProperty(
      "TranslationSearchRadius", 5e-2, mustBePositive,
      "This is the search radius when calibrating component translations "
      "using optimization. For CORELLI instrument, most panels will shift "
      "within 5cm, therefore the search radius is set to 5e-2.");
  declareProperty(
      "RotationSearchRadius", 5.0, mustBePositive,
      "This is the search radius when calibrating component orientations "
      "using optimization.  For CORELLI instrument, most panels will wobble "
      "within 5 degrees, therefore the default values is set to 5 here.");
  declareProperty(
      "SourceShiftSearchRadius", 0.1, mustBePositive,
      "This is the search radius when calibrating source shift, L1, using "
      "optimization.  For CORELLI instrument, the source shift is often "
      "within 10 cm, therefore the default value is set to 0.1.");
  declareProperty("VerboseOutput", false,
                  "Toggle of child algorithm console output.");
  // grouping into one category
  const std::string ADVCNTRL("AdvancedControl");
  setPropertyGroup("ToleranceOfTranslation", ADVCNTRL);
  setPropertyGroup("ToleranceOfReorientation", ADVCNTRL);
  setPropertyGroup("TranslationSearchRadius", ADVCNTRL);
  setPropertyGroup("RotationSearchRadius", ADVCNTRL);
  setPropertyGroup("SourceShiftSearchRadius", ADVCNTRL);
  setPropertyGroup("VerboseOutput", ADVCNTRL);
}

/**
 * @brief validate inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> SCDCalibratePanels2::validateInputs() {
  std::map<std::string, std::string> issues;

  // T0 calibration is not ready for production, raise an error
  // if requested by user
  bool calibrateT0 = getProperty("CalibrateT0");
  if (calibrateT0)
    issues["CalibrateT0"] = "Caliration of T0 is not ready for production, "
                            "please set it to False to continue";

  return issues;
}

/**
 * @brief execute calibration
 *
 */
void SCDCalibratePanels2::exec() {
  // parse all inputs
  PeaksWorkspace_sptr m_pws = getProperty("PeakWorkspace");

  parseLatticeConstant(m_pws);

  bool calibrateT0 = getProperty("CalibrateT0");
  bool calibrateL1 = getProperty("CalibrateL1");
  bool calibrateBanks = getProperty("CalibrateBanks");

  const std::string DetCalFilename = getProperty("DetCalFilename");
  const std::string XmlFilename = getProperty("XmlFilename");
  const std::string CSVFilename = getProperty("CSVFilename");

  // parsing advance control parameters
  m_tolerance_translation = getProperty("ToleranceOfTranslation");
  m_tolerance_rotation = getProperty("ToleranceOfReorientation");
  m_bank_translation_bounds = getProperty("TranslationSearchRadius");
  m_bank_rotation_bounds = getProperty("RotationSearchRadius");
  m_source_translation_bounds = getProperty("SourceShiftSearchRadius");
  LOGCHILDALG = getProperty("VerboseOutput");

  // STEP_0: sort the peaks
  std::vector<std::pair<std::string, bool>> criteria{{"BankName", true}};
  m_pws->sort(criteria);

  // STEP_1: preparation
  // get names of banks that can be calibrated
  getBankNames(m_pws);

  // STEP_2: optimize T0,L1,L2,etc.
  if (calibrateT0)
    optimizeT0(m_pws);
  if (calibrateL1)
    optimizeL1(m_pws);
  if (calibrateBanks)
    optimizeBanks(m_pws);

  // STEP_3: generate a table workspace to save the calibration results
  Instrument_sptr instCalibrated =
      std::const_pointer_cast<Geometry::Instrument>(m_pws->getInstrument());
  TableWorkspace_sptr tablews = generateCalibrationTable(instCalibrated);

  // STEP_4: Write to disk if required
  if (!XmlFilename.empty())
    saveXmlFile(XmlFilename, m_BankNames, instCalibrated);

  if (!DetCalFilename.empty())
    saveIsawDetCal(DetCalFilename, m_BankNames, instCalibrated, m_T0);

  if (!CSVFilename.empty())
    saveCalibrationTable(CSVFilename, tablews);

  // STEP_4: Cleanup
}

/// ------------------------------------------- ///
/// Core functions for Calibration&Optimizatoin ///
/// ------------------------------------------- ///

/**
 * @brief adjusting the deltaT0 to match the qSample_calculated and
 *        qSameple_measured
 *
 * @note this function currently only returns dT0=0, and the reason
 *       is still unkown.
 *
 * @param pws
 */
void SCDCalibratePanels2::optimizeT0(std::shared_ptr<PeaksWorkspace> pws) {
  // tl;dr; mocking a matrix workspace to store the target Qvectors
  // details
  //    The optimizer we are going to use is the Fit algorithm, which is
  //    designed to match one histogram with another target one.
  //    So what we are doing here is creating a fake histogram out of the
  //    qSamples so that Fit algorithm can take it in and start optimizing.
  //    The same goes for optimizeL1 and optimizeBanks.
  int npks = pws->getNumberPeaks();
  MatrixWorkspace_sptr t0ws = std::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create(
          "Workspace2D", // use workspace 2D to mock a histogram
          1,             // one vector
          3 * npks,      // X :: anything is fine
          3 * npks));    // Y :: flattened Q vector
  // setting values to t0ws
  auto &measured = t0ws->getSpectrum(0);
  auto &xv = measured.mutableX();
  auto &yv = measured.mutableY();
  auto &ev = measured.mutableE();

  for (int i = 0; i < npks; ++i) {
    const Peak &pk = pws->getPeak(i);
    V3D qv = pk.getQSampleFrame();

    for (int j = 0; j < 3; ++j) {
      xv[i * 3 + j] = i * 3 + j;
      yv[i * 3 + j] = qv[j];
      ev[i * 3 + j] = 1;
    }
  }

  // create child Fit alg to optimize T0
  IAlgorithm_sptr fitT0_alg = createChildAlgorithm("Fit", -1, -1, false);
  //-- obj func def
  //  dl;dr;
  //    Fit algorithm requires a IFunction1D to fit
  //  details
  //    Fit algorithm requires a class derived from IFunction1D as its
  //    input, so we have to implement the objective function as a separate
  //    class just to get Fit serving as an optimizer.
  //    For this particular case, we are constructing an objective function
  //    based on IFunction1D that outputs a fake histogram consist of
  //    qSample calucated based on perturbed instrument positions and
  //    orientations.
  std::ostringstream fun_str;
  fun_str << "name=SCDCalibratePanels2ObjFunc,Workspace=" << pws->getName()
          << ",ComponentName=none";
  //-- bounds&constraints def
  std::ostringstream tie_str;
  tie_str << "DeltaX=0.0,DeltaY=0.0,DeltaZ=0.0,Theta=1.0,Phi=0.0,"
             "DeltaRotationAngle=0.0";
  //-- set&go
  fitT0_alg->setPropertyValue("Function", fun_str.str());
  fitT0_alg->setProperty("Ties", tie_str.str());
  fitT0_alg->setProperty("InputWorkspace", t0ws);
  fitT0_alg->setProperty("CreateOutput", true);
  fitT0_alg->setProperty("Output", "fit");
  fitT0_alg->executeAsChildAlg();
  //-- parse output
  double chi2OverDOF = fitT0_alg->getProperty("OutputChi2overDoF");
  ITableWorkspace_sptr rst = fitT0_alg->getProperty("OutputParameters");
  double dT0_optimized = rst->getRef<double>("Value", 6);
  // update T0 for all peaks
  adjustT0(dT0_optimized, pws);

  //-- log
  g_log.notice() << "-- Fit T0 results using " << npks << " peaks:\n"
                 << "    dT0: " << dT0_optimized << " \n"
                 << "    chi2/DOF = " << chi2OverDOF << "\n";
}

/**
 * @brief
 *
 * @param pws
 */
void SCDCalibratePanels2::optimizeL1(std::shared_ptr<PeaksWorkspace> pws) {
  // cache starting L1 position
  double original_L1 = -pws->getInstrument()->getSource()->getPos().Z();
  int npks = pws->getNumberPeaks();
  MatrixWorkspace_sptr l1ws = std::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create(
          "Workspace2D", // use workspace 2D to mock a histogram
          1,             // one vector
          3 * npks,      // X :: anything is fine
          3 * npks));    // Y :: flattened Q vector

  auto &measured = l1ws->getSpectrum(0);
  auto &xv = measured.mutableX();
  auto &yv = measured.mutableY();
  auto &ev = measured.mutableE();

  for (int i = 0; i < npks; ++i) {
    const Peak &pk = pws->getPeak(i);
    V3D qv = pk.getQSampleFrame();
    for (int j = 0; j < 3; ++j) {
      xv[i * 3 + j] = i * 3 + j;
      yv[i * 3 + j] = qv[j];
      ev[i * 3 + j] = 1;
    }
  }

  // fit algorithm for the optimization of L1
  IAlgorithm_sptr fitL1_alg = createChildAlgorithm("Fit", -1, -1, false);
  //-- obj func def
  std::ostringstream fun_str;
  fun_str << "name=SCDCalibratePanels2ObjFunc,Workspace=" << pws->getName()
          << ",ComponentName=moderator";
  //-- bounds&constraints def
  std::ostringstream tie_str;
  tie_str << "DeltaX=0.0,DeltaY=0.0,Theta=1.0,Phi=0.0,DeltaRotationAngle=0.0,"
             "DeltaT0="
          << m_T0;
  //-- set and go
  fitL1_alg->setPropertyValue("Function", fun_str.str());
  fitL1_alg->setProperty("Ties", tie_str.str());
  fitL1_alg->setProperty("InputWorkspace", l1ws);
  fitL1_alg->setProperty("CreateOutput", true);
  fitL1_alg->setProperty("Output", "fit");
  fitL1_alg->executeAsChildAlg();
  //-- parse output
  double chi2OverDOF = fitL1_alg->getProperty("OutputChi2overDoF");
  ITableWorkspace_sptr rst = fitL1_alg->getProperty("OutputParameters");
  double dL1_optimized = rst->getRef<double>("Value", 2);
  adjustComponent(0.0, 0.0, dL1_optimized, 1.0, 0.0, 0.0, 0.0,
                  pws->getInstrument()->getSource()->getName(), pws);

  //-- log
  g_log.notice() << "-- Fit L1 results using " << npks << " peaks:\n"
                 << "    dL1: " << dL1_optimized << " \n"
                 << "    L1 " << original_L1 << " -> "
                 << -pws->getInstrument()->getSource()->getPos().Z() << " \n"
                 << "    chi2/DOF = " << chi2OverDOF << "\n";
}

/**
 * @brief Calibrate the position and rotation of each Bank, one at a time
 *
 * @param pws
 */
void SCDCalibratePanels2::optimizeBanks(std::shared_ptr<PeaksWorkspace> pws) {

  PARALLEL_FOR_IF(Kernel::threadSafe(*pws))
  for (int i = 0; i < static_cast<int>(m_BankNames.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    // prepare local copies to work with
    const std::string bankname = *std::next(m_BankNames.begin(), i);

    //-- step 0: extract peaks that lies on the current bank
    // NOTE: We are cloning the whole pws, then subtracting
    //       those that are not on the current bank.
    PeaksWorkspace_sptr pwsBanki = pws->clone();
    const std::string pwsBankiName = "_pws_" + bankname;
    AnalysisDataService::Instance().addOrReplace(pwsBankiName, pwsBanki);
    std::vector<Peak> &allPeaks = pwsBanki->getPeaks();
    auto notMyPeaks = std::remove_if(
        allPeaks.begin(), allPeaks.end(),
        [&bankname](const Peak &pk) { return pk.getBankName() != bankname; });
    allPeaks.erase(notMyPeaks, allPeaks.end());

    // Do not attempt correct panels with less than 6 peaks as the system will
    // be under-determined
    int nBankPeaks = pwsBanki->getNumberPeaks();
    if (nBankPeaks < MINIMUM_PEAKS_PER_BANK) {
      g_log.notice() << "-- Bank " << bankname << " have only " << nBankPeaks
                     << " (<" << MINIMUM_PEAKS_PER_BANK
                     << ") Peaks, skipping\n";
      continue;
    }

    //-- step 1: prepare a mocked workspace with QSample as its yValues
    MatrixWorkspace_sptr wsBankCali =
        std::dynamic_pointer_cast<MatrixWorkspace>(
            WorkspaceFactory::Instance().create(
                "Workspace2D",    // use workspace 2D to mock a histogram
                1,                // one vector
                3 * nBankPeaks,   // X :: anything is fine
                3 * nBankPeaks)); // Y :: flattened Q vector
    auto &measured = wsBankCali->getSpectrum(0);
    auto &xv = measured.mutableX();
    auto &yv = measured.mutableY();
    auto &ev = measured.mutableE();
    // TODO: non-uniform weighting (ev) will be implemented at a later date
    for (int i = 0; i < nBankPeaks; ++i) {
      const Peak &pk = pwsBanki->getPeak(i);
      V3D qv = pk.getQSampleFrame();
      for (int j = 0; j < 3; ++j) {
        xv[i * 3 + j] = i * 3 + j;
        yv[i * 3 + j] = qv[j];
        ev[i * 3 + j] = 1;
      }
    }

    //-- step 2&3: invoke fit to find both traslation and rotation
    IAlgorithm_sptr fitBank_alg = createChildAlgorithm("Fit", -1, -1, false);
    //---- setup obj fun def
    std::ostringstream fun_str;
    fun_str << "name=SCDCalibratePanels2ObjFunc,Workspace=" << pwsBankiName
            << ",ComponentName=" << bankname;
    //---- bounds&constraints def
    std::ostringstream tie_str;
    tie_str << "DeltaT0=" << m_T0;
    std::ostringstream constraint_str;
    double brb = std::abs(m_bank_rotation_bounds);
    constraint_str << "0.0<Theta<3.1415926,0<Phi<6.28318530718," << -brb
                   << "<DeltaRotationAngle<" << brb << ",";
    double btb = std::abs(m_bank_translation_bounds);
    constraint_str << -btb << "<DeltaX<" << btb << "," << -btb << "<DeltaY<"
                   << btb << "," << -btb << "<DeltaZ<" << btb;

    //---- set&go
    fitBank_alg->setPropertyValue("Function", fun_str.str());
    fitBank_alg->setProperty("Ties", tie_str.str());
    fitBank_alg->setProperty("Constraints", constraint_str.str());
    fitBank_alg->setProperty("InputWorkspace", wsBankCali);
    fitBank_alg->setProperty("CreateOutput", true);
    fitBank_alg->setProperty("Output", "fit");
    fitBank_alg->executeAsChildAlg();
    //---- cache results
    double chi2OverDOF = fitBank_alg->getProperty("OutputChi2overDoF");
    ITableWorkspace_sptr rstFitBank =
        fitBank_alg->getProperty("OutputParameters");
    double dx = rstFitBank->getRef<double>("Value", 0);
    double dy = rstFitBank->getRef<double>("Value", 1);
    double dz = rstFitBank->getRef<double>("Value", 2);
    double theta = rstFitBank->getRef<double>("Value", 3);
    double phi = rstFitBank->getRef<double>("Value", 4);
    double rotang = rstFitBank->getRef<double>("Value", 5);

    //-- step 4: update the instrument with optimization results
    //           if the fit results are above the tolerance/threshold
    std::string bn = bankname;
    if (pws->getInstrument()->getName().compare("CORELLI") == 0)
      bn.append("/sixteenpack");
    if ((std::abs(dx) < m_tolerance_translation) &&
        (std::abs(dy) < m_tolerance_translation) &&
        (std::abs(dz) < m_tolerance_translation) &&
        (std::abs(rotang) < m_tolerance_rotation)) {
      // skip the adjustment of the component as it is juat noise
      g_log.notice() << "-- Fit " << bn
                     << " results below tolerance, skippping\n";
    } else {
      double rvx = sin(theta) * cos(phi);
      double rvy = sin(theta) * sin(phi);
      double rvz = cos(theta);
      adjustComponent(dx, dy, dz, rvx, rvy, rvz, rotang, bn, pws);
      g_log.notice() << "-- Fit " << bn << " results using " << nBankPeaks
                     << " peaks:\n "
                     << "    d(x,y,z) = (" << dx << "," << dy << "," << dz
                     << ")\n"
                     << "    rotang(rx,ry,rz) =" << rotang << "(" << rvx << ","
                     << rvy << "," << rvz << ")\n"
                     << "    chi2/DOF = " << chi2OverDOF << "\n";
    }

    // -- cleanup
    AnalysisDataService::Instance().remove(pwsBankiName);
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/// ---------------- ///
/// helper functions ///
/// ---------------- ///

/**
 * @brief get lattice constants from either inputs or the
 *        input peak workspace
 *
 */
void SCDCalibratePanels2::parseLatticeConstant(
    std::shared_ptr<PeaksWorkspace> pws) {
  m_a = getProperty("a");
  m_b = getProperty("b");
  m_c = getProperty("c");
  m_alpha = getProperty("alpha");
  m_beta = getProperty("beta");
  m_gamma = getProperty("gamma");
  // if any one of the six lattice constants is missing, try to get
  // one from the workspace
  if ((m_a == EMPTY_DBL() || m_b == EMPTY_DBL() || m_c == EMPTY_DBL() ||
       m_alpha == EMPTY_DBL() || m_beta == EMPTY_DBL() ||
       m_gamma == EMPTY_DBL()) &&
      (pws->sample().hasOrientedLattice())) {
    OrientedLattice lattice = pws->mutableSample().getOrientedLattice();
    m_a = lattice.a();
    m_b = lattice.b();
    m_c = lattice.c();
    m_alpha = lattice.alpha();
    m_beta = lattice.beta();
    m_gamma = lattice.gamma();
  }
}

/**
 * @brief Gather names for bank for calibration
 *
 * @param pws
 */
void SCDCalibratePanels2::getBankNames(std::shared_ptr<PeaksWorkspace> pws) {
  int npeaks = static_cast<int>(pws->getNumberPeaks());
  for (int i = 0; i < npeaks; ++i) {
    std::string bname = pws->getPeak(i).getBankName();
    if (bname != "None")
      m_BankNames.insert(bname);
  }
}

/**
 * @brief shift T0 for both peakworkspace and all peaks
 *
 * @param dT0
 * @param pws
 */
void SCDCalibratePanels2::adjustT0(double dT0, PeaksWorkspace_sptr &pws) {
  // update the T0 record in peakworkspace
  Mantid::API::Run &run = pws->mutableRun();
  double T0 = 0.0;
  if (run.hasProperty("T0")) {
    T0 = run.getPropertyValueAsType<double>("T0");
  }
  T0 += dT0;
  run.addProperty<double>("T0", T0, true);

  // update wavelength of each peak using new T0
  for (int i = 0; i < pws->getNumberPeaks(); ++i) {
    Peak &pk = pws->getPeak(i);
    Units::Wavelength wl;
    wl.initialize(pk.getL1(), pk.getL2(), pk.getScattering(), 0,
                  pk.getInitialEnergy(), 0.0);
    pk.setWavelength(wl.singleFromTOF(pk.getTOF() + dT0));
  }
}

/**
 * @brief adjust instrument component position and orientation
 *
 * @param dx
 * @param dy
 * @param dz
 * @param rvx
 * @param rvy
 * @param rvz
 * @param rang
 * @param cmptName
 * @param pws
 */
void SCDCalibratePanels2::adjustComponent(
    double dx, double dy, double dz, double rvx, double rvy, double rvz,
    double rang, std::string cmptName, DataObjects::PeaksWorkspace_sptr &pws) {

  // orientation
  IAlgorithm_sptr rot_alg = Mantid::API::AlgorithmFactory::Instance().create(
      "RotateInstrumentComponent", -1);
  rot_alg->initialize();
  rot_alg->setChild(true);
  rot_alg->setLogging(LOGCHILDALG);
  rot_alg->setProperty<Workspace_sptr>("Workspace", pws);
  rot_alg->setProperty("ComponentName", cmptName);
  rot_alg->setProperty("X", rvx);
  rot_alg->setProperty("Y", rvy);
  rot_alg->setProperty("Z", rvz);
  rot_alg->setProperty("Angle", rang);
  rot_alg->setProperty("RelativeRotation", true);
  rot_alg->executeAsChildAlg();

  // translation
  IAlgorithm_sptr mv_alg = Mantid::API::AlgorithmFactory::Instance().create(
      "MoveInstrumentComponent", -1);
  mv_alg->initialize();
  mv_alg->setChild(true);
  mv_alg->setLogging(LOGCHILDALG);
  mv_alg->setProperty<Workspace_sptr>("Workspace", pws);
  mv_alg->setProperty("ComponentName", cmptName);
  mv_alg->setProperty("X", dx);
  mv_alg->setProperty("Y", dy);
  mv_alg->setProperty("Z", dz);
  mv_alg->setProperty("RelativePosition", true);
  mv_alg->executeAsChildAlg();
}

/**
 * @brief Generate a tableworkspace to store the calibration results
 *
 * @param instrument  :: calibrated instrument
 * @return DataObjects::TableWorkspace_sptr
 */
DataObjects::TableWorkspace_sptr SCDCalibratePanels2::generateCalibrationTable(
    std::shared_ptr<Geometry::Instrument> &instrument) {
  g_log.notice() << "Generate a TableWorkspace to store calibration results.\n";

  // Create table workspace
  ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();
  TableWorkspace_sptr tablews =
      std::dynamic_pointer_cast<TableWorkspace>(itablews);

  for (int i = 0; i < 8; ++i)
    tablews->addColumn(calibrationTableColumnTypes[i],
                       calibrationTableColumnNames[i]);

  // The first row is always the source
  IComponent_const_sptr source = instrument->getSource();
  V3D sourceRelPos = source->getRelativePos();
  Mantid::API::TableRow sourceRow = tablews->appendRow();
  // NOTE: source should not have any rotation, so we pass a zero
  //       rotation with a fixed axis
  sourceRow << instrument->getSource()->getName() << sourceRelPos.X()
            << sourceRelPos.Y() << sourceRelPos.Z() << 1.0 << 0.0 << 0.0 << 0.0;

  // Loop through banks and set row values
  for (auto bankName : m_BankNames) {
    // CORELLLI instrument has one extra layer that pack tubes into
    // banks, which is what we need here
    if (instrument->getName().compare("CORELLI") == 0)
      bankName.append("/sixteenpack");

    std::shared_ptr<const IComponent> bank =
        instrument->getComponentByName(bankName);

    Quat relRot = bank->getRelativeRot();
    V3D pos1 = bank->getRelativePos();

    // Calculate cosines using relRot
    double deg, xAxis, yAxis, zAxis;
    relRot.getAngleAxis(deg, xAxis, yAxis, zAxis);

    // Append a new row
    Mantid::API::TableRow bankRow = tablews->appendRow();
    // Row and positions
    bankRow << bankName << pos1.X() << pos1.Y() << pos1.Z() << xAxis << yAxis
            << zAxis << deg;
  }

  g_log.notice() << "finished generating tables\n";
  setProperty("OutputWorkspace", tablews);

  return tablews;
}

/**
 * Saves the new instrument to an xml file that can be used with the
 * LoadParameterFile Algorithm. If the filename is empty, nothing gets
 * done.
 *
 * @param FileName     The filename to save this information to
 *
 * @param AllBankNames The names of the banks in each group whose values
 * are to be saved to the file
 *
 * @param instrument   The instrument with the new values for the banks
 * in Groups
 *
 * TODO:
 *  - Need to find a way to add the information regarding calibrated T0
 */
void SCDCalibratePanels2::saveXmlFile(
    const std::string &FileName,
    boost::container::flat_set<std::string> &AllBankNames,
    std::shared_ptr<Instrument> &instrument) {
  g_log.notice() << "Generating xml tree"
                 << "\n";

  using boost::property_tree::ptree;
  ptree root;
  ptree parafile;

  // configure root node
  parafile.put("<xmlattr>.instrument", instrument->getName());
  parafile.put("<xmlattr>.valid-from",
               instrument->getValidFromDate().toISO8601String());

  // configure and add each bank
  for (auto bankName : AllBankNames) {
    // Prepare data for node
    if (instrument->getName().compare("CORELLI") == 0)
      bankName.append("/sixteenpack");

    std::shared_ptr<const IComponent> bank =
        instrument->getComponentByName(bankName);

    Quat relRot = bank->getRelativeRot();
    std::vector<double> relRotAngles = relRot.getEulerAngles("XYZ");
    V3D pos1 = bank->getRelativePos();
    // TODO: no handling of scaling for now, will add back later
    double scalex = 1.0;
    double scaley = 1.0;

    // prepare node
    ptree bank_root;
    ptree bank_dx, bank_dy, bank_dz;
    ptree bank_dx_val, bank_dy_val, bank_dz_val;
    ptree bank_drotx, bank_droty, bank_drotz;
    ptree bank_drotx_val, bank_droty_val, bank_drotz_val;
    ptree bank_sx, bank_sy;
    ptree bank_sx_val, bank_sy_val;

    // add data to node
    bank_dx_val.put("<xmlattr>.val", pos1.X());
    bank_dy_val.put("<xmlattr>.val", pos1.Y());
    bank_dz_val.put("<xmlattr>.val", pos1.Z());
    bank_dx.put("<xmlattr>.name", "x");
    bank_dy.put("<xmlattr>.name", "y");
    bank_dz.put("<xmlattr>.name", "z");

    bank_drotx_val.put("<xmlattr>.val", relRotAngles[0]);
    bank_droty_val.put("<xmlattr>.val", relRotAngles[1]);
    bank_drotz_val.put("<xmlattr>.val", relRotAngles[2]);
    bank_drotx.put("<xmlattr>.name", "rotx");
    bank_droty.put("<xmlattr>.name", "roty");
    bank_drotz.put("<xmlattr>.name", "rotz");

    bank_sx_val.put("<xmlattr>.val", scalex);
    bank_sy_val.put("<xmlattr>.val", scaley);
    bank_sx.put("<xmlattr>.name", "scalex");
    bank_sy.put("<xmlattr>.name", "scaley");

    bank_root.put("<xmlattr>.name", bankName);

    // configure structure
    bank_dx.add_child("value", bank_dx_val);
    bank_dy.add_child("value", bank_dy_val);
    bank_dz.add_child("value", bank_dz_val);

    bank_drotx.add_child("value", bank_drotx_val);
    bank_droty.add_child("value", bank_droty_val);
    bank_drotz.add_child("value", bank_drotz_val);

    bank_sx.add_child("value", bank_sx_val);
    bank_sy.add_child("value", bank_sy_val);

    bank_root.add_child("parameter", bank_drotx);

    bank_root.add_child("parameter", bank_droty);
    bank_root.add_child("parameter", bank_drotz);
    bank_root.add_child("parameter", bank_dx);
    bank_root.add_child("parameter", bank_dy);
    bank_root.add_child("parameter", bank_dz);
    bank_root.add_child("parameter", bank_sx);
    bank_root.add_child("parameter", bank_sy);

    parafile.add_child("component-link", bank_root);
  }

  // get L1 info for source
  ptree src;
  ptree src_dx, src_dy, src_dz;
  ptree src_dx_val, src_dy_val, src_dz_val;
  // -- get positional data from source
  IComponent_const_sptr source = instrument->getSource();
  V3D sourceRelPos = source->getRelativePos();
  // -- add date to node
  src_dx_val.put("<xmlattr>.val", sourceRelPos.X());
  src_dy_val.put("<xmlattr>.val", sourceRelPos.Y());
  src_dz_val.put("<xmlattr>.val", sourceRelPos.Z());
  src_dx.put("<xmlattr>.name", "x");
  src_dy.put("<xmlattr>.name", "y");
  src_dz.put("<xmlattr>.name", "z");
  src.put("<xmlattr>.name", source->getName());

  src_dx.add_child("value", src_dx_val);
  src_dy.add_child("value", src_dy_val);
  src_dz.add_child("value", src_dz_val);
  src.add_child("parameter", src_dx);
  src.add_child("parameter", src_dy);
  src.add_child("parameter", src_dz);

  parafile.add_child("component-link", src);

  // give everything to root
  root.add_child("parameter-file", parafile);
  // write the xml tree to disk
  g_log.notice() << "\tSaving parameter file as " << FileName << "\n";
  boost::property_tree::write_xml(
      FileName, root, std::locale(),
      boost::property_tree::xml_writer_settings<std::string>(' ', 2));
}

/**
 * Really this is the operator SaveIsawDetCal but only the results of the given
 * banks are saved.  L1 and T0 are also saved.
 *
 * @param filename     -The name of the DetCal file to save the results to
 * @param AllBankName  -the set of the NewInstrument names of the banks(panels)
 * @param instrument   -The instrument with the correct panel geometries
 *                      and initial path length
 * @param T0           -The time offset from the DetCal file
 */
void SCDCalibratePanels2::saveIsawDetCal(
    const std::string &filename,
    boost::container::flat_set<std::string> &AllBankName,
    std::shared_ptr<Instrument> &instrument, double T0) {
  g_log.notice() << "Saving DetCal file in " << filename << "\n";

  // create a workspace to pass to SaveIsawDetCal
  const size_t number_spectra = instrument->getNumberDetectors();
  Workspace2D_sptr wksp = std::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", number_spectra, 2, 1));
  wksp->setInstrument(instrument);
  wksp->rebuildSpectraMapping(true /* include monitors */);

  // convert the bank names into a vector
  std::vector<std::string> banknames(AllBankName.begin(), AllBankName.end());

  // call SaveIsawDetCal
  API::IAlgorithm_sptr alg = createChildAlgorithm("SaveIsawDetCal");
  alg->setProperty("InputWorkspace", wksp);
  alg->setProperty("Filename", filename);
  alg->setProperty("TimeOffset", T0);
  alg->setProperty("BankNames", banknames);
  alg->executeAsChildAlg();
}

/**
 * @brief Save the CORELLI calibration table into a CSV file
 *
 * @param FileName
 * @param tws
 */
void SCDCalibratePanels2::saveCalibrationTable(
    const std::string &FileName, DataObjects::TableWorkspace_sptr &tws) {
  API::IAlgorithm_sptr alg = createChildAlgorithm("SaveAscii");
  alg->setProperty("InputWorkspace", tws);
  alg->setProperty("Filename", FileName);
  alg->setPropertyValue("CommentIndicator", "#");
  alg->setPropertyValue("Separator", "CSV");
  alg->setProperty("ColumnHeader", true);
  alg->setProperty("AppendToFile", false);
  alg->executeAsChildAlg();
}

} // namespace Crystal
} // namespace Mantid
