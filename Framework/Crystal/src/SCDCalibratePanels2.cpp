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
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Logger.h"
#include <boost/container/flat_set.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <boost/math/special_functions/round.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

namespace Mantid::Crystal {

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
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("PeakWorkspace", "", Kernel::Direction::Input),
                  "Workspace of Indexed Peaks");

  // Lattice constant group
  auto mustBeNonNegative = std::make_shared<BoundedValidator<double>>();
  mustBeNonNegative->setLower(0.0);
  declareProperty("RecalculateUB", true, "Recalculate UB matrix using given lattice constants");
  declareProperty("a", EMPTY_DBL(), mustBeNonNegative,
                  "Lattice Parameter a (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("b", EMPTY_DBL(), mustBeNonNegative,
                  "Lattice Parameter b (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("c", EMPTY_DBL(), mustBeNonNegative,
                  "Lattice Parameter c (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("alpha", EMPTY_DBL(), mustBeNonNegative,
                  "Lattice Parameter alpha in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  declareProperty("beta", EMPTY_DBL(), mustBeNonNegative,
                  "Lattice Parameter beta in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  declareProperty("gamma", EMPTY_DBL(), mustBeNonNegative,
                  "Lattice Parameter gamma in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  const std::string LATTICE("Lattice Constants");
  setPropertyGroup("RecalculateUB", LATTICE);
  setPropertyGroup("a", LATTICE);
  setPropertyGroup("b", LATTICE);
  setPropertyGroup("c", LATTICE);
  setPropertyGroup("alpha", LATTICE);
  setPropertyGroup("beta", LATTICE);
  setPropertyGroup("gamma", LATTICE);
  setPropertySettings("a", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));
  setPropertySettings("b", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));
  setPropertySettings("c", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));
  setPropertySettings("alpha", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));
  setPropertySettings("beta", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));
  setPropertySettings("gamma", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));

  declareProperty("Tolerance", 0.15, mustBeNonNegative, "Peak indexing tolerance");

  // Calibration options group
  // NOTE:
  //  The general workflow of calibration is
  //  - calibrate L1 using all peaks
  //  - calibrate each bank
  //  - calibrate/update L1 again since bank movement will affect L1
  //  - calibrate T0
  //  - calibrate samplePos
  const std::string CALIBRATION("Calibration Options");
  // --------------
  // ----- L1 -----
  // --------------
  declareProperty("CalibrateL1", true, "Change the L1(source to sample) distance");
  declareProperty("SearchRadiusL1", 0.1, mustBeNonNegative,
                  "Search radius of delta L1 in meters, which is used to constrain optimization search space"
                  "when calibrating L1");
  // editability
  setPropertySettings("SearchRadiusL1", std::make_unique<EnabledWhenProperty>("CalibrateL1", IS_EQUAL_TO, "1"));
  // grouping
  setPropertyGroup("CalibrateL1", CALIBRATION);
  setPropertyGroup("SearchRadiusL1", CALIBRATION);
  // ----------------
  // ----- bank -----
  // ----------------
  declareProperty("CalibrateBanks", false, "Calibrate position and orientation of each bank.");
  declareProperty(
      "SearchRadiusTransBank", 5e-2, mustBeNonNegative,
      "This is the search radius (in meter) when calibrating component translations, used to constrain optimization"
      "search space when calibration translation of banks");
  declareProperty("SearchradiusRotXBank", 1.0, mustBeNonNegative,
                  "This is the search radius (in deg) when calibrating component reorientation, used to constrain "
                  "optimization search space");
  declareProperty("SearchradiusRotYBank", 1.0, mustBeNonNegative,
                  "This is the search radius (in deg) when calibrating component reorientation, used to constrain "
                  "optimization search space");
  declareProperty("SearchradiusRotZBank", 1.0, mustBeNonNegative,
                  "This is the search radius (in deg) when calibrating component reorientation, used to constrain "
                  "optimization search space");
  declareProperty("CalibrateSize", false, "Calibrate detector size for each bank.");
  declareProperty("SearchRadiusSize", 0.0, mustBeNonNegative,
                  "This is the search radius (unit less) of scale factor around at value 1.0 "
                  "when calibrating component size if it is a rectangualr detector.");
  declareProperty("FixAspectRatio", true,
                  "If true, the scaling factor for detector along X- and Y-axis "
                  "must be the same.  Otherwise, the 2 scaling factors are free.");
  declareProperty("BankName", "",
                  "If given, only the specified bank/component will be calibrated."
                  "Otherwise, all banks will be calibrated.");

  // editability
  setPropertySettings("BankName", std::make_unique<EnabledWhenProperty>("CalibrateBanks", IS_EQUAL_TO, "1"));
  setPropertySettings("SearchRadiusTransBank",
                      std::make_unique<EnabledWhenProperty>("CalibrateBanks", IS_EQUAL_TO, "1"));
  setPropertySettings("SearchradiusRotXBank",
                      std::make_unique<EnabledWhenProperty>("CalibrateBanks", IS_EQUAL_TO, "1"));
  setPropertySettings("SearchradiusRotYBank",
                      std::make_unique<EnabledWhenProperty>("CalibrateBanks", IS_EQUAL_TO, "1"));
  setPropertySettings("SearchradiusRotZBank",
                      std::make_unique<EnabledWhenProperty>("CalibrateBanks", IS_EQUAL_TO, "1"));
  setPropertySettings("CalibrateSize", std::make_unique<EnabledWhenProperty>("CalibrateBanks", IS_EQUAL_TO, "1"));
  setPropertySettings("SearchRadiusSize", std::make_unique<EnabledWhenProperty>("CalibrateSize", IS_EQUAL_TO, "1"));
  setPropertySettings("FixAspectRatio", std::make_unique<EnabledWhenProperty>("CalibrateSize", IS_EQUAL_TO, "1"));
  // grouping
  setPropertyGroup("CalibrateBanks", CALIBRATION);
  setPropertyGroup("BankName", CALIBRATION);
  setPropertyGroup("SearchRadiusTransBank", CALIBRATION);
  setPropertyGroup("SearchradiusRotXBank", CALIBRATION);
  setPropertyGroup("SearchradiusRotYBank", CALIBRATION);
  setPropertyGroup("SearchradiusRotZBank", CALIBRATION);
  setPropertyGroup("CalibrateSize", CALIBRATION);
  setPropertyGroup("SearchRadiusSize", CALIBRATION);
  setPropertyGroup("FixAspectRatio", CALIBRATION);

  // --------------
  // ----- T0 -----
  // --------------
  declareProperty("CalibrateT0", false, "Calibrate the T0 (initial TOF)");
  declareProperty("SearchRadiusT0", 10.0, mustBeNonNegative,
                  "Search radius of T0 (in ms), used to constrain optimization search space");
  // editability
  setPropertySettings("SearchRadiusT0", std::make_unique<EnabledWhenProperty>("CalibrateT0", IS_EQUAL_TO, "1"));
  // grouping
  setPropertyGroup("CalibrateT0", CALIBRATION);
  setPropertyGroup("SearchRadiusT0", CALIBRATION);
  // ---------------------
  // ----- samplePos -----
  // ---------------------
  declareProperty("TuneSamplePosition", false, "Fine tunning sample position");
  declareProperty("SearchRadiusSamplePos", 0.1, mustBeNonNegative,
                  "Search radius of sample position change (in meters), used to constrain optimization search space");
  // editability
  setPropertySettings("SearchRadiusSamplePos",
                      std::make_unique<EnabledWhenProperty>("TuneSamplePosition", IS_EQUAL_TO, "1"));
  // grouping
  setPropertyGroup("TuneSamplePosition", CALIBRATION);
  setPropertyGroup("SearchRadiusSamplePos", CALIBRATION);

  // Output options group
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The workspace containing the calibration table.");
  declareProperty("T0", 0.0, "Returns the TOF offset from optimization", Kernel::Direction::Output);
  const std::vector<std::string> detcalExts{".DetCal", ".Det_Cal"};
  declareProperty(
      std::make_unique<FileProperty>("DetCalFilename", "SCDCalibrate2.DetCal", FileProperty::OptionalSave, detcalExts),
      "Path to an ISAW-style .detcal file to save.");
  declareProperty(
      std::make_unique<FileProperty>("XmlFilename", "SCDCalibrate2.xml", FileProperty::OptionalSave, ".xml"),
      "Path to an Mantid .xml description(for LoadParameterFile) file to "
      "save.");
  declareProperty(
      std::make_unique<FileProperty>("CSVFilename", "SCDCalibrate2.csv", FileProperty::OptionalSave, ".csv"),
      "Path to an .csv file which contains the Calibration Table");
  // group into Output group
  const std::string OUTPUT("Output");
  setPropertyGroup("OutputWorkspace", OUTPUT);
  setPropertyGroup("DetCalFilename", OUTPUT);
  setPropertyGroup("XmlFilename", OUTPUT);
  setPropertyGroup("CSVFilename", OUTPUT);

  // Add new section for advanced control of the calibration/optimization
  // NOTE: profiling is expensive, think twice before start
  declareProperty("VerboseOutput", false, "Toggle of child algorithm console output.");
  declareProperty("ProfileL1", false, "Perform profiling of objective function with given input for L1");
  declareProperty("ProfileBanks", false, "Perform profiling of objective function with given input for Banks");
  declareProperty("ProfileT0", false, "Perform profiling of objective function with given input for T0");
  declareProperty("ProfileL1T0", false, "Perform profiling of objective function along L1 and T0");
  // grouping into one category
  const std::string ADVCNTRL("Advanced Option");
  setPropertyGroup("VerboseOutput", ADVCNTRL);
  setPropertyGroup("ProfileL1", ADVCNTRL);
  setPropertyGroup("ProfileBanks", ADVCNTRL);
  setPropertyGroup("ProfileT0", ADVCNTRL);
  setPropertyGroup("ProfileL1T0", ADVCNTRL);

  // About fitting
  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("MaxFitIterations", 500, mustBePositive,
                  "Stop after this number of iterations if a good fit is not found");
}

/**
 * @brief validate inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> SCDCalibratePanels2::validateInputs() {
  std::map<std::string, std::string> issues;

  // Lattice constants are required if no UB is attached to the input
  // peak workspace
  IPeaksWorkspace_sptr pws = getProperty("PeakWorkspace");
  double a = getProperty("a");
  double b = getProperty("b");
  double c = getProperty("c");
  double alpha = getProperty("alpha");
  double beta = getProperty("beta");
  double gamma = getProperty("gamma");
  if ((a == EMPTY_DBL() || b == EMPTY_DBL() || c == EMPTY_DBL() || alpha == EMPTY_DBL() || beta == EMPTY_DBL() ||
       gamma == EMPTY_DBL()) &&
      (!pws->sample().hasOrientedLattice())) {
    issues["RecalculateUB"] = "Lattice constants are needed for peak "
                              "workspace without a UB mattrix";
  }

  // sanity check
  if (calibrationTableColumnNames.size() != calibrationTableColumnTypes.size())
    throw std::runtime_error("calibrationTableColumnTypes and calibrationTableColumnTypes have different size.");

  return issues;
}

/**
 * @brief execute calibration
 *
 */
void SCDCalibratePanels2::exec() {
  // parse all inputs
  IPeaksWorkspace_sptr m_pws = getProperty("PeakWorkspace");

  // recalculate UB with given lattice constant
  // if required
  if (getProperty("RecalculateUB")) {
    // parse lattice constants
    parseLatticeConstant(m_pws);

    // recalculate UB and index peaks
    updateUBMatrix(m_pws);
  }

  // remove unindexed peaks
  m_pws = removeUnindexedPeaks(m_pws);

  bool calibrateT0 = getProperty("CalibrateT0");
  bool calibrateL1 = getProperty("CalibrateL1");
  bool calibrateBanks = getProperty("CalibrateBanks");
  bool tuneSamplePos = getProperty("TuneSamplePosition");
  mCalibBankName = getPropertyValue("BankName");
  bool profL1 = getProperty("ProfileL1");
  bool profBanks = getProperty("ProfileBanks");
  bool profT0 = getProperty("ProfileT0");
  bool profL1T0 = getProperty("ProfileL1T0");

  const std::string DetCalFilename = getProperty("DetCalFilename");
  const std::string XmlFilename = getProperty("XmlFilename");
  const std::string CSVFilename = getProperty("CSVFilename");

  // Properties for resizing rectangular detector size
  bool docalibsize = getProperty("CalibrateSize");
  double sizesearchradius = getProperty("SearchRadiusSize");
  bool fixdetxyratio = getProperty("FixAspectRatio");

  maxFitIterations = getProperty("MaxFitIterations");
  LOGCHILDALG = getProperty("VerboseOutput");

  // STEP_0: sort the peaks
  std::vector<std::pair<std::string, bool>> criteria{{"BankName", true}};
  m_pws->sort(criteria);
  // need to keep a copy of the peak workspace at its input state
  IPeaksWorkspace_sptr pws_original = m_pws->clone();

  // STEP_2: preparation
  // get names of banks that can be calibrated
  getBankNames(m_pws);

  // DEV ONLY
  // !!!WARNNING!!!
  //    Profiling a parameter space can be time-consuming and may freeze up your
  //    computing resources for days, therefore please proceed with caution.
  if (profL1) {
    profileL1(m_pws, pws_original);
  }
  if (profBanks) {
    profileBanks(m_pws, pws_original);
  }
  if (profT0) {
    profileT0(m_pws, pws_original);
  }
  if (profL1T0) {
    profileL1T0(m_pws, pws_original);
  }

  // STEP_3: optimize
  //  - L1 (with or without T0 cali attached)
  //  - Banks
  //  - sample position
  if (calibrateL1) {
    // NOTE:
    //    L1 and T0 can be calibrated together to provide stable calibration results.
    g_log.notice() << "** Calibrating L1 (moderator) as requested\n";
    optimizeL1(m_pws, pws_original);
  }

  if (calibrateBanks) {
    g_log.notice() << "** Calibrating L2 and orientation (bank) as requested\n";
    optimizeBanks(m_pws, pws_original, docalibsize, sizesearchradius, fixdetxyratio);
  }

  if (calibrateL1 && calibrateBanks) {
    g_log.notice() << "** Calibrating L1 (moderator) after bank adjusted\n";
    optimizeL1(m_pws, pws_original);
    // NOTE:
    //    Turns out 1 pass is sufficient (tested with the following block)
    //
    // double delta = 1;
    // int cnt = 0;
    // while (delta > 0.01) {
    //   double L1_pre = m_pws->getInstrument()->getSource()->getPos().Z();
    //   optimizeBanks(m_pws, pws_original);
    //   optimizeL1(m_pws, pws_original);
    //   double L1_post = m_pws->getInstrument()->getSource()->getPos().Z();
    //   delta = std::abs((L1_pre - L1_post) / L1_pre);
    //   cnt += 1;
    //   g_log.notice() << "@pass_" << cnt << "\n" << L1_pre << "-->" << L1_post << "\n";
    // }
  }

  if (calibrateT0 && !calibrateL1) {
    // NOTE:
    //    L1 and T0 can be calibrated together to provide a stable results, which is the
    //    recommended way.
    //    However, one can still calibrate T0 only if desired.
    g_log.notice() << "** Calibrating T0 only as requested\n";
    optimizeT0(m_pws, pws_original);
  }

  if (tuneSamplePos && !calibrateL1) {
    g_log.notice() << "** Tunning sample position only as requested\n";
    optimizeSamplePos(m_pws, pws_original);
  }

  if (calibrateT0 && tuneSamplePos && !calibrateL1) {
    g_log.warning() << "** You have chosen to calibrate T0 and sample position while ignoring"
                    << "   L1, which means an iterative search outside this calibration is needed"
                    << "   in order to find the minimum.\n";
  }

  // STEP_4: generate a table workspace to save the calibration results
  g_log.notice() << "-- Generate calibration table\n";
  Instrument_sptr instCalibrated = std::const_pointer_cast<Geometry::Instrument>(m_pws->getInstrument());
  const Geometry::ParameterMap &pmap = m_pws->instrumentParameters();
  ITableWorkspace_sptr tablews = generateCalibrationTable(instCalibrated, pmap);

  // STEP_5: Write to disk if required
  if (!XmlFilename.empty()) {
    saveXmlFile(XmlFilename, m_BankNames, instCalibrated, pmap);
  }

  if (!DetCalFilename.empty()) {
    saveIsawDetCal(DetCalFilename, m_BankNames, instCalibrated, m_T0);
  }

  if (!CSVFilename.empty()) {
    saveCalibrationTable(CSVFilename, tablews);
  }

  // STEP_4: Set the output
  setProperty("T0", m_T0); // output the calibrated T0 as a single value
}

/// ------------------------------------------- ///
/// Core functions for Calibration&Optimizatoin ///
/// ------------------------------------------- ///

/**
 * @brief
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::optimizeL1(IPeaksWorkspace_sptr pws, IPeaksWorkspace_sptr pws_original) {
  // cache starting L1 position
  double original_L1 = std::abs(pws->getInstrument()->getSource()->getPos().Z());
  // T0 can be calibrate along with L1 to provide a more stable results
  bool caliT0 = getProperty("CalibrateT0");
  bool tuneSamplepos = getProperty("TuneSamplePosition");

  MatrixWorkspace_sptr l1ws = getIdealQSampleAsHistogram1D(pws);

  // fit algorithm for the optimization of L1
  auto fitL1_alg = createChildAlgorithm("Fit", -1, -1, false);
  auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
  // NOTE: always use the original pws to get the tofs
  std::vector<double> tofs = captureTOF(pws_original);
  objf->setPeakWorkspace(pws, "moderator", tofs);
  fitL1_alg->setProperty("Function", std::dynamic_pointer_cast<IFunction>(objf));

  //-- bounds&constraints def
  std::ostringstream tie_str;
  tie_str << "DeltaX=0.0,DeltaY=0.0,"
          << "RotX=0.0,RotY=0.0,RotZ=0.0";
  if (!tuneSamplepos) {
    tie_str << ",DeltaSampleX=0.0,DeltaSampleY=0.0,DeltaSampleZ=0.0";
  }
  if (!caliT0) {
    tie_str << ",DeltaT0=" << m_T0;
  }
  std::ostringstream constraint_str;
  double r_L1 = getProperty("SearchRadiusL1"); // get search radius
  r_L1 = std::abs(r_L1);
  constraint_str << -r_L1 << "<DeltaZ<" << r_L1;
  // throw in the constrain for T0 cali if needed
  if (caliT0) {
    double r_dT0 = getProperty("SearchRadiusT0");
    r_dT0 = std::abs(r_dT0);
    constraint_str << "," << -r_dT0 << "<DeltaT0<" << r_dT0;
  }
  if (tuneSamplepos) {
    double r_dsp = getProperty("SearchRadiusSamplePos");
    r_dsp = std::abs(r_dsp);
    constraint_str << "," << -r_dsp << "<DeltaSampleX<" << r_dsp  // dsx
                   << "," << -r_dsp << "<DeltaSampleY<" << r_dsp  // dsy
                   << "," << -r_dsp << "<DeltaSampleZ<" << r_dsp; // dsz
  }
  //-- set and go
  fitL1_alg->setProperty("Ties", tie_str.str());
  fitL1_alg->setProperty("Constraints", constraint_str.str());
  fitL1_alg->setProperty("InputWorkspace", l1ws);
  fitL1_alg->setProperty("CreateOutput", true);
  fitL1_alg->setProperty("Output", "fit");
  fitL1_alg->executeAsChildAlg();

  //-- parse output
  std::ostringstream calilog;
  double chi2OverDOF = fitL1_alg->getProperty("OutputChi2overDoF");
  ITableWorkspace_sptr rst = fitL1_alg->getProperty("OutputParameters");
  // get results for L1
  double dL1_optimized = rst->getRef<double>("Value", 2);

  // get results for T0 (optional)
  double dT0_optimized = rst->getRef<double>("Value", 6);

  // get results for sample pos
  // NOTE:
  //    if samplePos is not part of calibration, we will get zeros here, which means zero
  //    negative impact on the whole pws
  double dsx_optimized = rst->getRef<double>("Value", 7);
  double dsy_optimized = rst->getRef<double>("Value", 8);
  double dsz_optimized = rst->getRef<double>("Value", 9);

  // apply the cali results (for output cali table and file)
  adjustComponent(0.0, 0.0, dL1_optimized, 0.0, 0.0, 0.0, EMPTY_DBL(), EMPTY_DBL(),
                  pws->getInstrument()->getSource()->getName(), pws);
  m_T0 = dT0_optimized;
  adjustComponent(dsx_optimized, dsy_optimized, dsz_optimized, 0.0, 0.0, 0.0, EMPTY_DBL(), EMPTY_DBL(),
                  "sample-position", pws);
  // logging
  int npks = pws->getNumberPeaks();
  calilog << "-- Fit L1 results using " << npks << " peaks:\n"
          << "    dL1: " << dL1_optimized << " \n"
          << "    L1 " << original_L1 << " -> " << -pws->getInstrument()->getSource()->getPos().Z() << " \n"
          << "    dT0 = " << m_T0 << " (ms)\n"
          << "    dSamplePos = (" << dsx_optimized << "," << dsy_optimized << "," << dsz_optimized << ")\n"
          << "    chi2/DOF = " << chi2OverDOF << "\n";
  g_log.notice() << calilog.str();
}

/**
 * @brief Calibrate the position and rotation of each Bank, one at a time
 *
 * @param pws
 * @param pws_original
 * @param docalibsize :: flag to calibrate rectangular detector size
 * @param sizesearchradius  :: searching radius for detector size calibration
 * @param fixdetxyratio:: flag to tie the rectangular detector
 */
void SCDCalibratePanels2::optimizeBanks(IPeaksWorkspace_sptr pws, const IPeaksWorkspace_sptr &pws_original,
                                        const bool &docalibsize, const double &sizesearchradius,
                                        const bool &fixdetxyratio) {

  PARALLEL_FOR_IF(Kernel::threadSafe(*pws))
  for (int i = 0; i < static_cast<int>(m_BankNames.size()); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    // prepare local copies to work with
    const std::string bankname = *std::next(m_BankNames.begin(), i);
    const std::string pwsBankiName = "_pws_" + bankname;

    // Find out whether to skip the calibration by user's specification
    // This check is only requied when mCalibBankName is not empty string
    if (mCalibBankName != "") {
      bool isbank = (bankname == mCalibBankName);
      std::stringstream ss;
      ss << "i = " << i << " m bank name = " << bankname;
      if (isbank)
        ss << " ... True ...";
      else
        ss << " ... Stop ...";
      g_log.notice(ss.str());
      // continue/skip if bank name is not what is specified
      if (!isbank)
        continue;
    }

    //-- step 0: extract peaks that lies on the current bank
    IPeaksWorkspace_sptr pwsBanki = selectPeaksByBankName(pws, bankname, pwsBankiName);
    //   get tofs from the original subset of pws
    IPeaksWorkspace_sptr pwsBanki_original = selectPeaksByBankName(pws_original, bankname, pwsBankiName);
    std::vector<double> tofs = captureTOF(pwsBanki_original);

    // Do not attempt correct panels with less than 6 peaks as the system will
    // be under-determined
    int nBankPeaks = pwsBanki->getNumberPeaks();
    if (nBankPeaks < MINIMUM_PEAKS_PER_BANK) {
      // use ostringstream to prevent OPENMP breaks log info
      std::ostringstream msg_npeakCheckFail;
      msg_npeakCheckFail << "-- Bank " << bankname << " have only " << nBankPeaks << " (<" << MINIMUM_PEAKS_PER_BANK
                         << ") Peaks, skipping\n";
      g_log.notice() << msg_npeakCheckFail.str();
      continue;
    }

    //-- step 1: prepare a mocked workspace with QSample as its yValues
    MatrixWorkspace_sptr wsBankCali = getIdealQSampleAsHistogram1D(pwsBanki);

    //-- step 2&3: invoke fit to find both traslation and rotation
    auto fitBank_alg = createChildAlgorithm("Fit", -1, -1, false);
    //---- setup obj fun def
    auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
    objf->setPeakWorkspace(pwsBanki, bankname, tofs);
    fitBank_alg->setProperty("Function", std::dynamic_pointer_cast<IFunction>(objf));

    //---- bounds&constraints def
    //
    double searchRadiusRotX = getProperty("SearchradiusRotXBank");
    searchRadiusRotX = std::abs(searchRadiusRotX);
    double searchRadiusRotY = getProperty("SearchradiusRotYBank");
    searchRadiusRotY = std::abs(searchRadiusRotY);
    double searchRadiusRotZ = getProperty("SearchradiusRotZBank");
    searchRadiusRotZ = std::abs(searchRadiusRotZ);
    //
    double searchRadiusTran = getProperty("SearchRadiusTransBank");
    searchRadiusTran = std::abs(searchRadiusTran);
    //
    std::ostringstream tie_str;
    tie_str << "DeltaSampleX=0.0,DeltaSampleY=0.0,DeltaSampleZ=0.0,"
            << "DeltaT0=" << m_T0;
    std::ostringstream constraint_str;
    // rot x
    if (searchRadiusRotX < Tolerance) {
      tie_str << ",RotX=0.0";
    } else {
      constraint_str << -searchRadiusRotX << "<RotX<" << searchRadiusRotX << ",";
    }
    // rot y
    if (searchRadiusRotY < Tolerance) {
      tie_str << ",RotY=0.0";
    } else {
      constraint_str << -searchRadiusRotY << "<RotY<" << searchRadiusRotY << ",";
    }
    // rot z
    if (searchRadiusRotZ < Tolerance) {
      tie_str << ",RotZ=0.0";
    } else {
      constraint_str << -searchRadiusRotZ << "<RotZ<" << searchRadiusRotZ << ","; // constrain rotation around Z-axis
    }
    // translation
    if (searchRadiusTran < Tolerance) {
      tie_str << ",DeltaX=0.0,DeltaY=0.0,DeltaZ=0.0";
    } else {
      constraint_str << -searchRadiusTran << "<DeltaX<" << searchRadiusTran << "," // restrict tranlastion along X
                     << -searchRadiusTran << "<DeltaY<" << searchRadiusTran << "," // restrict tranlastion along Y
                     << -searchRadiusTran << "<DeltaZ<" << searchRadiusTran;       // restrict tranlastion along Z
    }
    // calibration of detector size
    // docalibsize, sizesearchradius, fixdetxyratio
    Geometry::Instrument_sptr inst = std::const_pointer_cast<Geometry::Instrument>(pws->getInstrument());
    Geometry::IComponent_const_sptr comp = inst->getComponentByName(bankname);
    std::shared_ptr<const Geometry::RectangularDetector> rectDet =
        std::dynamic_pointer_cast<const Geometry::RectangularDetector>(comp);

    std::pair<double, double> scales = getRectangularDetectorScaleFactors(inst, bankname, pws->instrumentParameters());

    std::ostringstream scaleconstraints;
    std::ostringstream scaleties;
    if (rectDet && docalibsize) {
      // set up constraints
      scaleconstraints << scales.first - sizesearchradius << " <=ScaleX<" << scales.first + sizesearchradius;
      if (fixdetxyratio) {
        scaleties << "ScaleX=ScaleY";
      } else {
        scaleconstraints << "," << scales.second - sizesearchradius << " <=ScaleY<" << scales.second + sizesearchradius;
      }
    } else {
      // fix the scalex and scaley to its
      scaleties << "ScaleX=" << scales.first << ", ScaleY=" << scales.second;
    }

    // construct the final constraint and tie
    std::string fitconstraint{constraint_str.str()};
    if (scaleconstraints.str() != "") {
      if (fitconstraint == "")
        fitconstraint += scaleconstraints.str();
      else
        fitconstraint += "," + scaleconstraints.str();
    }
    std::string fittie{tie_str.str()};
    if (scaleties.str() != "") {
      if (fittie == "")
        fittie += scaleties.str();
      else
        fittie += "," + scaleties.str();
    }

    g_log.information("Fitting " + bankname + ": constraint = " + fitconstraint + "\n\t tie = " + fittie);

    //---- set&go
    if (fittie != "")
      fitBank_alg->setProperty("Ties", fittie);
    if (fitconstraint != "")
      fitBank_alg->setProperty("Constraints", fitconstraint);
    fitBank_alg->setProperty("InputWorkspace", wsBankCali);
    fitBank_alg->setProperty("CreateOutput", true);
    fitBank_alg->setProperty("Output", "fit");
    fitBank_alg->setProperty("MaxIterations", maxFitIterations);

    fitBank_alg->executeAsChildAlg();

    //---- cache results
    double chi2OverDOF = fitBank_alg->getProperty("OutputChi2overDoF");
    ITableWorkspace_sptr rstFitBank = fitBank_alg->getProperty("OutputParameters");
    double dx = rstFitBank->getRef<double>("Value", 0);
    double dy = rstFitBank->getRef<double>("Value", 1);
    double dz = rstFitBank->getRef<double>("Value", 2);
    double drx = rstFitBank->getRef<double>("Value", 3);
    double dry = rstFitBank->getRef<double>("Value", 4);
    double drz = rstFitBank->getRef<double>("Value", 5);
    double scalex = rstFitBank->getRef<double>("Value", 10);
    double scaley = rstFitBank->getRef<double>("Value", 11);

    //-- step 4: update the instrument with optimization results
    std::string bn = bankname;
    std::ostringstream calilog;
    if (pws->getInstrument()->getName().compare("CORELLI") == 0) {
      bn.append("/sixteenpack");
    }
    // update instrument for output
    if (rectDet && docalibsize) {
      // adjust detector size only if it is to be set to refine
      adjustComponent(dx, dy, dz, drx, dry, drz, scalex, scaley, bn, pws);
    } else {
      // (1) no rectangular det or (2) not to refine detector size:
      // do not set any physically possible scalex or scaley
      adjustComponent(dx, dy, dz, drx, dry, drz, EMPTY_DBL(), EMPTY_DBL(), bn, pws);
    }
    // logging
    V3D dtrans(dx, dy, dz);
    V3D drots(drx, dry, drz);
    calilog << "-- Fit " << bn << " results using " << nBankPeaks << " peaks:\n"
            << "    d(x,y,z) = " << dtrans << "\n"
            << "    r(x,y,z) = " << drots << "\n"
            << "    scale(x, y) = " << scalex << ", " << scaley << "    chi2/DOF = " << chi2OverDOF << "\n";
    g_log.notice() << calilog.str();

    // -- cleanup
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

/**
 * @brief adjusting the deltaT0 to match the qSample_calculated and
 *        qSameple_measured
 *
 * @note this function currently only returns dT0=0, and the reason
 *       is still unkown.
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::optimizeT0(IPeaksWorkspace_sptr pws, IPeaksWorkspace_sptr pws_original) {
  // create child Fit alg to optimize T0
  auto fitT0_alg = createChildAlgorithm("Fit", -1, -1, false);
  //-- obj func def
  //  dl;dr;
  //    Fit algorithm requires a IFunction1D to fit
  //  details
  //    Fit algorithm requires a class derived from IFunction1D as its
  //    input, so we have to implement the objective function as a separate
  //    class just to get Fit serving as an optimizer.
  //    For this particular case, we are constructing an objective function
  //    based on IFunction1D that outputs a fake histogram consist of
  //    qSample calculated based on perturbed instrument positions and
  //    orientations.
  MatrixWorkspace_sptr t0ws = getIdealQSampleAsHistogram1D(pws);

  auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
  // NOTE: always use the original pws to get the tofs
  std::vector<double> tofs = captureTOF(pws_original);
  objf->setPeakWorkspace(pws, "none", tofs);
  fitT0_alg->setProperty("Function", std::dynamic_pointer_cast<IFunction>(objf));

  //-- bounds&constraints def
  std::ostringstream tie_str;
  tie_str << "DeltaX=0.0,DeltaY=0.0,DeltaZ=0.0,"
          << "RotX=0.0,RotY=0.0,RotZ=0.0,"
          << "DeltaSampleX=0.0,DeltaSampleY=0.0,DeltaSampleZ=0.0";
  std::ostringstream constraint_str;
  double r_dT0 = getProperty("SearchRadiusT0");
  r_dT0 = std::abs(r_dT0);
  constraint_str << -r_dT0 << "<DeltaT0<" << r_dT0;

  //-- set&go
  fitT0_alg->setProperty("Ties", tie_str.str());
  fitT0_alg->setProperty("Constraints", constraint_str.str());
  fitT0_alg->setProperty("InputWorkspace", t0ws);
  fitT0_alg->setProperty("CreateOutput", true);
  fitT0_alg->setProperty("Output", "fit");
  fitT0_alg->executeAsChildAlg();

  //-- parse output
  std::ostringstream calilog;
  double chi2OverDOF = fitT0_alg->getProperty("OutputChi2overDoF");
  ITableWorkspace_sptr rst = fitT0_alg->getProperty("OutputParameters");
  double dT0_optimized = rst->getRef<double>("Value", 6);

  // apply calibration results (for output file and caliTable)
  m_T0 = dT0_optimized;
  int npks = pws->getNumberPeaks();
  // logging
  calilog << "-- Fit T0 results using " << npks << " peaks:\n"
          << "    dT0 = " << m_T0 << " (ms)\n"
          << "    chi2/DOF = " << chi2OverDOF << "\n";
  g_log.notice() << calilog.str();
}

/**
 * @brief fine tuning sample position to better match QSample
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::optimizeSamplePos(IPeaksWorkspace_sptr pws, IPeaksWorkspace_sptr pws_original) {
  // create child Fit alg to optimize T0
  auto fitSamplePos_alg = createChildAlgorithm("Fit", -1, -1, false);

  // creat input 1DHist from qSample
  MatrixWorkspace_sptr samplePosws = getIdealQSampleAsHistogram1D(pws);

  auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
  // NOTE: always use the original pws to get the tofs
  std::vector<double> tofs = captureTOF(pws_original);
  objf->setPeakWorkspace(pws, "none", tofs);
  fitSamplePos_alg->setProperty("Function", std::dynamic_pointer_cast<IFunction>(objf));

  //-- bounds&constraints def
  std::ostringstream tie_str;
  tie_str << "DeltaX=0.0,DeltaY=0.0,DeltaZ=0.0,"
          << "RotX=0.0,RotY=0.0,RotZ=0.0,"
          << "DeltaT0=" << m_T0;
  std::ostringstream constraint_str;
  double r_dsp = getProperty("SearchRadiusSamplePos");
  r_dsp = std::abs(r_dsp);
  constraint_str << -r_dsp << "<DeltaSampleX<" << r_dsp << "," << -r_dsp << "<DeltaSampleY<" << r_dsp << "," << -r_dsp
                 << "<DeltaSampleZ<" << r_dsp;

  //-- set&go
  fitSamplePos_alg->setProperty("Ties", tie_str.str());
  fitSamplePos_alg->setProperty("Constraints", constraint_str.str());
  fitSamplePos_alg->setProperty("InputWorkspace", samplePosws);
  fitSamplePos_alg->setProperty("CreateOutput", true);
  fitSamplePos_alg->setProperty("Output", "fit");
  fitSamplePos_alg->executeAsChildAlg();

  //-- parse output
  std::ostringstream calilog;
  double chi2OverDOF = fitSamplePos_alg->getProperty("OutputChi2overDoF");
  ITableWorkspace_sptr rst = fitSamplePos_alg->getProperty("OutputParameters");
  double dsx_optimized = rst->getRef<double>("Value", 7);
  double dsy_optimized = rst->getRef<double>("Value", 8);
  double dsz_optimized = rst->getRef<double>("Value", 9);

  // apply the calibration results to pws for ouptut file
  adjustComponent(dsx_optimized, dsy_optimized, dsz_optimized, 0.0, 0.0, 0.0, EMPTY_DBL(), EMPTY_DBL(),
                  "sample-position", pws);
  int npks = pws->getNumberPeaks();
  // logging
  calilog << "-- Tune SamplePos results using " << npks << " peaks:\n"
          << "  deltaSamplePos = (" << dsx_optimized << "," << dsy_optimized << "," << dsz_optimized << ")\n"
          << "  chi2/DOF = " << chi2OverDOF << "\n";
  g_log.notice() << calilog.str();
}

/// ---------------- ///
/// helper functions ///
/// ---------------- ///

/**
 * @brief get lattice constants from either inputs or the
 *        input peak workspace
 *
 */
void SCDCalibratePanels2::parseLatticeConstant(const IPeaksWorkspace_sptr &pws) {
  m_a = getProperty("a");
  m_b = getProperty("b");
  m_c = getProperty("c");
  m_alpha = getProperty("alpha");
  m_beta = getProperty("beta");
  m_gamma = getProperty("gamma");
  // if any one of the six lattice constants is missing, try to get
  // one from the workspace
  if ((m_a == EMPTY_DBL() || m_b == EMPTY_DBL() || m_c == EMPTY_DBL() || m_alpha == EMPTY_DBL() ||
       m_beta == EMPTY_DBL() || m_gamma == EMPTY_DBL()) &&
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
 * @brief update UB matrix embeded in the peakworkspace using lattice constants
 *        and redo the peak indexation afterwards
 *
 * @param pws
 */
void SCDCalibratePanels2::updateUBMatrix(const IPeaksWorkspace_sptr &pws) {
  auto calcUB_alg = createChildAlgorithm("CalculateUMatrix", -1, -1, false);
  calcUB_alg->setLogging(LOGCHILDALG);
  calcUB_alg->setProperty("PeaksWorkspace", pws);
  calcUB_alg->setProperty("a", m_a);
  calcUB_alg->setProperty("b", m_b);
  calcUB_alg->setProperty("c", m_c);
  calcUB_alg->setProperty("alpha", m_alpha);
  calcUB_alg->setProperty("beta", m_beta);
  calcUB_alg->setProperty("gamma", m_gamma);
  calcUB_alg->executeAsChildAlg();

  double tol = getProperty("Tolerance");

  // Since UB is updated, we need to redo the indexation
  auto idxpks_alg = createChildAlgorithm("IndexPeaks", -1, -1, false);
  idxpks_alg->setLogging(LOGCHILDALG);
  idxpks_alg->setProperty("PeaksWorkspace", pws);
  idxpks_alg->setProperty("RoundHKLs", true); // using default
  idxpks_alg->setProperty("Tolerance", tol);  // values
  idxpks_alg->executeAsChildAlg();
}

/**
 * @brief
 *
 * @param pws
 * @return IPeaksWorkspace_sptr
 */
IPeaksWorkspace_sptr SCDCalibratePanels2::removeUnindexedPeaks(const Mantid::API::IPeaksWorkspace_sptr &pws) {
  auto fltpk_alg = createChildAlgorithm("FilterPeaks");
  fltpk_alg->setLogging(LOGCHILDALG);
  fltpk_alg->setProperty("InputWorkspace", pws);
  fltpk_alg->setProperty("FilterVariable", "h^2+k^2+l^2");
  fltpk_alg->setProperty("Operator", ">");
  fltpk_alg->setProperty("FilterValue", 0.0);
  fltpk_alg->setProperty("OutputWorkspace", "pws_filtered");
  fltpk_alg->executeAsChildAlg();

  IPeaksWorkspace_sptr outWS = fltpk_alg->getProperty("OutputWorkspace");
  return outWS;
}

/**
 * @brief Capture TOFs that are equivalent to thoes measured from experiment.
 *        This step should be carried out after the indexation (if required)
 *
 * @param pws
 */
std::vector<double> SCDCalibratePanels2::captureTOF(const Mantid::API::IPeaksWorkspace_sptr &pws) {
  std::vector<double> tofs;

  for (int i = 0; i < pws->getNumberPeaks(); ++i) {
    tofs.emplace_back(pws->getPeak(i).getTOF());
  }

  return tofs;
}

/**
 * @brief Gather names for bank for calibration
 *
 * @param pws
 */
void SCDCalibratePanels2::getBankNames(const IPeaksWorkspace_sptr &pws) {
  auto peaksWorkspace = std::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(pws);
  if (!peaksWorkspace)
    throw std::invalid_argument("a PeaksWorkspace is required to retrieve bank names");
  int npeaks = static_cast<int>(pws->getNumberPeaks());
  for (int i = 0; i < npeaks; ++i) {
    std::string bname = peaksWorkspace->getPeak(i).getBankName();
    if (bname != "None")
      m_BankNames.insert(bname);
  }
}

/**
 * @brief Select peaks with give bankname
 *
 * @param pws
 * @param bankname
 * @param outputwsn
 * @return DataObjects::PeaksWorkspace_sptr
 */
IPeaksWorkspace_sptr SCDCalibratePanels2::selectPeaksByBankName(const IPeaksWorkspace_sptr &pws,
                                                                const std::string &bankname,
                                                                const std::string &outputwsn) {
  auto fltpk_alg = createChildAlgorithm("FilterPeaks");
  fltpk_alg->setLogging(LOGCHILDALG);
  fltpk_alg->setProperty("InputWorkspace", pws);
  fltpk_alg->setProperty("BankName", bankname);
  fltpk_alg->setProperty("Criterion", "=");
  fltpk_alg->setProperty("OutputWorkspace", outputwsn);
  fltpk_alg->executeAsChildAlg();

  IPeaksWorkspace_sptr outWS = fltpk_alg->getProperty("OutputWorkspace");
  return outWS;
}

/**
 * @brief Return a 1D histogram consists of ideal qSample calculated from
 *        integer HKL directly
 *
 * @param pws
 * @return MatrixWorkspace_sptr
 */
MatrixWorkspace_sptr SCDCalibratePanels2::getIdealQSampleAsHistogram1D(const IPeaksWorkspace_sptr &pws) {
  int npeaks = pws->getNumberPeaks();

  // prepare workspace to store qSample as Histogram1D
  MatrixWorkspace_sptr mws = std::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", // use workspace 2D to mock a histogram
                                          1,             // one vector
                                          3 * npeaks,    // X :: anything is fine
                                          3 * npeaks));  // Y :: flattened Q vector
  auto &spectrum = mws->getSpectrum(0);
  auto &xvector = spectrum.mutableX();
  auto &yvector = spectrum.mutableY();
  auto &evector = spectrum.mutableE();

  // quick check to see what kind of weighting we can use
  double totalSigmaInt = 0.0;
  for (int i = 0; i < npeaks; ++i) {
    totalSigmaInt += pws->getPeak(i).getSigmaIntensity();
  }
  double totalInt = 0.0;
  for (int i = 0; i < npeaks; ++i) {
    totalInt += pws->getPeak(i).getIntensity();
  }
  double totalCnt = 0.0;
  for (int i = 0; i < npeaks; ++i) {
    totalCnt += pws->getPeak(i).getBinCount();
  }

  // directly compute qsample from UBmatrix and HKL
  auto ubmatrix = pws->sample().getOrientedLattice().getUB();
  for (int i = 0; i < npeaks; ++i) {

    V3D qv = ubmatrix * pws->getPeak(i).getIntHKL();
    qv *= 2 * PI;
    // qv = qv / qv.norm();
    double wgt = 1.0;
    if (totalSigmaInt > 0.0) {
      wgt = 1.0 / pws->getPeak(i).getSigmaIntensity();
    } else if (totalInt > 0.0) {
      wgt = 1.0 / pws->getPeak(i).getIntensity();
    } else if (totalCnt > 0.0) {
      wgt = 1.0 / pws->getPeak(i).getBinCount();
    }
    // make 1dhist
    for (int j = 0; j < 3; ++j) {
      xvector[i * 3 + j] = i * 3 + j;
      yvector[i * 3 + j] = qv[j];
      evector[i * 3 + j] = wgt;
    }
  }

  return mws;
}

/**
 * @brief adjust instrument component position and orientation
 *
 * @param dx
 * @param dy
 * @param dz
 * @param drx
 * @param dry
 * @param drz
 * @package scalex: detector size scale at x-direction
 * @package scaley: detector size scale at y-direction
 * @param cmptName
 * @param pws
 */
void SCDCalibratePanels2::adjustComponent(double dx, double dy, double dz, double drx, double dry, double drz,
                                          double scalex, double scaley, const std::string &cmptName,
                                          IPeaksWorkspace_sptr &pws) {
  // translation
  auto mv_alg = createChildAlgorithm("MoveInstrumentComponent", -1, -1, false);
  mv_alg->setLogging(LOGCHILDALG);
  mv_alg->setProperty<Workspace_sptr>("Workspace", pws);
  mv_alg->setProperty("ComponentName", cmptName);
  mv_alg->setProperty("X", dx);
  mv_alg->setProperty("Y", dy);
  mv_alg->setProperty("Z", dz);
  mv_alg->setProperty("RelativePosition", true);
  mv_alg->executeAsChildAlg();

  // rotation
  auto rot_alg = createChildAlgorithm("RotateInstrumentComponent", -1, -1, false);
  rot_alg->setLogging(LOGCHILDALG);
  // - x-axis
  rot_alg->setProperty<Workspace_sptr>("Workspace", pws);
  rot_alg->setProperty("ComponentName", cmptName);
  rot_alg->setProperty("X", 1.0);
  rot_alg->setProperty("Y", 0.0);
  rot_alg->setProperty("Z", 0.0);
  rot_alg->setProperty("Angle", drx);
  rot_alg->setProperty("RelativeRotation", true);
  rot_alg->executeAsChildAlg();
  // - y-axis
  rot_alg->setProperty<Workspace_sptr>("Workspace", pws);
  rot_alg->setProperty("ComponentName", cmptName);
  rot_alg->setProperty("X", 0.0);
  rot_alg->setProperty("Y", 1.0);
  rot_alg->setProperty("Z", 0.0);
  rot_alg->setProperty("Angle", dry);
  rot_alg->setProperty("RelativeRotation", true);
  rot_alg->executeAsChildAlg();
  // - z-axis
  rot_alg->setProperty<Workspace_sptr>("Workspace", pws);
  rot_alg->setProperty("ComponentName", cmptName);
  rot_alg->setProperty("X", 0.0);
  rot_alg->setProperty("Y", 0.0);
  rot_alg->setProperty("Z", 1.0);
  rot_alg->setProperty("Angle", drz);
  rot_alg->setProperty("RelativeRotation", true);
  rot_alg->executeAsChildAlg();

  // scale detector size
  if (scalex != Mantid::EMPTY_DBL() && scaley != Mantid::EMPTY_DBL()) {
    auto resizeAlg = createChildAlgorithm("ResizeRectangularDetector", -1, -1, false);
    resizeAlg->initialize();
    resizeAlg->setProperty("Workspace", pws);
    resizeAlg->setProperty("ComponentName", cmptName);
    resizeAlg->setProperty("ScaleX", scalex);
    resizeAlg->setProperty("ScaleY", scaley);
    resizeAlg->execute();

    g_log.notice() << "Resize " << cmptName << " by (absolute) " << scalex << ", " << scaley << "\n";
  }
}

/**
 * @brief Generate a tableworkspace to store the calibration results
 *
 * @param instrument  :: calibrated instrument
 * @package pmap :: parameter map from workspace
 * @return DataObjects::TableWorkspace_sptr
 */
ITableWorkspace_sptr SCDCalibratePanels2::generateCalibrationTable(std::shared_ptr<Geometry::Instrument> &instrument,
                                                                   const Geometry::ParameterMap &pmap) {
  g_log.notice() << "Generate a TableWorkspace to store calibration results.\n";

  // Create table workspace
  ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();
  for (size_t i = 0; i < calibrationTableColumnNames.size(); ++i)
    itablews->addColumn(calibrationTableColumnTypes[i], calibrationTableColumnNames[i]);

  // The first row is always the source
  IComponent_const_sptr source = instrument->getSource();
  V3D sourceRelPos = source->getRelativePos();
  Mantid::API::TableRow sourceRow = itablews->appendRow();
  // NOTE: source should not have any rotation, so we pass a zero
  //       rotation with a fixed axis
  sourceRow << instrument->getSource()->getName() << sourceRelPos.X() << sourceRelPos.Y() << sourceRelPos.Z() << 1.0
            << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;

  // Loop through banks and set row values
  for (auto bankName : m_BankNames) {
    // CORELLLI instrument has one extra layer that pack tubes into
    // banks, which is what we need here
    if (instrument->getName().compare("CORELLI") == 0)
      bankName.append("/sixteenpack");

    std::shared_ptr<const IComponent> bank = instrument->getComponentByName(bankName);

    Quat relRot = bank->getRelativeRot();
    V3D pos1 = bank->getRelativePos();

    // Calculate cosines using relRot
    double deg, xAxis, yAxis, zAxis;
    relRot.getAngleAxis(deg, xAxis, yAxis, zAxis);

    // Detector scaling
    std::pair<double, double> scales = getRectangularDetectorScaleFactors(instrument, bankName, pmap);

    // Append a new row
    Mantid::API::TableRow bankRow = itablews->appendRow();
    // Row and positions
    bankRow << bankName << pos1.X() << pos1.Y() << pos1.Z() << xAxis << yAxis << zAxis << deg << scales.first
            << scales.second;
  }

  g_log.notice() << "finished generating tables\n";
  setProperty("OutputWorkspace", itablews);

  return itablews;
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
 * @param pmap :: parameter map from workspace
 *
 */
void SCDCalibratePanels2::saveXmlFile(const std::string &FileName,
                                      const boost::container::flat_set<std::string> &AllBankNames,
                                      std::shared_ptr<Instrument> &instrument, const Geometry::ParameterMap &pmap) {
  g_log.notice() << "Generating xml tree \n";

  using boost::property_tree::ptree;
  ptree root;
  ptree parafile;

  // configure root node
  parafile.put("<xmlattr>.instrument", instrument->getName());
  parafile.put("<xmlattr>.valid-from", instrument->getValidFromDate().toISO8601String());

  // get L1 info for source
  ptree src;
  ptree src_dx, src_dy, src_dz;
  ptree src_dx_val, src_dy_val, src_dz_val;
  // -- get positional data from source
  IComponent_const_sptr source = instrument->getSource();
  V3D sourceRelPos = source->getRelativePos();
  // -- add data to node
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

  // add node for T0
  // -- property_root is a dumping group for property type values that are not tied to particular
  //    component (i.e. virtual properties)
  ptree property_root;
  property_root.put("<xmlattr>.name", instrument->getName());
  ptree tof0;
  ptree tof0_val;
  tof0.put("<xmlattr>.name", "T0");
  tof0_val.put("<xmlattr>.val", m_T0);
  tof0.add_child("value", tof0_val);
  property_root.add_child("parameter", tof0);
  parafile.add_child("component-link", property_root);

  // save sample position as a standalone component-link
  ptree samplePos;
  ptree samplePos_dx, samplePos_dy, samplePos_dz;
  ptree samplePos_dx_val, samplePos_dy_val, samplePos_dz_val;
  // -- get positional data from sample
  std::shared_ptr<const IComponent> sp = instrument->getComponentByName("sample-position");
  V3D sppos = sp->getRelativePos();
  samplePos_dx_val.put("<xmlattr>.val", sppos.X());
  samplePos_dy_val.put("<xmlattr>.val", sppos.Y());
  samplePos_dz_val.put("<xmlattr>.val", sppos.Z());
  samplePos_dx.put("<xmlattr>.name", "x");
  samplePos_dy.put("<xmlattr>.name", "y");
  samplePos_dz.put("<xmlattr>.name", "z");
  samplePos.put("<xmlattr>.name", "sample-position");

  samplePos_dx.add_child("value", samplePos_dx_val);
  samplePos_dy.add_child("value", samplePos_dy_val);
  samplePos_dz.add_child("value", samplePos_dz_val);
  samplePos.add_child("parameter", samplePos_dx);
  samplePos.add_child("parameter", samplePos_dy);
  samplePos.add_child("parameter", samplePos_dz);

  parafile.add_child("component-link", samplePos);

  // configure and add each bank
  for (auto bankName : AllBankNames) {
    // Prepare data for node
    if (instrument->getName().compare("CORELLI") == 0)
      bankName.append("/sixteenpack");

    std::shared_ptr<const IComponent> bank = instrument->getComponentByName(bankName);

    Quat relRot = bank->getRelativeRot();
    std::vector<double> relRotAngles = relRot.getEulerAngles("XYZ");
    V3D pos1 = bank->getRelativePos();
    std::pair<double, double> scales = getRectangularDetectorScaleFactors(instrument, bankName, pmap);

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

    bank_sx_val.put("<xmlattr>.val", scales.first);
    bank_sy_val.put("<xmlattr>.val", scales.second);
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

  // give everything to root
  root.add_child("parameter-file", parafile);
  // write the xml tree to disk
  g_log.notice() << "\tSaving parameter file as " << FileName << "\n";
  boost::property_tree::write_xml(FileName, root, std::locale(),
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
void SCDCalibratePanels2::saveIsawDetCal(const std::string &filename,
                                         boost::container::flat_set<std::string> &AllBankName,
                                         std::shared_ptr<Instrument> &instrument, double T0) {
  g_log.notice() << "Saving DetCal file in " << filename << "\n";

  bool tuneSamplePos = getProperty("TuneSamplePosition");
  if (tuneSamplePos) {
    g_log.warning() << "!!!WARNING!!!\n"
                    << "DetCal format cannot retain sample position info, therefore the calibrated "
                    << "sample position will be lost if DetCal format is the only output!\n";
  }

  // create a workspace to pass to SaveIsawDetCal
  const size_t number_spectra = instrument->getNumberDetectors();
  Workspace2D_sptr wksp =
      std::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", number_spectra, 2, 1));
  wksp->setInstrument(instrument);
  wksp->rebuildSpectraMapping(true /* include monitors */);

  // convert the bank names into a vector
  std::vector<std::string> banknames(AllBankName.begin(), AllBankName.end());

  // call SaveIsawDetCal
  auto alg = createChildAlgorithm("SaveIsawDetCal");
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
void SCDCalibratePanels2::saveCalibrationTable(const std::string &FileName, Mantid::API::ITableWorkspace_sptr &tws) {
  auto alg = createChildAlgorithm("SaveAscii");
  alg->setProperty("InputWorkspace", tws);
  alg->setProperty("Filename", FileName);
  alg->setPropertyValue("CommentIndicator", "#");
  alg->setPropertyValue("Separator", "CSV");
  alg->setProperty("ColumnHeader", true);
  alg->setProperty("AppendToFile", false);
  alg->executeAsChildAlg();
}

/**
 * @brief Profile obj func along L1 axis
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::profileL1(Mantid::API::IPeaksWorkspace_sptr &pws,
                                    Mantid::API::IPeaksWorkspace_sptr pws_original) {
  g_log.notice() << "START of profiling objective func along L1\n";

  // control option
  bool verbose = getProperty("VerboseOutput");
  if (verbose) {
    // header to console
    g_log.notice() << "deltaL1 -- residual\n";
  }

  // prepare container for profile information
  std::ostringstream msgrst;
  msgrst.precision(12);
  msgrst << "dL1\tresidual\n";

  // setting up as if we are doing optimization
  auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
  // NOTE: always use the original pws to get the tofs
  std::vector<double> tofs = captureTOF(pws_original);
  objf->setPeakWorkspace(pws, "moderator", tofs);

  // call the obj to perform evaluation
  const int n_peaks = pws->getNumberPeaks();
  std::unique_ptr<double[]> target(new double[n_peaks * 3]);

  // generate the target
  auto ubmatrix = pws->sample().getOrientedLattice().getUB();
  for (int i = 0; i < n_peaks; ++i) {
    V3D qv = ubmatrix * pws->getPeak(i).getIntHKL();
    qv *= 2 * PI;
    for (int j = 0; j < 3; ++j) {
      target[i * 3 + j] = qv[j];
    }
  }

  const double xValues[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // xValues is not used

  // scan from -4cm to 4cm along dL1 where the minimum is supposed to be at 0 for null
  // case with instrument at the engineering position
  double deltaL1 = -4e-2;
  while (deltaL1 < 4e-2) {
    std::unique_ptr<double[]> out(new double[n_peaks * 3]);
    objf->setParameter("DeltaZ", deltaL1);
    objf->setParameter("DeltaT0", 0.0); // need to set dT0 to 0.0 if we are not cali it
    objf->function1D(out.get(), xValues, 1);

    // calc residual
    double residual = 0.0;
    for (int i = 0; i < n_peaks * 3; ++i) {
      residual += (out[i] - target[i]) * (out[i] - target[i]);
    }
    residual = std::sqrt(residual) / (n_peaks - 1); // only 1 deg of freedom here
    // log rst
    msgrst << deltaL1 << "\t" << residual << "\n";

    if (verbose) {
      g_log.notice() << deltaL1 << " -- " << residual << "\n";
    }

    // increment
    deltaL1 += 1e-4; // 0.1mm step size
  }

  // output to file
  auto filenamebase = std::filesystem::temp_directory_path() / "profileSCDCalibratePanels2_L1.csv";
  std::ofstream profL1File;
  profL1File.open(filenamebase.string());
  profL1File << msgrst.str();
  profL1File.close();
  g_log.notice() << "Profile data is saved at:\n"
                 << filenamebase << "\n"
                 << "END of profiling objective func along L1\n";
}

/**
 * @brief Profiling obj func along six degree of freedom, which can be very slow.
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::profileBanks(Mantid::API::IPeaksWorkspace_sptr &pws,
                                       const Mantid::API::IPeaksWorkspace_sptr &pws_original) {
  g_log.notice() << "START of profiling all banks along six degree of freedom\n";

  // control option
  bool verbose = getProperty("VerboseOutput");
  if (verbose) {
    // header to console
    g_log.notice() << "--bankname: residual\n";
  }

  // Use OPENMP to speed up the profiling
  PARALLEL_FOR_IF(Kernel::threadSafe(*pws))
  for (int bankIndex = 0; bankIndex < static_cast<int>(m_BankNames.size()); ++bankIndex) {
    PARALLEL_START_INTERRUPT_REGION
    // prepare local copies to work with
    const std::string bankname = *std::next(m_BankNames.begin(), bankIndex);
    const std::string pwsBankiName = "_pws_" + bankname;

    //-- step 0: extract peaks that lies on the current bank
    IPeaksWorkspace_sptr pwsBanki = selectPeaksByBankName(pws, bankname, pwsBankiName);
    //   get tofs from the original subset of pws
    IPeaksWorkspace_sptr pwsBanki_original = selectPeaksByBankName(pws_original, bankname, pwsBankiName);
    std::vector<double> tofs = captureTOF(pwsBanki_original);

    // Do not attempt correct panels with less than 6 peaks as the system will
    // be under-determined
    int nBankPeaks = pwsBanki->getNumberPeaks();
    if (nBankPeaks < MINIMUM_PEAKS_PER_BANK) {
      // use ostringstream to prevent OPENMP breaks log info
      std::ostringstream msg_npeakCheckFail;
      msg_npeakCheckFail << "-- Cannot profile Bank " << bankname << " have only " << nBankPeaks << " (<"
                         << MINIMUM_PEAKS_PER_BANK << ") Peaks, skipping\n";
      g_log.notice() << msg_npeakCheckFail.str();
      continue;
    }

    //
    MatrixWorkspace_sptr wsBankCali = getIdealQSampleAsHistogram1D(pwsBanki);
    std::ostringstream msgrst;
    msgrst.precision(12);
    msgrst << "dx\tdy\tdz\ttheta\tphi\trogang\tresidual\n";
    //
    auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
    objf->setPeakWorkspace(pwsBanki, bankname, tofs);
    //
    const int n_peaks = pwsBanki->getNumberPeaks();
    std::unique_ptr<double[]> target(new double[n_peaks * 3]);
    // generate the target
    auto ubmatrix = pwsBanki->sample().getOrientedLattice().getUB();
    for (int i = 0; i < n_peaks; ++i) {
      V3D qv = ubmatrix * pwsBanki->getPeak(i).getIntHKL();
      qv *= 2 * PI;
      for (int j = 0; j < 3; ++j) {
        target[i * 3 + j] = qv[j];
      }
    }

    const double xValues[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // xValues is not used

    // NOTE: very expensive scan of the parameter space
    for (double dx = -1e-2; dx < 1e-2; dx += 2e-2 / 20.0) {
      // deltaX: meter
      for (double dy = -1e-2; dy < 1e-2; dy += 2e-2 / 20.0) {
        // deltaY: meter
        for (double dz = -1e-2; dz < 1e-2; dz += 2e-2 / 20.0) {
          // deltaZ: meter
          for (double theta = 0.0; theta < PI; theta += PI / 20.0) {
            // theta: rad
            for (double phi = 0.0; phi < 2 * PI; phi += 2 * PI / 20.0) {
              // phi: rad
              for (double ang = -5.0; ang < 5.0; ang += 5.0 / 20.0) {
                // ang: degrees
                // configure the objfunc
                std::unique_ptr<double[]> out(new double[n_peaks * 3]);
                objf->setParameter("DeltaX", dx);
                objf->setParameter("DeltaY", dy);
                objf->setParameter("DeltaZ", dz);
                objf->setParameter("Theta", theta);
                objf->setParameter("Phi", phi);
                objf->setParameter("DeltaRotationAngle", ang);
                objf->setParameter("DeltaT0", 0.0); // need to set dT0 to 0.0 if we are not cali it
                objf->function1D(out.get(), xValues, 1);
                // calc residual
                double residual = 0.0;
                for (int i = 0; i < n_peaks * 3; ++i) {
                  residual += (out[i] - target[i]) * (out[i] - target[i]);
                }
                residual = std::sqrt(residual) / (n_peaks - 6);
                // record
                msgrst << dx << "\t" << dy << "\t" << dz << "\t" << theta << "\t" << phi << "\t" << ang << "\t"
                       << residual << "\n";

                if (verbose) {
                  g_log.notice() << "--" << bankname << ": " << residual << "\n";
                }
              }
            }
          }
        }
      }
    }

    // output to file
    const std::string csvname = "profileSCDCalibratePanels2_" + bankname + ".csv";
    auto filenamebase = std::filesystem::temp_directory_path() / csvname;
    std::ofstream profBankFile;
    profBankFile.open(filenamebase.string());
    profBankFile << msgrst.str();
    profBankFile.close();

    // notify at the terminal
    std::ostringstream msg;
    msg << "Profile of " << bankname << " is saved at:\n"
        << filenamebase << "\n"
        << "END of profiling objective func for " << bankname << "\n";
    g_log.notice() << msg.str();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

/**
 * @brief Profile obj func along T0 axis
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::profileT0(Mantid::API::IPeaksWorkspace_sptr &pws,
                                    Mantid::API::IPeaksWorkspace_sptr pws_original) {
  g_log.notice() << "START of profiling objective func along T0\n";

  // control option
  bool verbose = getProperty("VerboseOutput");
  if (verbose) {
    // print the header to console
    g_log.notice() << "deltaT0 -- residual\n";
  }

  // prepare container for profile information
  std::ostringstream msgrst;
  msgrst.precision(12);
  msgrst << "dT0\tresidual\n";

  // setting up as if we are doing optimization
  auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
  // NOTE: always use the original pws to get the tofs
  std::vector<double> tofs = captureTOF(pws_original);
  objf->setPeakWorkspace(pws, "none", tofs);

  // generate the target
  const int n_peaks = pws->getNumberPeaks();
  std::unique_ptr<double[]> target(new double[n_peaks * 3]);
  auto ubmatrix = pws->sample().getOrientedLattice().getUB();
  for (int i = 0; i < n_peaks; ++i) {
    V3D qv = ubmatrix * pws->getPeak(i).getIntHKL();
    qv *= 2 * PI;
    for (int j = 0; j < 3; ++j) {
      target[i * 3 + j] = qv[j];
    }
  }

  const double xValues[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // xValues is not used

  // scan from -10 ~ 10 ms along dT0
  double deltaT0 = -10;
  while (deltaT0 < 10) {
    std::unique_ptr<double[]> out(new double[n_peaks * 3]);
    objf->setParameter("DeltaT0", deltaT0);
    objf->function1D(out.get(), xValues, 1);

    // calc residual
    double residual = 0.0;
    for (int i = 0; i < n_peaks * 3; ++i) {
      residual += (out[i] - target[i]) * (out[i] - target[i]);
    }
    residual = std::sqrt(residual) / (n_peaks - 1); // only 1 deg of freedom here
    // log rst
    msgrst << deltaT0 << "\t" << residual << "\n";

    if (verbose) {
      g_log.notice() << deltaT0 << " -- " << residual << "\n";
    }

    // increment
    deltaT0 += 0.01; // 20/2000.0
  }

  // output to file
  auto filenamebase = std::filesystem::temp_directory_path() / "profileSCDCalibratePanels2_T0.csv";
  std::ofstream profL1File;
  profL1File.open(filenamebase.string());
  profL1File << msgrst.str();
  profL1File.close();
  g_log.notice() << "Profile data is saved at:\n"
                 << filenamebase << "\n"
                 << "END of profiling objective func along T0\n";
}

/**
 * @brief Profile obj func along L1 and T0 axis
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::profileL1T0(Mantid::API::IPeaksWorkspace_sptr &pws,
                                      Mantid::API::IPeaksWorkspace_sptr pws_original) {
  g_log.notice() << "START of profiling objective func along L1 and T0\n";

  // control option
  bool verbose = getProperty("VerboseOutput");
  if (verbose) {
    // print the header to console
    g_log.notice() << "deltaL1 -- deltaT0 -- residual\n";
  }

  // prepare container for profile information
  std::ostringstream msgrst;
  msgrst.precision(12);
  msgrst << "dL1\tdT0\tresidual\n";

  // setting up as if we are doing optimization
  auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
  // NOTE: always use the original pws to get the tofs
  std::vector<double> tofs = captureTOF(pws_original);
  objf->setPeakWorkspace(pws, "moderator", tofs);

  // generate the target
  const int n_peaks = pws->getNumberPeaks();
  std::unique_ptr<double[]> target(new double[n_peaks * 3]);
  auto ubmatrix = pws->sample().getOrientedLattice().getUB();
  for (int i = 0; i < n_peaks; ++i) {
    V3D qv = ubmatrix * pws->getPeak(i).getIntHKL();
    qv *= 2 * PI;
    for (int j = 0; j < 3; ++j) {
      target[i * 3 + j] = qv[j];
    }
  }

  const double xValues[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // xValues is not used

  // profile begin
  for (double deltaL1 = -4e-2; deltaL1 < 4e-2; deltaL1 += 1e-4) {
    for (double deltaT0 = -4.0; deltaT0 < 4.0; deltaT0 += 1e-2) {
      std::unique_ptr<double[]> out(new double[n_peaks * 3]);
      objf->setParameter("DeltaZ", deltaL1);
      objf->setParameter("DeltaT0", deltaT0);
      objf->function1D(out.get(), xValues, 1);

      // calc residual
      double residual = 0.0;
      for (int i = 0; i < n_peaks * 3; ++i) {
        residual += (out[i] - target[i]) * (out[i] - target[i]);
      }
      residual = std::sqrt(residual) / (n_peaks - 2); // only 1 deg of freedom here

      if (verbose) {
        g_log.notice() << deltaL1 << " -- " << deltaT0 << " -- " << residual << "\n";
      }
      // log rst
      msgrst << deltaL1 << "\t" << deltaT0 << "\t" << residual << "\n";
    }
  }

  // output to file
  auto filenamebase = std::filesystem::temp_directory_path() / "profileSCDCalibratePanels2_L1T0.csv";
  std::ofstream profL1File;
  profL1File.open(filenamebase.string());
  profL1File << msgrst.str();
  profL1File.close();

  // log
  g_log.notice() << "Profile data is saved at:\n"
                 << filenamebase << "\n"
                 << "END of profiling objective func along L1 and T0\n";
}

/**
 * @brief Retrieve "scalex" and "scaley" from a workspace's parameter map if the component is rectangular detector
 *
 * The default cases for the return value includes
 * 1. the bank is not a rectagular detector OR
 * 2. the component does not have "scalex" and "scaley" in parameter map
 *
 * @param instrument :: Instrument geometry
 * @param bankname :: bank name (component name)
 * @param pmap :: parameter map from the same workspace where instrument is belonged to
 * @return :: pair as scalex and scaley. Default is (1., 1.)
 */
std::pair<double, double>
SCDCalibratePanels2::getRectangularDetectorScaleFactors(std::shared_ptr<Geometry::Instrument> &instrument,
                                                        const std::string &bankname,
                                                        const Geometry::ParameterMap &pmap) {

  std::pair<double, double> scales{1.0, 1.0};

  // docalibsize, sizesearchradius, fixdetxyratio
  Geometry::IComponent_const_sptr comp = instrument->getComponentByName(bankname);
  std::shared_ptr<const Geometry::RectangularDetector> rectDet =
      std::dynamic_pointer_cast<const Geometry::RectangularDetector>(comp);

  if (rectDet) {
    // retrieve the (scalex, scaley) stored in the workspace for this bank/component
    auto scalexparams = pmap.getDouble(rectDet->getName(), "scalex");
    auto scaleyparams = pmap.getDouble(rectDet->getName(), "scaley");
    if (!scalexparams.empty())
      scales.first = scalexparams[0];
    if (!scaleyparams.empty())
      scales.second = scaleyparams[0];
  }

  return scales;
}

} // namespace Mantid::Crystal
