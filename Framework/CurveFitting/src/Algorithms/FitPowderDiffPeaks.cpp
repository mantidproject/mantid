// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/FitPowderDiffPeaks.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Statistics.h"

#include "MantidAPI/Column.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidCurveFitting/FuncMinimizers/DampedGaussNewtonMinimizer.h"
#include "MantidCurveFitting/Functions/BackToBackExponential.h"
#include "MantidCurveFitting/Functions/BackgroundFunction.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/Polynomial.h"
#include "MantidCurveFitting/Functions/ThermalNeutronBk2BkExpConvPVoigt.h"
#include "MantidCurveFitting/Functions/ThermalNeutronDtoTOFFunction.h"

#include <cmath>
#include <utility>

#include <gsl/gsl_sf_erf.h>

/// Factor on FWHM for fitting a peak
#define PEAKFITRANGEFACTOR 5.0

/// Factor on FWHM for defining a peak's range
#define PEAKBOUNDARYFACTOR 2.5

/// Factor on FWHM for excluding peak to fit background
#define EXCLUDEPEAKRANGEFACTOR 8.0
/// Factor on FWHM to fit a peak
#define WINDOWSIZE 3.0

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting::Constraints;

using namespace std;

namespace Mantid::CurveFitting::Algorithms {

DECLARE_ALGORITHM(FitPowderDiffPeaks)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
FitPowderDiffPeaks::FitPowderDiffPeaks()
    : m_wsIndex(-1), m_tofMin(0.), m_tofMax(0.), m_useGivenTOFh(false), m_confidentInInstrumentParameters(false),
      m_minimumHKL(), m_numPeaksLowerToMin(-1), m_indexGoodFitPeaks(), m_chi2GoodFitPeaks(), m_fitMode(ROBUSTFIT),
      m_genPeakStartingValue(HKLCALCULATION), m_rightmostPeakHKL(), m_rightmostPeakLeftBound(0.),
      m_rightmostPeakRightBound(0.), m_minPeakHeight(0.), m_unitCell(), m_fitPeakBackgroundComposite(false) {}

//----------------------------------------------------------------------------------------------
/** Parameter declaration
 */
void FitPowderDiffPeaks::init() {
  // Input data workspace
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "Anonymous", Direction::Input),
                  "Input workspace for data (diffraction pattern). ");

  // Output workspace
  declareProperty(std::make_unique<WorkspaceProperty<Workspace2D>>("OutputWorkspace", "Anonymous2", Direction::Output),
                  "Output Workspace2D for the fitted peaks. ");

  // Input/output peaks table workspace
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("BraggPeakParameterWorkspace", "AnonymousPeak",
                                                                      Direction::Input),
                  "TableWorkspace containg all peaks' parameters.");

  // Input and output instrument parameters table workspace
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("InstrumentParameterWorkspace",
                                                                      "AnonymousInstrument", Direction::InOut),
                  "TableWorkspace containg instrument's parameters.");

  // Workspace to output fitted peak parameters
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("OutputBraggPeakParameterWorkspace",
                                                                      "AnonymousOut2", Direction::Output),
                  "Output TableWorkspace containing the fitted peak parameters for each "
                  "peak.");

  // Data workspace containing fitted peak parameters
  declareProperty(std::make_unique<WorkspaceProperty<Workspace2D>>("OutputBraggPeakParameterDataWorkspace",
                                                                   "ParameterData", Direction::Output),
                  "Output Workspace2D containing fitted peak parameters for "
                  "further refinement.");

  // Zscore table workspace
  declareProperty(
      std::make_unique<WorkspaceProperty<TableWorkspace>>("OutputZscoreWorkspace", "ZscoreTable", Direction::Output),
      "Output TableWorkspace containing the Zscore of the fitted "
      "peak parameters. ");

  // Workspace index of the
  declareProperty("WorkspaceIndex", 0, "Worskpace index for the data to refine against.");

  // Range of the peaks to fit
  declareProperty("MinTOF", EMPTY_DBL(), "Minimum TOF to fit peaks.  ");
  declareProperty("MaxTOF", EMPTY_DBL(), "Maximum TOF to fit peaks.  ");

  vector<string> fitmodes(2);
  fitmodes[0] = "Robust";
  fitmodes[1] = "Confident";
  auto fitvalidator = std::make_shared<StringListValidator>(fitmodes);
  declareProperty("FittingMode", "Robust", fitvalidator,
                  "Fitting mode such that user can determine"
                  "whether the input parameters are trustful or not.");

  // Option to calculate peak position from (HKL) and d-spacing data
  declareProperty("UseGivenPeakCentreTOF", true,
                  "Use each Bragg peak's centre in TOF given in "
                  "BraggPeakParameterWorkspace."
                  "Otherwise, calculate each peak's centre from d-spacing.");

  vector<string> genpeakoptions{"(HKL) & Calculation", "From Bragg Peak Table"};
  auto propvalidator = std::make_shared<StringListValidator>(genpeakoptions);
  declareProperty("PeakParametersStartingValueFrom", "(HKL) & Calculation", propvalidator,
                  "Choice of how to generate starting values of "
                  "Bragg peak profile parmeters.");

  declareProperty("MinimumPeakHeight", 0.20,
                  "Minimum peak height (with background removed) "
                  "Any peak whose maximum height under this value will be "
                  "treated as zero intensity. ");

  // Flag to calculate and trust peak parameters from instrument
  // declareProperty("ConfidentInInstrumentParameters", false, "Option to
  // calculate peak parameters from "
  //     "instrument parameters.");

  // Option to denote that peaks are related
  declareProperty("PeaksCorrelated", false,
                  "Flag for fact that all peaks' corresponding profile parameters "
                  "are correlated by an analytical function");

  // Option for peak's HKL for minimum d-spacing
  auto arrayprop = std::make_unique<ArrayProperty<int>>("MinimumHKL", "");
  declareProperty(std::move(arrayprop), "Miller index of the left most peak (peak with "
                                        "minimum d-spacing) to be fitted. ");

  // Number of the peaks to fit left to peak with minimum HKL
  declareProperty("NumberPeaksToFitBelowLowLimit", 0,
                  "Number of peaks to fit with d-spacing value "
                  "less than specified minimum. ");

  // Right most peak property
  auto righthklprop = std::make_unique<ArrayProperty<int>>("RightMostPeakHKL", "");
  declareProperty(std::move(righthklprop), "Miller index of the right most peak. "
                                           "It is only required and used in RobustFit mode.");

  declareProperty("RightMostPeakLeftBound", EMPTY_DBL(),
                  "Left bound of the right most peak. "
                  "Used in RobustFit mode.");

  declareProperty("RightMostPeakRightBound", EMPTY_DBL(),
                  "Right bound of the right most peak. "
                  "Used in RobustFit mode.");

  // Fit option
  declareProperty("FitCompositePeakBackground", true,
                  "Flag to do fit to both peak and background in a composite "
                  "function as last fit step.");
}

//----------------------------------------------------------------------------------------------
/** Main execution
 */
void FitPowderDiffPeaks::exec() {
  // 1. Get input
  processInputProperties();

  // 2. Crop input workspace
  cropWorkspace(m_tofMin, m_tofMax);

  // 3. Parse input table workspace
  importInstrumentParameterFromTable(m_profileTable);

  // 4. Unit cell
  double latticesize = getParameter("LatticeConstant");
  if (latticesize == EMPTY_DBL())
    throw runtime_error("Input instrument table workspace lacks LatticeConstant. "
                        "Unable to complete processing.");
  m_unitCell.set(latticesize, latticesize, latticesize, 90.0, 90.0, 90.0);

  // 5. Generate peaks
  genPeaksFromTable(m_peakParamTable);

  // 6. Fit peaks & get peak centers
  m_indexGoodFitPeaks.clear();
  m_chi2GoodFitPeaks.clear();
  size_t numpts = m_dataWS->x(m_wsIndex).size();
  m_peakData.resize(numpts);

  g_log.information() << "[FitPeaks] Total Number of Peak = " << m_vecPeakFunctions.size() << '\n';
  m_peakFitChi2.resize(m_vecPeakFunctions.size(), -1.0 * DBL_MIN);
  m_goodFit.resize(m_vecPeakFunctions.size(), false);

  if (m_fitMode == ROBUSTFIT) {
    g_log.information() << "Fit (Single) Peaks In Robust Mode.\n";
    fitPeaksRobust();
  } else if (m_fitMode == TRUSTINPUTFIT) {
    g_log.information() << "Fit Peaks In Trust Mode.  Possible to fit "
                           "overlapped "
                           "peaks.\n";
    fitPeaksWithGoodStartingValues();
  } else {
    g_log.error("Unsupported fit mode!");
    throw runtime_error("Unsupported fit mode.");
  }

  // 5. Create Output
  // a) Create a Table workspace for peak profile
  pair<TableWorkspace_sptr, TableWorkspace_sptr> tables = genPeakParametersWorkspace();
  TableWorkspace_sptr outputpeaksws = tables.first;
  TableWorkspace_sptr ztablews = tables.second;
  setProperty("OutputBraggPeakParameterWorkspace", outputpeaksws);
  setProperty("OutputZscoreWorkspace", ztablews);

  // b) Create output data workspace (as a middle stage product)
  Workspace2D_sptr outdataws = genOutputFittedPatternWorkspace(m_peakData, m_wsIndex);
  setProperty("OutputWorkspace", outdataws);

  // c) Create data workspace for X0, A, B and S of peak with good fit
  Workspace2D_sptr peakparamvaluews = genPeakParameterDataWorkspace();
  setProperty("OutputBraggPeakParameterDataWorkspace", peakparamvaluews);
}

//----------------------------------------------------------------------------------------------
/** Process input parameters
 */
void FitPowderDiffPeaks::processInputProperties() {
  // data workspace
  m_dataWS = this->getProperty("InputWorkspace");
  m_wsIndex = this->getProperty("WorkspaceIndex");
  if (m_wsIndex < 0 || m_wsIndex > static_cast<int>(m_dataWS->getNumberHistograms())) {
    stringstream errss;
    errss << "Input workspace = " << m_wsIndex << " is out of range [0, " << m_dataWS->getNumberHistograms();
    g_log.error(errss.str());
    throw std::invalid_argument(errss.str());
  }

  // table workspaces for parameters
  m_peakParamTable = this->getProperty("BraggPeakParameterWorkspace");
  m_profileTable = this->getProperty("InstrumentParameterWorkspace");

  // fitting range
  m_tofMin = getProperty("MinTOF");
  m_tofMax = getProperty("MaxTOF");
  if (m_tofMin == EMPTY_DBL())
    m_tofMin = m_dataWS->x(m_wsIndex)[0];
  if (m_tofMax == EMPTY_DBL())
    m_tofMax = m_dataWS->x(m_wsIndex).back();

  m_minimumHKL = getProperty("MinimumHKL");
  m_numPeaksLowerToMin = getProperty("NumberPeaksToFitBelowLowLimit");

  // fitting algorithm option
  string fitmode = getProperty("FittingMode");
  if (fitmode == "Robust") {
    m_fitMode = ROBUSTFIT;
  } else if (fitmode == "Confident") {
    m_fitMode = TRUSTINPUTFIT;
  } else {
    throw runtime_error("Input fit mode can only accept either Robust or Confident. ");
  }

  m_useGivenTOFh = getProperty("UseGivenPeakCentreTOF");
  // m_confidentInInstrumentParameters =
  // getProperty("ConfidentInInstrumentParameters");

  // peak parameter generation option
  string genpeakparamalg = getProperty("PeakParametersStartingValueFrom");
  if (genpeakparamalg == "(HKL) & Calculation") {
    m_genPeakStartingValue = HKLCALCULATION;
  } else if (genpeakparamalg == "From Bragg Peak Table") {
    m_genPeakStartingValue = FROMBRAGGTABLE;
  } else {
    throw runtime_error("Input option from PeakParametersStaringValueFrom is not supported.");
  }

  // Right most peak information
  m_rightmostPeakHKL = getProperty("RightMostPeakHKL");
  m_rightmostPeakLeftBound = getProperty("RightMostPeakLeftBound");
  m_rightmostPeakRightBound = getProperty("RightMostPeakRightBound");

  if (m_fitMode == ROBUSTFIT) {
    if (m_rightmostPeakHKL.empty() || m_rightmostPeakLeftBound == EMPTY_DBL() ||
        m_rightmostPeakRightBound == EMPTY_DBL()) {
      stringstream errss;
      errss << "If fit mode is 'RobustFit', then user must specify all 3 "
               "properties of right most peak "
            << "(1) Miller Index   (given size  = " << m_rightmostPeakHKL.size() << "), "
            << "(2) Left boundary  (given value = " << m_rightmostPeakLeftBound << "), "
            << "(3) Right boundary (given value = " << m_rightmostPeakRightBound << "). ";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }
  }

  m_minPeakHeight = getProperty("MinimumPeakHeight");

  m_fitPeakBackgroundComposite = getProperty("FitCompositePeakBackground");
}

//=================================  Fit Peaks In Robust Mode
//==================================

//----------------------------------------------------------------------------------------------
/** Fit peaks in Robust mode.
 * Prerequisite:
 * 1. There are not any peaks that overlap to others;
 * Algorithm: All peaks are fit individually
 * Challenge:
 *   1. Starting geometry parameters can be off
 *   2. Peak profile parameters cannot be trusted at all.
 */
void FitPowderDiffPeaks::fitPeaksRobust() {
  // I. Prepare
  BackToBackExponential_sptr rightpeak;
  bool isrightmost = true;
  size_t numpeaks = m_vecPeakFunctions.size();
  if (numpeaks == 0)
    throw runtime_error("There is no peak to fit!");

  vector<string> peakparnames = m_vecPeakFunctions[0].second.second->getParameterNames();

  // II. Create local background function.
  Polynomial_sptr backgroundfunction = std::make_shared<Polynomial>();
  backgroundfunction->setAttributeValue("n", 1);
  backgroundfunction->initialize();

  // III. Fit peaks
  double chi2;
  double refpeakshift = 0.0;

  for (int peakindex = static_cast<int>(numpeaks) - 1; peakindex >= 0; --peakindex) {
    vector<int> peakhkl = m_vecPeakFunctions[peakindex].second.first;
    BackToBackExponential_sptr thispeak = m_vecPeakFunctions[peakindex].second.second;

    stringstream infoss;

    bool goodfit = false;

    if (isrightmost && peakhkl == m_rightmostPeakHKL) {
      // It is the specified right most peak.  Estimate background, peak height,
      // fwhm, ...
      // 1. Determine the starting value of the peak
      double peakleftbound, peakrightbound;
      peakleftbound = m_rightmostPeakLeftBound;
      peakrightbound = m_rightmostPeakRightBound;

      double predictpeakcentre = thispeak->centre();

      infoss << "[DBx102] The " << numpeaks - 1 - peakindex << "-th rightmost peak's miller index = " << peakhkl[0]
             << ", " << peakhkl[1] << ", " << peakhkl[2] << ", predicted at TOF = " << thispeak->centre()
             << ";  User specify boundary = [" << peakleftbound << ", " << peakrightbound << "].";
      g_log.information() << infoss.str() << '\n';

      map<string, double> rightpeakparameters;
      goodfit = fitSinglePeakRobust(thispeak, std::dynamic_pointer_cast<BackgroundFunction>(backgroundfunction),
                                    peakleftbound, peakrightbound, rightpeakparameters, chi2);

      m_peakFitChi2[peakindex] = chi2;

      if (!goodfit)
        throw runtime_error("Failed to fit the right most peak.  Unable to process. ");

      stringstream robmsgss;
      for (auto &parname : peakparnames) {
        robmsgss << parname << " = " << thispeak->getParameter(parname) << '\n';
      }
      g_log.information() << "[DB1151] Robust Fit Result:   Chi^2 = " << chi2 << '\n' << robmsgss.str();

      rightpeak = thispeak;
      isrightmost = false;

      // iii. Reference peak shift
      refpeakshift = thispeak->centre() - predictpeakcentre;

      g_log.notice() << "[DBx332] Peak -" << static_cast<int>(numpeaks) - peakindex - 1
                     << ": shifted = " << refpeakshift << '\n';
    } else if (!isrightmost) {
      // All peaks but not the right most peak
      // 1. Validate inputs
      if (peakindex == static_cast<int>(numpeaks) - 1)
        throw runtime_error("Impossible to have peak index as the right most peak here!");

      double predictcentre = thispeak->centre();

      // 2. Determine the peak range by observation
      double peakleftbound, peakrightbound;
      observePeakRange(thispeak, rightpeak, refpeakshift, peakleftbound, peakrightbound);

      stringstream dbxss;
      dbxss << '\n';
      for (int i = 0; i < 10; ++i)
        dbxss << "==\n[DBx323] Peak (" << peakhkl[0] << ", " << peakhkl[1] << "," << peakhkl[2]
              << ").  Centre predicted @ TOF = " << predictcentre << ".  Observed range = " << peakleftbound << ", "
              << peakrightbound;
      g_log.notice(dbxss.str());

      // 3. Fit peak
      /* Disabled and replaced by fit1PeakRobust
      goodfit = fitSinglePeakRefRight(thispeak, backgroundfunction, rightpeak,
      peakleftbound,
                                      peakrightbound, chi2);
                                      */

      map<string, double> rightpeakparameters;
      storeFunctionParameters(rightpeak, rightpeakparameters);
      goodfit = fitSinglePeakRobust(thispeak, std::dynamic_pointer_cast<BackgroundFunction>(backgroundfunction),
                                    peakleftbound, peakrightbound, rightpeakparameters, chi2);

      if (goodfit) {
        // Fit successful
        m_peakFitChi2[peakindex] = chi2;
        // Update right peak and reference peak shift if peak is not trivial
        if (thispeak->height() >= m_minPeakHeight) {
          rightpeak = thispeak;
          refpeakshift = thispeak->centre() - predictcentre;
        }

      } else {
        // Bad fit
        m_peakFitChi2[peakindex] = -1.0;
        g_log.warning() << "Fitting peak @ " << thispeak->centre() << " failed. \n";
      }

    } else {
      // It is right to the specified right most peak.  Skip to next peak
      double peakleftbound, peakrightbound;
      peakleftbound = m_rightmostPeakLeftBound;
      peakrightbound = m_rightmostPeakRightBound;

      infoss << "[DBx102] The " << numpeaks - 1 - peakindex << "-th rightmost peak's miller index = " << peakhkl[0]
             << ", " << peakhkl[1] << ", " << peakhkl[2] << ", predicted at TOF = " << thispeak->centre() << "; "
             << "User specify right most peak's miller index = " << m_rightmostPeakHKL[0] << ", "
             << m_rightmostPeakHKL[1] << ", " << m_rightmostPeakHKL[2] << " User specify boundary = [" << peakleftbound
             << ", " << peakrightbound << "].";
      g_log.information() << infoss.str() << '\n';
      continue;
    }

  } // ENDFOR Peaks
}

//----------------------------------------------------------------------------------------------
/** Observe peak range with hint from right peak's properties
 * Assumption: the background is reasonably flat within peak range
 */
void FitPowderDiffPeaks::observePeakRange(const BackToBackExponential_sptr &thispeak,
                                          const BackToBackExponential_sptr &rightpeak, double refpeakshift,
                                          double &peakleftbound, double &peakrightbound) {
  double predictcentre = thispeak->centre();
  double rightfwhm = rightpeak->fwhm();

  // 1. Roughly determine the peak range from this peak's starting values and
  // right peak' fitted
  //    parameters values
  if (refpeakshift > 0) {
    // tend to shift to right
    peakleftbound = predictcentre - rightfwhm;
    peakrightbound = predictcentre + rightfwhm + refpeakshift;
  } else {
    // tendency to shift to left
    peakleftbound = predictcentre - rightfwhm + refpeakshift;
    peakrightbound = predictcentre + rightfwhm;
  }
  if (peakrightbound > rightpeak->centre() - 3 * rightpeak->fwhm()) {
    // the search of peak's right end shouldn't exceed the left tail of its real
    // right peak!
    // Remember this is robust mode.  Any 2 adjacent peaks should be faw enough.
    peakrightbound = rightpeak->centre() - 3 * rightpeak->fwhm();
  }

  // 2. Search for maximum
  const auto &vecX = m_dataWS->x(m_wsIndex);

  size_t icentre = findMaxValue(m_dataWS, m_wsIndex, peakleftbound, peakrightbound);
  double peakcentre = vecX[icentre];

  // 3. Narrow now the peak range
  peakleftbound = vecX[icentre] - 4.0 * rightfwhm;
  peakrightbound = vecX[icentre] + 4.0 * rightfwhm;

  double rightpeakleftbound = rightpeak->centre() - 3 * rightfwhm;
  if (peakrightbound > rightpeakleftbound) {
    peakrightbound = rightpeakleftbound;
    if (peakrightbound < 2.0 * rightfwhm + peakcentre)
      g_log.warning() << "Peak @ " << peakcentre << "'s right boundary is too close to its right peak!\n";
  }
}

//----------------------------------------------------------------------------------------------
/** Fit a single peak including its background by a robust algorithm
 * Algorithm will
 *  1. Locate Maximum
 *  2.
 *
 * Assumption:
 * 1. peak must be in the range of [input peak center - leftdev, + rightdev]
 *
 * Prerequisites:
 * ---- NONE!
 *
 * Algorithms:
 *  1. Build partial workspace for peak
 *  2. Estimate background
 *  3. Estimate peak position and height (by observing)
 *  4. Fit peak by Gaussian for more accurate peak position, height and sigma
 *
 * Peak Fit Algorithms: 4 different approaches are used.  3 of them use normal
 *minimzers such that
 * 1. Use X0 and I calculated by Gaussian, then use A, B, S from input;
 * 2. Use X0, I and S calculated by Gaussian, then use A and B from input;
 * 3. Use X0, I and S calculated by Gaussian, then use A and B from right peak;
 * 4. Use X0, I and S calculated by Gaussian, then use simulated annealing on A
 *and B;
 *
 * Arguments
 * @param peak :: A peak function.
 * @param backgroundfunction :: A background function
 * @param peakleftbound :: peak left bound
 * @param peakrightbound :: peak right bound
 * @param rightpeakparammap :: peakrightbound
 * @param finalchi2   ::  (output) chi square of the fit result
 *
 * Arguments:
 * 1. leftdev, rightdev:  search range for the peak from the estimatio
 *(theoretical)
 * Return: chi2 ... all the other parameter should be just in peak
 */
bool FitPowderDiffPeaks::fitSinglePeakRobust(const BackToBackExponential_sptr &peak,
                                             const BackgroundFunction_sptr &backgroundfunction, double peakleftbound,
                                             double peakrightbound, const map<string, double> &rightpeakparammap,
                                             double &finalchi2) {
  // 1. Build partial workspace
  Workspace2D_sptr peakws = buildPartialWorkspace(m_dataWS, m_wsIndex, peakleftbound, peakrightbound);
  g_log.debug() << "[DB1208] Build partial workspace for peak @ " << peak->centre() << " (predicted).\n";

  // 2. Estimate and remove background
  size_t rawdata_wsindex = 0;
  size_t estbkgd_wsindex = 2;
  size_t peak_wsindex = 1;
  estimateBackgroundCoarse(peakws, backgroundfunction, rawdata_wsindex, estbkgd_wsindex, peak_wsindex);

  stringstream dbss;
  dbss << "[DBx203] Removed background peak data: \n";
  for (size_t i = 0; i < peakws->x(peak_wsindex).size(); ++i)
    dbss << peakws->x(peak_wsindex)[i] << "\t\t" << peakws->y(peak_wsindex)[i] << "\t\t" << peakws->e(peak_wsindex)[i]
         << '\n';
  g_log.debug(dbss.str());

  // 3. Estimate FWHM, peak centre, and height
  double centre, fwhm, height;
  string errmsg;
  bool pass = observePeakParameters(peakws, 1, centre, height, fwhm, errmsg);
  if (!pass) {
// If estiamtion fails, quit b/c first/rightmost peak must be fitted.
#if 0
      g_log.error(errmsg);
      throw runtime_error(errmsg);
#else
    g_log.notice("Unable to observe peak parameters.  Proceed to next peak.");
    return false;
#endif
  } else if (height < m_minPeakHeight) {
    g_log.notice() << "[FLAGx409] Peak proposed @ TOF = " << peak->centre() << " has a trivial "
                   << "peak height = " << height << " by observation.  Skipped.\n";
    return false;
  } else {
    g_log.information() << "[DBx201] Peak Predicted @ TOF = " << peak->centre()
                        << ", Estimated (observation) Centre = " << centre << ", FWHM = " << fwhm
                        << " Height = " << height << '\n';
  }

  // 4. Fit by Gaussian to get some starting value
  double tof_h, sigma;
  doFitGaussianPeak(peakws, peak_wsindex, centre, fwhm, fwhm, tof_h, sigma, height);

  // 5. Fit by various methods
  //    Set all parameters for fit
  vector<string> peakparnames = peak->getParameterNames();
  for (size_t i = 0; i < peakparnames.size(); ++i)
    peak->unfix(i);

  //    Set up the universal starting parameter
  peak->setParameter("I", height * fwhm);
  peak->setParameter("X0", tof_h);

  size_t numsteps = 2;
  vector<string> minimizers(numsteps);
  minimizers[0] = "Simplex";
  minimizers[1] = "Levenberg-Marquardt";
  vector<size_t> maxiterations(numsteps, 10000);
  vector<double> dampfactors(numsteps, 0.0);

  //    Record the start value
  map<string, double> origparammap;
  storeFunctionParameters(peak, origparammap);

  vector<double> chi2s;
  vector<bool> goodfits;
  vector<map<string, double>> solutions;

  // a) Fit by using input peak parameters
  string peakinfoa0 = getFunctionInfo(std::dynamic_pointer_cast<IFunction>(peak));
  g_log.notice() << "[DBx533A] Approach A: Starting Peak Function Information: \n" << peakinfoa0 << '\n';

  double chi2a;
  bool fitgooda = doFit1PeakSequential(peakws, peak_wsindex, peak, minimizers, maxiterations, dampfactors, chi2a);
  map<string, double> solutiona;
  storeFunctionParameters(peak, solutiona);

  chi2s.emplace_back(chi2a);
  goodfits.emplace_back(fitgooda);
  solutions.emplace_back(solutiona);

  string peakinfoa1 = getFunctionInfo(std::dynamic_pointer_cast<IFunction>(peak));
  g_log.notice() << "[DBx533A] Approach A:  Fit Successful = " << fitgooda << ", Chi2 = " << chi2a
                 << ", Peak Function Information: \n"
                 << peakinfoa1 << '\n';

  // b) Fit by using Gaussian result (Sigma)
  restoreFunctionParameters(peak, origparammap);
  peak->setParameter("S", sigma);

  string peakinfob0 = getFunctionInfo(std::dynamic_pointer_cast<IFunction>(peak));
  g_log.notice() << "[DBx533B] Approach B: Starting Peak Function Information: \n" << peakinfob0 << '\n';

  double chi2b;
  bool fitgoodb = doFit1PeakSequential(peakws, peak_wsindex, peak, minimizers, maxiterations, dampfactors, chi2b);

  map<string, double> solutionb;
  storeFunctionParameters(peak, solutionb);

  chi2s.emplace_back(chi2b);
  goodfits.emplace_back(fitgoodb);
  solutions.emplace_back(solutionb);

  string peakinfob1 = getFunctionInfo(std::dynamic_pointer_cast<IFunction>(peak));
  g_log.notice() << "[DBx533B] Approach 2: Fit Successful = " << fitgoodb << ", Chi2 = " << chi2b
                 << ", Peak Function Information: \n"
                 << peakinfob1 << '\n';

  // c) Fit peak parameters by the value from right peak
  if (!rightpeakparammap.empty()) {
    restoreFunctionParameters(peak, rightpeakparammap);
    peak->setParameter("X0", tof_h);
    peak->setParameter("I", height * fwhm);

    string peakinfoc0 = getFunctionInfo(std::dynamic_pointer_cast<IFunction>(peak));
    g_log.notice() << "[DBx533C] Approach C: Starting Peak Function Information: \n" << peakinfoc0 << '\n';

    double chi2c;
    bool fitgoodc = doFit1PeakSequential(peakws, peak_wsindex, peak, minimizers, maxiterations, dampfactors, chi2c);
    map<string, double> solutionc;
    storeFunctionParameters(peak, solutionc);

    chi2s.emplace_back(chi2c);
    goodfits.emplace_back(fitgoodc);
    solutions.emplace_back(solutionc);

    string peakinfoc1 = getFunctionInfo(std::dynamic_pointer_cast<IFunction>(peak));
    g_log.notice() << "[DBx533C] Approach C:  Fit Successful = " << fitgoodc << ", Chi2 = " << chi2c
                   << ", Peak Function Information: \n"
                   << peakinfoc1 << '\n';
  } else {
    // No right peak information: set a error entry
    chi2s.emplace_back(DBL_MAX);
    goodfits.emplace_back(false);
    solutions.emplace_back(rightpeakparammap);
  }

  // 6. Summarize the above 3 approach
  size_t bestapproach = goodfits.size() + 1;
  double bestchi2 = DBL_MAX;
  for (size_t i = 0; i < goodfits.size(); ++i) {
    if (goodfits[i] && chi2s[i] < bestchi2) {
      bestapproach = i;
      bestchi2 = chi2s[i];
    }
  }

  stringstream fitsumss;
  fitsumss << "Best fit result is obtained by approach " << bestapproach << " of total " << goodfits.size()
           << " approaches.  Best Chi^2 = " << bestchi2 << ", Peak Height = " << peak->height();
  g_log.notice() << "[DB1127] " << fitsumss.str() << '\n';

  bool fitgood = true;
  if (bestapproach < goodfits.size()) {
    restoreFunctionParameters(peak, solutions[bestapproach]);
  } else {
    fitgood = false;
  }

  // 7. Fit by Monte Carlo if previous failed
  if (!fitgood) {
    peak->setParameter("S", sigma);
    peak->setParameter("I", height * fwhm);
    peak->setParameter("X0", tof_h);

    vector<string> paramsinmc{"A", "B"};

    fitSinglePeakSimulatedAnnealing(peak, paramsinmc);
  }

  // 8. Fit with background
  if (m_fitPeakBackgroundComposite) {
    // Fit peak + background
    double chi2compf;
    bool fitcompfunsuccess = doFit1PeakBackground(peakws, rawdata_wsindex, peak, backgroundfunction, chi2compf);
    if (fitcompfunsuccess) {
      finalchi2 = chi2compf;
    } else {
      finalchi2 = bestchi2;
      g_log.warning("Fit peak-background composite function failed! Need to "
                    "find out how this case peak value is changed from best "
                    "fit.");
    }
  } else {
    // Flag is turned off
    finalchi2 = bestchi2;
  }

  // 9. Plot function
  FunctionDomain1DVector domain(peakws->x(0).rawData());
  plotFunction(peak, backgroundfunction, domain);

  return fitgood;
}

//----------------------------------------------------------------------------------------------
/** Fit single peak with background to raw data
 * Note 1: in a limited range (4*FWHM)
 */
bool FitPowderDiffPeaks::doFit1PeakBackground(const Workspace2D_sptr &dataws, size_t wsindex,
                                              const BackToBackExponential_sptr &peak,
                                              const BackgroundFunction_sptr &backgroundfunction, double &chi2) {
  // 0. Set fit parameters
  string minimzername("Levenberg-MarquardtMD");
  double startx = peak->centre() - 3.0 * peak->fwhm();
  double endx = peak->centre() + 3.0 * peak->fwhm();

  // 1. Create composite function
  CompositeFunction_sptr compfunc(new CompositeFunction);
  compfunc->addFunction(peak);
  compfunc->addFunction(backgroundfunction);

  // 2. Unfix all parameters
  vector<string> comparnames = compfunc->getParameterNames();
  for (size_t ipar = 0; ipar < comparnames.size(); ++ipar)
    compfunc->unfix(ipar);

  // 3. Fit
  string cominfoa = getFunctionInfo(compfunc);
  g_log.notice() << "[DBx533X-0] Fit All: Starting Peak Function Information: \n"
                 << cominfoa << "Fit range = " << startx << ", " << endx << '\n';

  // 3. Set
  auto fitalg = createChildAlgorithm("Fit", -1, -1, true);
  fitalg->initialize();

  fitalg->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(compfunc));
  fitalg->setProperty("InputWorkspace", dataws);
  fitalg->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fitalg->setProperty("Minimizer", minimzername);
  fitalg->setProperty("CostFunction", "Least squares");
  fitalg->setProperty("MaxIterations", 1000);
  fitalg->setProperty("Output", "FitPeakBackground");
  fitalg->setProperty("StartX", startx);
  fitalg->setProperty("EndX", endx);

  // 3. Execute and parse the result
  bool isexecute = fitalg->execute();
  bool fitsuccess;
  chi2 = DBL_MAX;

  if (isexecute) {
    std::string fitresult = parseFitResult(fitalg, chi2, fitsuccess);

    // Figure out result
    stringstream cominfob;
    cominfob << "[DBx533X] Fit All: Fit Successful = " << fitsuccess << ", Chi^2 = " << chi2 << '\n';
    cominfob << "Detailed info = " << fitresult << '\n';
    string fitinfo = getFunctionInfo(compfunc);
    cominfob << fitinfo;

    g_log.notice(cominfob.str());
  } else {
    g_log.notice() << "[DB1203B] Failed To Fit Peak+Background @ " << peak->centre() << '\n';
    fitsuccess = false;
  }

  return fitsuccess;
}

//----------------------------------------------------------------------------------------------
/** Fit signle peak by Monte Carlo/simulated annealing
 */
bool FitPowderDiffPeaks::fitSinglePeakSimulatedAnnealing(const BackToBackExponential_sptr &peak,
                                                         const vector<string> &paramtodomc) {
  UNUSED_ARG(peak);
  UNUSED_ARG(paramtodomc);
  throw runtime_error("To Be Implemented Soon!");

  /*
  // 6. Fit peak by the result from Gaussian
  pair<bool, double> fitresult;

  double varA = 1.0;
  double varB = 1.0;


  // a) Get initial chi2
  double startchi2 = DBL_MAX;
  doFit1PeakSimple(peakws, 1, peak, "Levenberg-MarquardtMD", 0, newchi2);
  g_log.debug() << "[DBx401] Peak @ TOF = " << peak->centre() << ", Starting
  Chi^2 = "
                << newchi2 << '\n';

  bool continuetofit = true;
  bool goodfit;

  int count = 0;
  size_t numfittohave = 5;
  int maxnumloops = 100;
  vector<pair<double, map<string, double> > > fitparammaps;  // <chi2,
  <parameter, value> >

  map<string, double> curparammap;
  double curchi2 = startchi2;

  srand(0);
  size_t reject = 0;
  size_t count10 = 0;
  double stepratio = 1.0;
  while(continuetofit)
  {
    // a) Store
    storeFunctionParameters(peak, curparammap);

    // b) Monte Carlo
    double aorb = static_cast<double>(rand())/static_cast<double>(RAND_MAX)-0.5;
    double change =
  static_cast<double>(rand())/static_cast<double>(RAND_MAX)-0.5;
    if (aorb > 0)
    {
      // A
      varA = varA*(1+stepratio*change);
      if (varA <= DBL_MIN)
        varA = fabs(varA) + DBL_MIN;
    }
    else
    {
      // B
      varB = varB*(1+stepratio*change);
      if (varB <= DBL_MIN)
        varB = fabs(varB) + DBL_MIN;
    }

    // b) Set A and B
    peak->setParameter("A", varA);
    peak->setParameter("B", varB);

    // b) Fit
    g_log.debug() << "DBx329:  Proposed A = " << varA << ", B = " << varB <<
  '\n';
    fitresult = doFitPeak(peakws, peak, fwhm);

    // c) Get fit result
    goodfit = fitresult.first;
    newchi2 = fitresult.second;

    // d) Take it or not!
    ++ count10;
    if (!goodfit)
    {
      // Bad fit, reverse
      restoreFunctionParameters(peak, curparammap);
      ++ reject;
    }
    else if (newchi2 < curchi2)
    {
      // Good.  Take it
      curchi2 = newchi2;
    }
    else
    {
      // Toss the dice again
      double bar = exp(-(newchi2-curchi2)/curchi2);
      double dice = static_cast<double>(rand())/static_cast<double>(RAND_MAX);
      if (dice < bar)
      {
        // Take it
        curchi2 = newchi2;
      }
      else
      {
        // Reject it
        restoreFunctionParameters(peak, curparammap);
        ++ reject;
      }
    }

    // Adjust step size
    if (count10 >=10 && reject >= 8)
    {
      reject = 10;
      count10 = 0;
      stepratio *= 5;
    }

    // e) Record
    if (goodfit && newchi2 < startchi2)
    {
      // i. Store parameters;
      map<string,double> parammap;
      for (size_t i = 0; i < peakparnames.size(); ++i)
        parammap.emplace(peakparnames[i],
  peak->getParameter(peakparnames[i]));
      fitparammaps.emplace_back(newchi2, parammap);

      // ii. sort
      sort(fitparammaps.begin(), fitparammaps.end());

      // iii. keep size
      if (fitparammaps.size() > numfittohave)
        fitparammaps.pop_back();
    }

    // f) Loop control & MC
    count += 1;
    if (count >= maxnumloops)
    {
      continuetofit = false;
    }
  } // ENDWHILE For Fit

  // 7. Process MC fit results
  if (fitparammaps.size() > 0)
  {
    // Good results
    goodfit = true;
    sort(fitparammaps.begin(), fitparammaps.end());
    newchi2 = fitparammaps[0].first;

    // Set the best result to peak
    for (size_t i = 0; i < peakparnames.size(); ++i)
    {
      string& parname = peakparnames[i];
      map<string, double>& thisparams = fitparammaps[0].second;
      map<string, double>::iterator liter = thisparams.find(parname);
      if (liter == thisparams.end())
      {
        stringstream errmsg;
        errmsg << "In parameter value map, there is no key named " << parname <<
  '\n';
        throw runtime_error(errmsg.str());
      }
      peak->setParameter(parname, liter->second);
    }

    // Plot the peak
    FunctionDomain1DVector domain(peakws->x(1));
    plotFunction(peak, backgroundfunction, domain);

    // Debug print the best solutions
    for (size_t i = 0; i < fitparammaps.size(); ++i)
    {
      double thischi2 = fitparammaps[i].first;
      map<string, double>& thisparams = fitparammaps[i].second;

      g_log.information() << "[DB1055] Chi2 = " << thischi2 << ", A = " <<
  thisparams["A"]
                          << ", B = " << thisparams["B"] << '\n';
    }
  }
  else
  {
    // Bad result
    goodfit = false;
    stringstream errmsg;
    errmsg << "Failed to fit peak @ " << tof_h << " in robust mode!\n";
    g_log.error(errmsg.str());
  }

  return goodfit;
  */

  return false;
}

//==============================  Fit Peaks With Good Starting Values
//==========================

//----------------------------------------------------------------------------------------------
/** Fit individual peak or group of overlapped peaks with good starting values
 *
 * Strategy:
 * 1. From high d-spacing, search for peak or overlapped peaks
 *
 * Output: (1) goodfitpeaks, (2) goodfitchi2
 */
void FitPowderDiffPeaks::fitPeaksWithGoodStartingValues() {
  // 1. Initialize (local) background function
  Polynomial_sptr backgroundfunction = std::make_shared<Polynomial>();
  backgroundfunction->setAttributeValue("n", 1);
  backgroundfunction->initialize();

  // 2. Fit peak / peaks group
  int ipeak = static_cast<int>(m_vecPeakFunctions.size()) - 1;
  double chi2;

  // BackToBackExponential_sptr rightpeak = m_peaks[ipeak].second.second;

  while (ipeak >= 0) {
    // 1. Make a peak group
    vector<size_t> indexpeakgroup;

    bool makegroup = true;
    // Loop over 2nd level: peak groups: 1 peak or overlapped peaks
    while (makegroup) {
      // There is no need to worry about its right neighbor, b/c
      // this situation is already considered as its right neighbor is
      // treated;

      // a) Add this peak,
      BackToBackExponential_sptr thispeak = m_vecPeakFunctions[ipeak].second.second;
      indexpeakgroup.emplace_back(ipeak);

      // b) update the peak index
      --ipeak;

      if (ipeak < 0) {
        // this is last peak.  next peak does not exist
        makegroup = false;
      } else {
        // this is not the last peak.  search the left one.
        double thispeakleftbound = thispeak->centre() - thispeak->fwhm() * 2.5;
        BackToBackExponential_sptr leftpeak = m_vecPeakFunctions[ipeak].second.second;
        double leftpeakrightbound = leftpeak->centre() + leftpeak->fwhm() * 2.5;
        if (thispeakleftbound > leftpeakrightbound) {
          // This peak and next peak is faw enough!
          makegroup = false;
        }
      }
    } // while

    if (indexpeakgroup.size() == 1) {
      // Fit a single peak
      const size_t ifit = indexpeakgroup[0];
      double peakfitleftbound, peakfitrightbound;
      calculatePeakFitBoundary(ifit, ifit, peakfitleftbound, peakfitrightbound);

      g_log.information() << "\n[T] Fit Peak Indexed " << ifit << " (" << m_vecPeakFunctions.size() - 1 - ifit
                          << ")\t----------------------------------\n";

      BackToBackExponential_sptr thispeak = m_vecPeakFunctions[ifit].second.second;
      bool annihilatedpeak;
      m_goodFit[ifit] = fitSinglePeakConfident(thispeak, backgroundfunction, peakfitleftbound, peakfitrightbound, chi2,
                                               annihilatedpeak);
      m_peakFitChi2[ifit] = chi2;
      if (annihilatedpeak)
        thispeak->setHeight(0.0);

      // Debug output
      vector<int> &hkl = m_vecPeakFunctions[ifit].second.first;
      stringstream dbss;
      dbss << "Peak [" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << "] expected @ TOF = " << thispeak->centre()
           << ": \t";
      if (annihilatedpeak)
        dbss << "Annihilated!";
      else
        dbss << "Fit Status = " << m_goodFit[ifit] << ",   Chi2 = " << chi2;
      g_log.information() << "[DB531] " << dbss.str() << '\n';
    } else {
      // Fit overlapped peaks
      vector<BackToBackExponential_sptr> peaksgroup;
      for (auto ipk : indexpeakgroup) {
        BackToBackExponential_sptr temppeak = m_vecPeakFunctions[ipk].second.second;
        peaksgroup.emplace_back(temppeak);
      }

      fitOverlappedPeaks(peaksgroup, backgroundfunction, -1.0);
      // rightpeak = indexpeaksgroup[0];
    }
  } // ENDWHILE

  // 2. Output

  g_log.information() << "DBx415: Number of good fit peaks = " << m_indexGoodFitPeaks.size() << '\n';

  // 3. Clean up
  g_log.information() << "[FitPeaks] Number of peak of good chi2 = " << m_chi2GoodFitPeaks.size() << '\n';
}

//----------------------------------------------------------------------------------------------
/** Fit peak with trustful peak parameters
 * @param peak :: A peak BackToBackExponential function
 * @param backgroundfunction :: A background function
 * @param leftbound ::  left boundary of the peak for fitting
 * @param rightbound :: right boundary of the peak for fitting
 * @param chi2 :: The output chi squared
 * @param annhilatedpeak :: (output) annhilatedpeak
 */
bool FitPowderDiffPeaks::fitSinglePeakConfident(const BackToBackExponential_sptr &peak,
                                                const BackgroundFunction_sptr &backgroundfunction, double leftbound,
                                                double rightbound, double &chi2, bool &annhilatedpeak) {
  // 1. Build the partial workspace
  // a) Determine boundary if necessary
  if (leftbound < 0 || leftbound >= peak->centre())
    leftbound = peak->centre() - 5 * peak->fwhm();
  if (rightbound < 0 || rightbound <= peak->centre())
    rightbound = peak->centre() + 5 * peak->fwhm();

  // b) Build partial
  Workspace2D_sptr peakdataws = buildPartialWorkspace(m_dataWS, m_wsIndex, leftbound, rightbound);

  // 2. Remove background
  estimateBackgroundCoarse(peakdataws, backgroundfunction, 0, 2, 1);

  stringstream dbss2;
  dbss2 << "[T] Partial workspace No Background: \n";
  for (size_t i = 0; i < peakdataws->x(1).size(); ++i)
    dbss2 << peakdataws->x(1)[i] << "\t\t" << peakdataws->y(1)[i] << "\t\t" << peakdataws->e(1)[i] << "\t\t"
          << peakdataws->y(0)[i] << '\n';
  g_log.notice(dbss2.str());

  // 3. Estimate peak heights
  size_t imaxheight = findMaxValue(peakdataws->y(1).rawData());
  double maxheight = peakdataws->y(1)[imaxheight];
  if (maxheight <= m_minPeakHeight) {
    // Max height / peak height is smaller than user defined minimum height.  No
    // fit, Zero
    annhilatedpeak = true;
    return false;
  } else {
    // Max hieght is larger than user defined minimum.  Fit it
    annhilatedpeak = false;
  }

  // 4. Set the constraint and height
  // a) Peak centre
  double peakcentreleftbound = peak->centre() - peak->fwhm();
  double peakcentrerightbound = peak->centre() + peak->fwhm();
  auto x0bc = std::make_unique<BoundaryConstraint>(peak.get(), "X0", peakcentreleftbound, peakcentrerightbound);
  peak->addConstraint(std::move(x0bc));

  // b) A
  auto abc = std::make_unique<BoundaryConstraint>(peak.get(), "A", 1.0E-10, false);
  peak->addConstraint(std::move(abc));

  // c) B
  auto bbc = std::make_unique<BoundaryConstraint>(peak.get(), "B", 1.0E-10, false);
  peak->addConstraint(std::move(bbc));

  // d) Guessed height
  peak->setHeight(maxheight);

  /*  Debug information */
  stringstream dbss0;
  dbss0 << "[DBx100] Peak @" << peak->centre() << ", FWHM = " << peak->fwhm() << '\n';
  vector<string> peakparams = peak->getParameterNames();
  for (size_t i = 0; i < peakparams.size(); ++i)
    dbss0 << peakparams[i] << " = " << peak->getParameter(i) << '\n';
  g_log.notice(dbss0.str());

  // 5. Fit peak with simple scheme
  vector<string> peakparamnames = peak->getParameterNames();
  vector<map<string, double>> fitparamvaluemaps;
  vector<pair<double, size_t>> chi2indexvec;

  // a) Fit peak height
  for (size_t iparam = 0; iparam < peakparamnames.size(); ++iparam) {
    string &parname = peakparams[iparam];
    if (parname == "I")
      peak->unfix(iparam);
    else
      peak->fix(iparam);
  }
  double chi2height;
  bool goodfit1 = doFit1PeakSimple(peakdataws, 1, peak, "Levenberg-MarquardtMD", 10000, chi2height);

  // Store parameters
  map<string, double> step1params;
  storeFunctionParameters(peak, step1params);
  fitparamvaluemaps.emplace_back(step1params);
  if (!goodfit1)
    chi2height = 1.0E20;
  chi2indexvec.emplace_back(chi2height, 0);

  // Fix background
  vector<string> bkgdparnames = backgroundfunction->getParameterNames();
  for (size_t iname = 0; iname < bkgdparnames.size(); ++iname)
    backgroundfunction->fix(iname);

  // b) Plan A: fit all parameters
  double chi2planA;
  for (size_t iparam = 0; iparam < peakparamnames.size(); ++iparam)
    peak->unfix(iparam);
  bool goodfitA = doFit1PeakSimple(peakdataws, 1, peak, "Simplex", 10000, chi2planA);

  // Store A's result
  map<string, double> planAparams;
  storeFunctionParameters(peak, planAparams);
  if (!goodfitA)
    chi2planA = 1.0E20;
  fitparamvaluemaps.emplace_back(planAparams);
  chi2indexvec.emplace_back(chi2planA, 1);

  // c) Plan B: fit parameters in two groups in 2 steps
  // i.   Restore step 1's result
  restoreFunctionParameters(peak, step1params);

  // ii.   Fit peak height and everything else but "A"
  double chi2planB;
  for (size_t iparam = 0; iparam < peakparamnames.size(); ++iparam) {
    string parname = peakparams[iparam];
    if (parname == "A")
      peak->fix(iparam);
    else
      peak->unfix(iparam);
  }
  bool goodfitB1 = doFit1PeakSimple(peakdataws, 1, peak, "Levenberg-MarquardtMD", 10000, chi2planB);

  // iii. Fit "A" only
  for (size_t iparam = 0; iparam < peakparamnames.size(); ++iparam) {
    string parname = peakparams[iparam];
    if (parname == "A" || parname == "I")
      peak->unfix(iparam);
    else
      peak->fix(iparam);
  }
  bool goodfitB2 = doFit1PeakSimple(peakdataws, 1, peak, "Levenberg-MarquardtMD", 10000, chi2planB);

  bool goodfitB = goodfitB1 || goodfitB2;
  map<string, double> planBparams;
  storeFunctionParameters(peak, planBparams);
  if (!goodfitB)
    chi2planB = 1.0E20;
  fitparamvaluemaps.emplace_back(planBparams);
  chi2indexvec.emplace_back(chi2planB, 2);

  // d) Plan C: fit parameters in two groups in 2 steps in alternate order
  // i.   Restore step 1's result
  restoreFunctionParameters(peak, step1params);

  // ii.  Fit "A"
  double chi2planC;
  for (size_t iparam = 0; iparam < peakparamnames.size(); ++iparam) {
    string parname = peakparams[iparam];
    if (parname != "A")
      peak->fix(iparam);
    else
      peak->unfix(iparam);
  }
  bool goodfitC1 = doFit1PeakSimple(peakdataws, 1, peak, "Levenberg-MarquardtMD", 10000, chi2planC);

  // iii. Fit peak height and everything else but "A"
  for (size_t iparam = 0; iparam < peakparamnames.size(); ++iparam) {
    string parname = peakparams[iparam];
    if (parname == "A")
      peak->fix(iparam);
    else
      peak->unfix(iparam);
  }
  bool goodfitC2 = doFit1PeakSimple(peakdataws, 1, peak, "Levenberg-MarquardtMD", 10000, chi2planB);

  bool goodfitC = goodfitC1 || goodfitC2;
  map<string, double> planCparams;
  storeFunctionParameters(peak, planCparams);
  if (!goodfitC)
    chi2planC = 1.0E20;
  fitparamvaluemaps.emplace_back(planCparams);
  chi2indexvec.emplace_back(chi2planC, 3);

  // d) Summarize and compare result
  stringstream sumss;
  sumss << "[DBx833] Confident fit on peak summary: \n";
  for (size_t i = 0; i < 4; ++i)
    sumss << "Index " << i << ": chi^2 = " << chi2indexvec[i].first << '\n';
  g_log.notice(sumss.str());

  sort(chi2indexvec.begin(), chi2indexvec.end());

  bool goodfit;
  if (chi2indexvec[0].first < 1.0E19) {
    // There is good fit.
    size_t goodindex = chi2indexvec[0].second;
    restoreFunctionParameters(peak, fitparamvaluemaps[goodindex]);
    chi2 = chi2indexvec[0].first;
    goodfit = true;
  } else {
    // There is no good fit
    chi2 = DBL_MAX;
    goodfit = false;
  }

  // 6. Plot the peak in the output workspace data
  if (goodfit) {
    FunctionDomain1DVector domain(peakdataws->x(1).rawData());
    plotFunction(peak, backgroundfunction, domain);
  } else {
    // Throw exception if fit peak bad.  This is NOT a PERMANANT solution.
    stringstream errss;
    errss << "Fit Peak @ " << peak->centre() << " Error!  Chi^2 (false) = " << chi2
          << ". Do Not Know How To Proceed To Next Peak!";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  // 7. Debug output
  vector<string> parnames = peak->getParameterNames();
  stringstream debugss;
  debugss << "DB1251 Single Peak Confident Fit Result:  Chi^2 = " << chi2 << '\n';
  for (auto &parname : parnames) {
    debugss << parname << "  =  " << peak->getParameter(parname) << '\n';
  }
  g_log.notice(debugss.str());

  return goodfit;
}

//----------------------------------------------------------------------------------------------
/** Calculate the range to fit peak/peaks group
 * by avoiding including the tails of the neighboring peaks
 *
 * Assumption: all peaks' parameters on centre and FWHM are close to the true
 *value
 *
 * @param ileftpeak ::  index of the left most peak in the peak groups;
 * @param irightpeak :: index of the right most peak in the peak groups;
 * @param peakleftboundary ::  left boundary of the peak for fitting (output)
 * @param peakrightboundary :: right boundary of the peak for fitting (output)
 */
void FitPowderDiffPeaks::calculatePeakFitBoundary(size_t ileftpeak, size_t irightpeak, double &peakleftboundary,
                                                  double &peakrightboundary) {
  BackToBackExponential_sptr leftpeak = m_vecPeakFunctions[ileftpeak].second.second;
  BackToBackExponential_sptr rightpeak = m_vecPeakFunctions[irightpeak].second.second;

  // 1. Determine its left boundary
  peakleftboundary = leftpeak->centre() - PEAKFITRANGEFACTOR * leftpeak->fwhm();

  int ileftneighbor = static_cast<int>(ileftpeak) - 1;
  if (ileftneighbor < 0) {
    // a) No left neighbor, compare to TOF_Min
    if (peakleftboundary < m_tofMin)
      peakleftboundary = m_tofMin;
  } else {
    // b) Compare to the right peak boundary of its left neighbor
    BackToBackExponential_sptr leftneighbor = m_vecPeakFunctions[ileftneighbor].second.second;
    double leftneighborrightbound = leftneighbor->centre() + PEAKBOUNDARYFACTOR * leftneighbor->fwhm();
    if (leftneighborrightbound > peakleftboundary)
      peakleftboundary = leftneighborrightbound;
  }

  // 2. Determine its right boundary
  peakrightboundary = rightpeak->centre() + PEAKFITRANGEFACTOR * rightpeak->fwhm();

  size_t irightneighbor = irightpeak + 1;
  if (irightneighbor < m_vecPeakFunctions.size()) {
    // a) right peak exists
    BackToBackExponential_sptr rightneighbor = m_vecPeakFunctions[irightneighbor].second.second;
    double rightneighborleftbound = rightneighbor->centre() - PEAKBOUNDARYFACTOR * rightneighbor->fwhm();
    if (rightneighborleftbound < peakrightboundary)
      peakrightboundary = rightneighborleftbound;
  }
}

//=======================  Fit 1 Set of Overlapped Peaks ======================

//----------------------------------------------------------------------------
/** Fit peak with flexiblity in multiple steps
 * Prerequisit:
 * 1. Peak parameters are set up to the peak function
 * 2. Background is removed
 *
 * @param dataws :: The data workspace
 * @param peakfunction :: An instance of the BackToBackExponential peak
 *function
 * @param guessedfwhm: Guessed fwhm in order to constain the peak.  If
 *negative, then no constraint
 */
std::pair<bool, double> FitPowderDiffPeaks::doFitPeak(const Workspace2D_sptr &dataws,
                                                      const BackToBackExponential_sptr &peakfunction,
                                                      double guessedfwhm) {
  // 1. Set up boundary
  if (guessedfwhm > 0) {
    double tof_h = peakfunction->centre();
    double centerleftend = tof_h - guessedfwhm * 3.0;
    double centerrightend = tof_h + guessedfwhm * 3.0;
    auto centerbound =
        std::make_unique<BoundaryConstraint>(peakfunction.get(), "X0", centerleftend, centerrightend, false);
    peakfunction->addConstraint(std::move(centerbound));

    g_log.debug() << "[DoFitPeak] Peak Center Boundary = " << centerleftend << ", " << centerrightend << '\n';
  }

  // A > 0, B > 0, S > 0
  auto abound = std::make_unique<BoundaryConstraint>(peakfunction.get(), "A", 0.0000001, DBL_MAX, false);
  peakfunction->addConstraint(std::move(abound));

  auto bbound = std::make_unique<BoundaryConstraint>(peakfunction.get(), "B", 0.0000001, DBL_MAX, false);
  peakfunction->addConstraint(std::move(bbound));

  auto sbound = std::make_unique<BoundaryConstraint>(peakfunction.get(), "S", 0.0001, DBL_MAX, false);
  peakfunction->addConstraint(std::move(sbound));

  // 2. Unfix all parameters
  vector<string> paramnames = peakfunction->getParameterNames();
  size_t numparams = paramnames.size();
  for (size_t i = 0; i < numparams; ++i)
    peakfunction->unfix(i);

  // 3. Set up the fitting scheme
  vector<vector<string>> vecMinimizers;
  vector<vector<size_t>> vecMaxIterations;
  vector<vector<double>> vecDampings;

  // a) Scheme 1: Levenberg and Levenger
  /*
  vector<string> minimizers(2);
  minimizers[0] = "Levenberg-MarquardtMD";
  minimizers[1] = "Levenberg-Marquardt";
  vector<size_t> maxiterations(2, 1000);
  vector<double> dampings(2, 0.0);
  vecMinimizers.emplace_back(minimizers);
  vecMaxIterations.emplace_back(maxiterations);
  vecDampings.emplace_back(dampings);
  */

  vector<string> minimizers2(3);
  minimizers2[0] = "Simplex";
  minimizers2[1] = "Levenberg-MarquardtMD";
  minimizers2[2] = "Levenberg-Marquardt";
  vector<size_t> maxiterations2(3, 1000);
  maxiterations2[0] = 10000;
  vector<double> dampings2(3, 0.0);
  vecMinimizers.emplace_back(minimizers2);
  vecMaxIterations.emplace_back(maxiterations2);
  vecDampings.emplace_back(dampings2);

  // 4. Fit in different sequential
  bool goodfit = false;
  size_t numschemes = vecMinimizers.size();

  map<string, double> origparams, bestparams;
  storeFunctionParameters(peakfunction, origparams);
  bestparams = origparams;
  double bestchi2 = DBL_MAX;

  for (size_t is = 0; is < numschemes; ++is) {
    // a) Restore the starting value
    restoreFunctionParameters(peakfunction, origparams);

    // b) Fit in multiple steps
    double thischi2;
    bool localgoodfit = doFit1PeakSequential(dataws, 1, peakfunction, vecMinimizers[is], vecMaxIterations[is],
                                             vecDampings[is], thischi2);

    // c) Book keep
    if (localgoodfit && !goodfit) {
      // First local good fit
      bestchi2 = thischi2;
      storeFunctionParameters(peakfunction, bestparams);
      goodfit = true;
    } else if (localgoodfit && goodfit) {
      // Not the first time to have a good fit
      if (thischi2 < bestchi2) {
        bestchi2 = thischi2;
        storeFunctionParameters(peakfunction, bestparams);
      }
    }
  } // ENDFOR

  pair<bool, double> returnvalue = make_pair(goodfit, bestchi2);

  return returnvalue;
}

//----------------------------------------------------------------------------
/** Store the function's parameter values to a map
 */
void FitPowderDiffPeaks::storeFunctionParameters(const IFunction_sptr &function, std::map<string, double> &parammaps) {
  vector<string> paramnames = function->getParameterNames();
  parammaps.clear();
  for (auto &paramname : paramnames)
    parammaps.emplace(paramname, function->getParameter(paramname));
}

//----------------------------------------------------------------------------
/** Restore the function's parameter values from a map
 */
void FitPowderDiffPeaks::restoreFunctionParameters(const IFunction_sptr &function, map<string, double> parammap) {
  vector<string> paramnames = function->getParameterNames();
  for (auto &parname : paramnames) {
    auto miter = parammap.find(parname);
    if (miter != parammap.end())
      function->setParameter(parname, miter->second);
  }
}

//----------------------------------------------------------------------------
/** Fit 1 peak by 1 minimizer of 1 call of minimzer (simple version)
 * @param dataws :: A data aworkspace
 * @param workspaceindex :: A histogram index
 * @param peakfunction :: An instance of the BackToBackExponential function
 * @param minimzername :: A name of the minimizer to use
 * @param maxiteration :: A maximum number of iterations
 * @param chi2 :: The chi squared value (output)
 *
 * Return
 * 1. fit success?
 * 2. chi2
 */
bool FitPowderDiffPeaks::doFit1PeakSimple(const Workspace2D_sptr &dataws, size_t workspaceindex,
                                          const BackToBackExponential_sptr &peakfunction, const string &minimzername,
                                          size_t maxiteration, double &chi2) {
  stringstream dbss;
  dbss << peakfunction->asString() << '\n';
  dbss << "Starting Value: ";
  vector<string> paramNames = peakfunction->getParameterNames();
  for (auto &paramName : paramNames)
    dbss << paramName << "= " << peakfunction->getParameter(paramName) << ", \t";
  for (size_t i = 0; i < dataws->x(workspaceindex).size(); ++i)
    dbss << dataws->x(workspaceindex)[i] << "\t\t" << dataws->y(workspaceindex)[i] << "\t\t"
         << dataws->e(workspaceindex)[i] << '\n';
  g_log.debug() << "DBx430 " << dbss.str() << '\n';

  // 1. Peak height
  if (peakfunction->height() < 1.0E-5)
    peakfunction->setHeight(4.0);

  // 2. Create fit
  Algorithm_sptr fitalg = createChildAlgorithm("Fit", -1, -1, true);
  fitalg->initialize();

  // 3. Set
  fitalg->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(peakfunction));
  fitalg->setProperty("InputWorkspace", dataws);
  fitalg->setProperty("WorkspaceIndex", static_cast<int>(workspaceindex));
  fitalg->setProperty("Minimizer", minimzername);
  fitalg->setProperty("CostFunction", "Least squares");
  fitalg->setProperty("MaxIterations", static_cast<int>(maxiteration));
  fitalg->setProperty("Output", "FitPeak");

  // 3. Execute and parse the result
  bool isexecute = fitalg->execute();
  bool fitsuccess = false;
  chi2 = DBL_MAX;

  if (isexecute) {
    std::string fitresult = parseFitResult(fitalg, chi2, fitsuccess);

    // Figure out result
    g_log.information() << "[DBx138A] Fit Peak @ " << peakfunction->centre() << " Result:" << fitsuccess << "\n"
                        << "Detailed info = " << fitresult << ", Chi^2 = " << chi2 << '\n';

    // Debug information output
    API::ITableWorkspace_sptr paramws = fitalg->getProperty("OutputParameters");
    std::string infofit = parseFitParameterWorkspace(paramws);
    g_log.information() << "Fitted Parameters: \n" << infofit << '\n';
  } else {
    g_log.error() << "[DBx128B] Failed to execute fitting peak @ " << peakfunction->centre() << '\n';
  }

  return fitsuccess;
}

//----------------------------------------------------------------------------------------------
/** Fit 1 peak by using a sequential of minimizer
 * @param dataws :: A data aworkspace
 * @param workspaceindex :: A histogram index
 * @param peakfunction :: An instance of the BackToBackExponential function
 * @param minimzernames :: A vector of the minimizer names
 * @param maxiterations :: A vector if maximum numbers of iterations
 * @param dampfactors :: A vector of damping factors
 * @param chi2 :: The chi squared value (output)
 */
bool FitPowderDiffPeaks::doFit1PeakSequential(const Workspace2D_sptr &dataws, size_t workspaceindex,
                                              const BackToBackExponential_sptr &peakfunction,
                                              vector<string> minimzernames, vector<size_t> maxiterations,
                                              const vector<double> &dampfactors, double &chi2) {
  // 1. Check
  if (minimzernames.size() != maxiterations.size() && minimzernames.size() != dampfactors.size()) {
    throw runtime_error("doFit1PeakSequential should have the input vectors of same size.");
  }

  // 2.  Start Chi2
  map<string, double> parambeforefit;
  storeFunctionParameters(peakfunction, parambeforefit);

  double startchi2;
  doFit1PeakSimple(dataws, workspaceindex, peakfunction, "Levenberg-MarquardtMD", 0, startchi2);

  restoreFunctionParameters(peakfunction, parambeforefit);

  double currchi2 = startchi2;
  bool goodfit = false;

  // 3. Fit sequentially
  for (size_t i = 0; i < minimzernames.size(); ++i) {
    string minimizer = minimzernames[i];
    size_t maxiteration = maxiterations[i];
    g_log.notice() << "DBx315 Start Chi2 = " << startchi2 << ", Minimizer = " << minimizer
                   << ", Max Iterations = " << maxiteration << ", Workspace Index = " << workspaceindex
                   << ", Data Range = " << dataws->x(workspaceindex)[0] << ", " << dataws->x(workspaceindex).back()
                   << '\n';

    storeFunctionParameters(peakfunction, parambeforefit);

    double newchi2;
    bool localgoodfit = doFit1PeakSimple(dataws, workspaceindex, peakfunction, minimizer, maxiteration, newchi2);

    if (localgoodfit && newchi2 < currchi2) {
      // A better solution
      currchi2 = newchi2;
      goodfit = true;
    } else {
      // A same of worse one
      restoreFunctionParameters(peakfunction, parambeforefit);
      g_log.information() << "DBx315C  Fit Failed.  Fit Good = " << localgoodfit << '\n';
    }
  }

  // 4. Return
  chi2 = currchi2;

  return goodfit;
}

//----------------------------------------------------------------------------
/** Fit background-removed peak by Gaussian
 */
bool FitPowderDiffPeaks::doFitGaussianPeak(const DataObjects::Workspace2D_sptr &dataws, size_t workspaceindex,
                                           double in_center, double leftfwhm, double rightfwhm, double &center,
                                           double &sigma, double &height) {
  // 1. Estimate
  const auto &X = dataws->x(workspaceindex);
  const auto &Y = dataws->y(workspaceindex);

  height = 0;
  for (size_t i = 1; i < X.size(); ++i) {
    height += (X[i] - X[i - 1]) * Y[i];
  }
  sigma = (leftfwhm + rightfwhm) * 0.5;

  // 2. Use factory to generate Gaussian
  auto temppeak = API::FunctionFactory::Instance().createFunction("Gaussian");
  auto gaussianpeak = std::dynamic_pointer_cast<API::IPeakFunction>(temppeak);
  gaussianpeak->setHeight(height);
  gaussianpeak->setCentre(in_center);
  gaussianpeak->setFwhm(sigma);

  // b) Constraint
  double centerleftend = in_center - leftfwhm * 0.5;
  double centerrightend = in_center + rightfwhm * 0.5;
  auto centerbound =
      std::make_unique<BoundaryConstraint>(gaussianpeak.get(), "PeakCentre", centerleftend, centerrightend, false);
  gaussianpeak->addConstraint(std::move(centerbound));

  // 3. Fit
  auto fitalg = createChildAlgorithm("Fit", -1, -1, true);
  fitalg->initialize();

  fitalg->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(gaussianpeak));
  fitalg->setProperty("InputWorkspace", dataws);
  fitalg->setProperty("WorkspaceIndex", 1);
  fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
  fitalg->setProperty("CostFunction", "Least squares");
  fitalg->setProperty("MaxIterations", 1000);
  fitalg->setProperty("Output", "FitGaussianPeak");

  // iv)  Result
  bool successfulfit = fitalg->execute();
  if (!fitalg->isExecuted() || !successfulfit) {
    // Early return due to bad fit
    g_log.warning() << "Fitting Gaussian peak for peak around " << gaussianpeak->centre() << '\n';
    return false;
  }

  double chi2;
  bool fitsuccess;
  std::string fitresult = parseFitResult(fitalg, chi2, fitsuccess);
  g_log.information() << "[Fit Gaussian Peak] Successful = " << fitsuccess << ", Result:\n" << fitresult << '\n';

  // 4. Get result
  center = gaussianpeak->centre();
  height = gaussianpeak->height();
  double fwhm = gaussianpeak->fwhm();
  if (fwhm <= 0.0) {
    return false;
  }
  sigma = fwhm / 2.35;

  // 5. Debug output
  API::ITableWorkspace_sptr paramws = fitalg->getProperty("OutputParameters");
  std::string infofit = parseFitParameterWorkspace(paramws);
  g_log.information() << "[DBx133] Fitted Gaussian Parameters: \n" << infofit << '\n';

  return true;
}

//----------------------------------------------------------------------------------------------
/** Fit peaks with confidence in fwhm and etc.
 * @param peaks :: A vector of instances of BackToBackExponential
 * @param backgroundfunction :: An instance of BackgroundFunction
 * @param gfwhm : guessed fwhm.  If negative, then use the input value
 */
bool FitPowderDiffPeaks::fitOverlappedPeaks(vector<BackToBackExponential_sptr> peaks,
                                            const BackgroundFunction_sptr &backgroundfunction, double gfwhm) {
  // 1. Sort peak if necessary
  vector<pair<double, BackToBackExponential_sptr>> tofpeakpairs(peaks.size());
  for (size_t i = 0; i < peaks.size(); ++i)
    tofpeakpairs[i] = make_pair(peaks[i]->centre(), peaks[i]);
  sort(tofpeakpairs.begin(), tofpeakpairs.end());

  // 2. Determine range of the data
  BackToBackExponential_sptr leftpeak = tofpeakpairs[0].second;
  BackToBackExponential_sptr rightpeak = tofpeakpairs.back().second;
  double peaksleftboundary, peaksrightboundary;
  if (gfwhm <= 0) {
    // Use peaks' preset value
    peaksleftboundary = leftpeak->centre() - 4 * leftpeak->fwhm();
    peaksrightboundary = rightpeak->centre() + 4 * rightpeak->fwhm();
  } else {
    // Use user input's guess fwhm
    peaksleftboundary = leftpeak->centre() - 4 * gfwhm;
    peaksrightboundary = rightpeak->centre() + 4 * gfwhm;
  }

  // 3. Build partial data workspace
  Workspace2D_sptr peaksws = buildPartialWorkspace(m_dataWS, m_wsIndex, peaksleftboundary, peaksrightboundary);

  // 4. Remove background
  estimateBackgroundCoarse(peaksws, backgroundfunction, 0, 2, 1);

  // [DB] Debug output
  stringstream peakInfo;
  peakInfo << peaks.size() << "-Peaks Group Information: \n";
  for (size_t ipk = 0; ipk < tofpeakpairs.size(); ++ipk) {
    BackToBackExponential_sptr tmppeak = tofpeakpairs[ipk].second;
    peakInfo << "Peak " << ipk << "  @ TOF = " << tmppeak->centre() << ", A = " << tmppeak->getParameter("A")
             << ", B = " << tmppeak->getParameter("B") << ", S = " << tmppeak->getParameter("S")
             << ", FWHM = " << tmppeak->fwhm() << '\n';
  }
  g_log.information() << "[DB1034] " << peakInfo.str();

  stringstream peakWSData;
  peakWSData << "Partial workspace for peaks: \n";
  for (size_t i = 0; i < peaksws->x(0).size(); ++i)
    peakWSData << peaksws->x(1)[i] << "\t\t" << peaksws->y(1)[i] << "\t\t" << peaksws->e(1)[i] << "\t\t"
               << peaksws->y(0)[i] << '\n';
  g_log.information() << "[DB1042] " << peakWSData.str();

  // 5. Estimate peak height according to pre-set peak value
  estimatePeakHeightsLeBail(peaksws, 1, peaks);

  // 6. Set boundaries
  setOverlappedPeaksConstraints(peaks);

  // 7. Set up the composite function
  CompositeFunction_sptr peaksfunction(new CompositeFunction());
  for (auto &peak : peaks)
    peaksfunction->addFunction(peak);

  // 8. Fit multiple peaks
  vector<double> chi2s;
  vector<bool> fitgoods;
  bool fitsuccess = doFitMultiplePeaks(peaksws, 1, peaksfunction, peaks, fitgoods, chi2s);

  // 9. Plot peaks
  if (fitsuccess) {
    FunctionDomain1DVector domain(peaksws->x(1).rawData());
    plotFunction(peaksfunction, backgroundfunction, domain);
  }

  return fitsuccess;
}

//----------------------------------------------------------------------------------------------
/** Fit multiple (overlapped) peaks
 */
bool FitPowderDiffPeaks::doFitMultiplePeaks(const Workspace2D_sptr &dataws, size_t wsindex,
                                            const CompositeFunction_sptr &peaksfunc,
                                            vector<BackToBackExponential_sptr> peakfuncs, vector<bool> &vecfitgood,
                                            vector<double> &vecchi2s) {
  // 0. Pre-debug output
  stringstream dbss;
  dbss << "[DBx529] Composite Function: " << peaksfunc->asString();
  g_log.notice(dbss.str());

  // 1. Fit peaks intensities first
  const size_t numpeaks = peakfuncs.size();
  map<string, double> peaksfuncparams;

  // a) Set up fit/fix
  vector<string> peakparnames = peakfuncs[0]->getParameterNames();
  for (size_t ipn = 0; ipn < peakparnames.size(); ++ipn) {
    bool isI = peakparnames[ipn] == "I";

    for (size_t ipk = 0; ipk < numpeaks; ++ipk) {
      BackToBackExponential_sptr thispeak = peakfuncs[ipk];
      if (isI)
        thispeak->unfix(ipn);
      else
        thispeak->fix(ipn);
    }
  }

  stringstream dbss1;
  dbss1 << "[DBx529A] Composite Function: " << peaksfunc->asString();
  g_log.notice(dbss1.str());

  // b) Fit
  double chi2;
  bool fitgood = doFitNPeaksSimple(dataws, wsindex, peaksfunc, peakfuncs, "Levenberg-MarquardtMD", 1000, chi2);
  bool evergood = fitgood;

  // c) Process result; possibly early return
  if (!fitgood) {
    vecfitgood.resize(numpeaks, false);
    vecchi2s.resize(numpeaks, -1.0);
    return false;
  } else {
    vecfitgood.resize(numpeaks, true);
    vecchi2s.resize(numpeaks, chi2);
  }

  // 2. Fit A/B/S peak by peak
  for (size_t ipkfit = 0; ipkfit < numpeaks; ++ipkfit) {
    // a) Fix / unfix parameters
    for (size_t ipk = 0; ipk < numpeaks; ++ipk) {
      BackToBackExponential_sptr thispeak = peakfuncs[ipk];
      for (size_t iparam = 0; iparam < peakparnames.size(); ++iparam) {
        if (ipk == ipkfit) {
          // Peak to have parameters fit
          thispeak->unfix(iparam);
        } else {
          // Not the peak to fit, fix all
          thispeak->fix(iparam);
        }
      }
    }

    // b) Fit
    storeFunctionParameters(peaksfunc, peaksfuncparams);

    fitgood = doFitNPeaksSimple(dataws, wsindex, peaksfunc, peakfuncs, "Levenberg-MarquardtMD", 1000, chi2);

    // not required. before loop starts, evergood=fitgood WITH fitgood==true
    // evergood = evergood || fitgood;

    // c) Process the result
    if (!fitgood)
      restoreFunctionParameters(peaksfunc, peaksfuncparams);
  } // END OF FIT ALL SINGLE PEAKS

  // 3. Fit all parameters (dangerous)
  for (size_t ipk = 0; ipk < numpeaks; ++ipk) {
    BackToBackExponential_sptr thispeak = peakfuncs[ipk];
    for (size_t iparam = 0; iparam < peakparnames.size(); ++iparam)
      thispeak->unfix(iparam);
  }

  storeFunctionParameters(peaksfunc, peaksfuncparams);
  fitgood = doFitNPeaksSimple(dataws, wsindex, peaksfunc, peakfuncs, "Levenberg-MarquardtMD", 1000, chi2);
  // not required. before, evergood=fitgood WITH fitgood==true
  // evergood = evergood || fitgood;

  if (!fitgood)
    restoreFunctionParameters(peaksfunc, peaksfuncparams);

  // -1. Final debug output
  FunctionDomain1DVector domain(dataws->x(wsindex).rawData());
  FunctionValues values(domain);
  peaksfunc->function(domain, values);
  stringstream rss;
  for (size_t i = 0; i < domain.size(); ++i)
    rss << domain[i] << "\t\t" << values[i] << '\n';
  g_log.information() << "[T] Multiple peak fitting pattern:\n" << rss.str();

  return evergood;
}

//----------------------------------------------------------------------------------------------
/** Use Le Bail method to estimate and set the peak heights
 * @param dataws:  workspace containing the background-removed data
 * @param wsindex: workspace index of the data without background
 * @param peaks :: A vector of instances of BackToBackExponential function
 */
void FitPowderDiffPeaks::estimatePeakHeightsLeBail(const Workspace2D_sptr &dataws, size_t wsindex,
                                                   vector<BackToBackExponential_sptr> peaks) {
  // 1. Build data structures
  FunctionDomain1DVector domain(dataws->x(wsindex).rawData());
  FunctionValues values(domain);
  vector<vector<double>> peakvalues;
  for (size_t i = 0; i < (peaks.size() + 1); ++i) {
    vector<double> peakvalue(domain.size(), 0.0);
    peakvalues.emplace_back(peakvalue);
  }

  // 2. Calcualte peak values
  size_t isum = peaks.size();
  for (size_t ipk = 0; ipk < peaks.size(); ++ipk) {
    BackToBackExponential_sptr thispeak = peaks[ipk];
    thispeak->setHeight(1.0);
    thispeak->function(domain, values);
    for (size_t j = 0; j < domain.size(); ++j) {
      peakvalues[ipk][j] = values[j];
      peakvalues[isum][j] += values[j];
    }
  }

  // 3. Calculate peak height
  const auto &vecY = dataws->y(wsindex);
  for (size_t ipk = 0; ipk < peaks.size(); ++ipk) {
    double height = 0.0;
    for (size_t j = 0; j < domain.size() - 1; ++j) {
      if (vecY[j] > 0.0 && peakvalues[isum][j] > 1.0E-5) {
        double dtof = domain[j + 1] - domain[j];
        double temp = vecY[j] * peakvalues[ipk][j] / peakvalues[isum][j] * dtof;
        height += temp;
      }
    }

    BackToBackExponential_sptr thispeak = peaks[ipk];
    thispeak->setHeight(height);

    g_log.information() << "[DBx256] Peak @ " << thispeak->centre()
                        << "  Set Guessed Height (LeBail) = " << thispeak->height() << '\n';
  }
}

//----------------------------------------------------------------------------------------------
/** Set constraints on a group of overlapped peaks for fitting
 */
void FitPowderDiffPeaks::setOverlappedPeaksConstraints(const vector<BackToBackExponential_sptr> &peaks) {
  for (const auto &thispeak : peaks) {
    // 1. Set constraint on X.
    double fwhm = thispeak->fwhm();
    double centre = thispeak->centre();
    double leftcentrebound = centre - 0.5 * fwhm;
    double rightcentrebound = centre + 0.5 * fwhm;

    auto bc = std::make_unique<BoundaryConstraint>(thispeak.get(), "X0", leftcentrebound, rightcentrebound, false);
    thispeak->addConstraint(std::move(bc));
  }
}

//----------------------------------------------------------------------------------------------
/** Fit N overlapped peaks in a simple manner
 */
bool FitPowderDiffPeaks::doFitNPeaksSimple(const Workspace2D_sptr &dataws, size_t wsindex,
                                           const CompositeFunction_sptr &peaksfunc,
                                           const vector<BackToBackExponential_sptr> &peakfuncs,
                                           const string &minimizername, size_t maxiteration, double &chi2) {
  // 1. Debug output
  stringstream dbss0;
  dbss0 << "Starting Value: ";
  vector<string> paramNames = peaksfunc->getParameterNames();
  for (auto &paramName : paramNames)
    dbss0 << paramName << "= " << peaksfunc->getParameter(paramName) << ", \t";
  g_log.information() << "DBx430 " << dbss0.str() << '\n';

  // 2. Create fit
  Algorithm_sptr fitalg = createChildAlgorithm("Fit", -1, -1, true);
  fitalg->initialize();

  // 3. Set
  fitalg->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(peaksfunc));
  fitalg->setProperty("InputWorkspace", dataws);
  fitalg->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fitalg->setProperty("Minimizer", minimizername);
  fitalg->setProperty("CostFunction", "Least squares");
  fitalg->setProperty("MaxIterations", static_cast<int>(maxiteration));
  fitalg->setProperty("Output", "FitPeak");

  // 3. Execute and parse the result
  bool isexecute = fitalg->execute();
  bool fitsuccess = false;
  chi2 = DBL_MAX;

  // 4. Output
  stringstream dbss;
  dbss << "Fit N-Peaks @ ";
  for (auto &peakfunc : peakfuncs)
    dbss << peakfunc->centre() << ", ";

  if (isexecute) {
    // Figure out result
    std::string fitresult = parseFitResult(fitalg, chi2, fitsuccess);

    dbss << " Result:" << fitsuccess << "\n"
         << "Detailed info = " << fitresult;

    g_log.information() << "[DBx149A] " << dbss.str() << '\n';

    // Debug information output
    API::ITableWorkspace_sptr paramws = fitalg->getProperty("OutputParameters");
    std::string infofit = parseFitParameterWorkspace(paramws);
    g_log.information() << "[DBx149B] Fitted Parameters: \n" << infofit << '\n';
  } else {
    dbss << ": Failed ";
    g_log.error() << "[DBx149C] " << dbss.str() << '\n';
  }

  return fitsuccess;
}

//===================================  Process Fit Result
//=====================================
//----------------------------------------------------------------------------------------------
/** Parse fit result
 */
std::string FitPowderDiffPeaks::parseFitResult(const API::IAlgorithm_sptr &fitalg, double &chi2, bool &fitsuccess) {
  stringstream rss;

  chi2 = fitalg->getProperty("OutputChi2overDoF");
  string fitstatus = fitalg->getProperty("OutputStatus");

  fitsuccess = (fitstatus == "success");

  rss << "  [Algorithm Fit]:  Chi^2 = " << chi2 << "; Fit Status = " << fitstatus;

  return rss.str();
}

//----------------------------------------------------------------------------------------------
/** Parse parameter workspace returned from Fit()
 */
std::string FitPowderDiffPeaks::parseFitParameterWorkspace(const API::ITableWorkspace_sptr &paramws) {
  // 1. Check
  if (!paramws) {
    g_log.warning() << "Input table workspace is NULL.  \n";
    return "";
  }

  // 2. Parse
  std::stringstream msgss;
  size_t numrows = paramws->rowCount();
  for (size_t i = 0; i < numrows; ++i) {
    API::TableRow row = paramws->getRow(i);
    std::string parname;
    double parvalue, parerror;
    row >> parname >> parvalue >> parerror;

    msgss << parname << " = " << setw(10) << setprecision(5) << parvalue << " +/- " << setw(10) << setprecision(5)
          << parerror << '\n';
  }

  return msgss.str();
}

//================================  Process Input/Output
//======================================
//----------------------------------------------------------------------------------------------
/** Import TableWorkspace containing the parameters for fitting
 * the diffrotometer geometry parameters
 */
void FitPowderDiffPeaks::importInstrumentParameterFromTable(const DataObjects::TableWorkspace_sptr &parameterWS) {
  // 1. Check column orders
  std::vector<std::string> colnames = parameterWS->getColumnNames();
  if (colnames.size() < 2) {
    stringstream errss;
    errss << "Input parameter table workspace does not have enough number of "
             "columns. "
          << " Number of columns = " << colnames.size() << " >= 2 as required. ";
    g_log.error(errss.str());
    throw std::runtime_error(errss.str());
  }

  if (colnames[0] != "Name" || colnames[1] != "Value") {
    stringstream errss;
    errss << "Input parameter table workspace does not have the columns in "
             "order as  "
          << "Name, Value and etc. ";
    g_log.error(errss.str());
    throw std::runtime_error(errss.str());
  }

  size_t numrows = parameterWS->rowCount();

  g_log.notice() << "[DBx409] Import TableWorkspace " << parameterWS->getName() << " containing " << numrows
                 << " instrument profile parameters\n";

  // 2. Import data to maps
  std::string parname;
  double value;
  m_instrumentParmaeters.clear();

  for (size_t ir = 0; ir < numrows; ++ir) {
    API::TableRow trow = parameterWS->getRow(ir);
    trow >> parname >> value;
    m_instrumentParmaeters.emplace(parname, value);
    g_log.notice() << "[DBx211] Import parameter " << parname << ": " << value << '\n';
  }
}

/** Import Bragg peak table workspace
 */
void FitPowderDiffPeaks::parseBraggPeakTable(const TableWorkspace_sptr &peakws, vector<map<string, double>> &parammaps,
                                             vector<map<string, int>> &hklmaps) {
  // 1. Get columns' types and names
  vector<string> paramnames = peakws->getColumnNames();
  size_t numcols = paramnames.size();
  vector<string> coltypes(numcols);
  for (size_t i = 0; i < numcols; ++i) {
    Column_sptr col = peakws->getColumn(i);
    string coltype = col->type();
    coltypes[i] = coltype;
  }

  // 2. Parse table rows
  size_t numrows = peakws->rowCount();
  for (size_t irow = 0; irow < numrows; ++irow) {
    // 1. Create map
    map<string, int> intmap;
    map<string, double> doublemap;

    // 2. Parse
    for (size_t icol = 0; icol < numcols; ++icol) {
      string coltype = coltypes[icol];
      string colname = paramnames[icol];

      if (coltype == "int") {
        // Integer
        int temp = peakws->cell<int>(irow, icol);
        intmap.emplace(colname, temp);
      } else if (coltype == "double") {
        // Double
        double temp = peakws->cell<double>(irow, icol);
        doublemap.emplace(colname, temp);
      }

    } // ENDFOR Column

    parammaps.emplace_back(doublemap);
    hklmaps.emplace_back(intmap);

  } // ENDFOR Row

  g_log.information() << "Import " << hklmaps.size() << " entries from Bragg peak TableWorkspace " << peakws->getName()
                      << '\n';
}

//----------------------------------------------------------------------------
/** Create a Workspace2D for fitted peaks (pattern) and also the workspace for
 * Zscores!
 */
Workspace2D_sptr FitPowderDiffPeaks::genOutputFittedPatternWorkspace(std::vector<double> pattern, int workspaceindex) {
  // 1. Init
  const auto &X = m_dataWS->x(workspaceindex);

  if (pattern.size() != X.size()) {
    stringstream errmsg;
    errmsg << "Input pattern (" << pattern.size() << ") and algorithm's input workspace (" << X.size()
           << ") have different size. ";
    g_log.error() << errmsg.str() << '\n';
    throw std::logic_error(errmsg.str());
  }

  // 2. Create data workspace
  Workspace2D_sptr dataws = std::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", 5, pattern.size(), pattern.size()));

  // 3. Set up
  for (size_t iw = 0; iw < 5; ++iw) {
    dataws->setSharedX(iw, m_dataWS->sharedX(workspaceindex));
  }

  dataws->setSharedY(0, m_dataWS->sharedY(workspaceindex));
  dataws->mutableY(1) = pattern;
  dataws->mutableY(2) = m_dataWS->y(workspaceindex) - pattern;

  return dataws;
}

//----------------------------------------------------------------------------
/** Create data workspace for X0, A, B and S of peak with good fit
 */
Workspace2D_sptr FitPowderDiffPeaks::genPeakParameterDataWorkspace() {
  // 1. Check and prepare
  if (m_vecPeakFunctions.size() != m_peakFitChi2.size()) {
    throw runtime_error("Wrong definition of m_peakFitChi2");
  }

  size_t numpeaks = m_vecPeakFunctions.size();

  // 2. Collect parameters of peak fitted good
  vector<double> vecdh, vectofh, vecalpha, vecbeta, vecsigma, vecchi2;
  for (size_t i = 0; i < numpeaks; ++i) {
    double &chi2 = m_peakFitChi2[i];
    if (chi2 > 0) {
      // a) Get values
      double dh = m_vecPeakFunctions[i].first;
      BackToBackExponential_sptr peak = m_vecPeakFunctions[i].second.second;

      double p_a = peak->getParameter("A");
      double p_b = peak->getParameter("B");
      double p_x = peak->getParameter("X0");
      double p_s = peak->getParameter("S");

      // b) To vectors
      vecchi2.emplace_back(chi2);
      vecdh.emplace_back(dh);
      vectofh.emplace_back(p_x);
      vecalpha.emplace_back(p_a);
      vecbeta.emplace_back(p_b);
      vecsigma.emplace_back(p_s);
    }
  } // ENDFOR i(peak)

  // 3. Create workspace2D
  size_t numgoodpeaks = vecdh.size();
  Workspace2D_sptr paramws = std::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", 4, numgoodpeaks, numgoodpeaks));
  for (size_t j = 0; j < 4; ++j) {
    paramws->mutableX(j) = vecdh;
    paramws->mutableE(j) = vecchi2;
  }
  paramws->mutableY(0) = vectofh;
  paramws->mutableY(1) = vecalpha;
  paramws->mutableY(2) = vecbeta;
  paramws->mutableY(3) = vecsigma;

  // 4. Set Axis label
  paramws->getAxis(0)->setUnit("dSpacing");

  auto tAxis = std::make_unique<TextAxis>(4);
  tAxis->setLabel(0, "X0");
  tAxis->setLabel(1, "A");
  tAxis->setLabel(2, "B");
  tAxis->setLabel(3, "S");

  paramws->replaceAxis(1, std::move(tAxis));

  return paramws;
}

//----------------------------------------------------------------------------
/** Generate a TableWorkspace for peaks with good fitting.
 * Table has column as H, K, L, d_h, X0, A(lpha), B(eta), S(igma), Chi2
 * Each row is a peak
 */
pair<TableWorkspace_sptr, TableWorkspace_sptr> FitPowderDiffPeaks::genPeakParametersWorkspace() {
  // 1. Debug/Test Output
  for (size_t i = 0; i < m_vecPeakFunctions.size(); ++i) {
    g_log.debug() << "Peak @ d = " << m_vecPeakFunctions[i].first << ":  Chi^2 = " << m_peakFitChi2[i] << '\n';
  }

  if (m_vecPeakFunctions.size() != m_peakFitChi2.size()) {
    throw runtime_error("Wrong definition of m_peakFitChi2");
  }

  size_t numpeaks = m_vecPeakFunctions.size();
  vector<double> vectofh(numpeaks), vecalpha(numpeaks), vecbeta(numpeaks), vecsigma(numpeaks);

  // 2. Generate the TableWorkspace for peak parameters
  auto tablewsptr = new TableWorkspace();
  TableWorkspace_sptr tablews = TableWorkspace_sptr(tablewsptr);

  tablews->addColumn("int", "H");
  tablews->addColumn("int", "K");
  tablews->addColumn("int", "L");

  tablews->addColumn("double", "d_h");
  tablews->addColumn("double", "TOF_h");
  tablews->addColumn("double", "Height");
  tablews->addColumn("double", "Alpha");
  tablews->addColumn("double", "Beta");
  tablews->addColumn("double", "Sigma");
  tablews->addColumn("double", "Chi2");

  /*
  stringstream outbuf;
  outbuf << setw(10) << "H" << setw(10) << "K" << setw(10) << "L"
         << setw(10) << "d_h" << setw(10) << "X0" << setw(10) << "I"
         << setw(10) << "A" << setw(10) << "B" << setw(10) << "S" << setw(10) <<
  "Chi2\n";
  */

  for (size_t i = 0; i < numpeaks; ++i) {
    double &chi2 = m_peakFitChi2[i];
    if (chi2 > 0) {
      // Bad fit peak has chi^2 < 0;
      double dh = m_vecPeakFunctions[i].first;
      vector<int> &hkl = m_vecPeakFunctions[i].second.first;
      BackToBackExponential_sptr peak = m_vecPeakFunctions[i].second.second;

      TableRow newrow = tablews->appendRow();

      // i. H, K, L, d_h
      newrow << hkl[0] << hkl[1] << hkl[2] << dh;

      // ii. A, B, I, S, X0
      double p_a = peak->getParameter("A");
      double p_b = peak->getParameter("B");
      double p_i = peak->getParameter("I");
      double p_x = peak->getParameter("X0");
      double p_s = peak->getParameter("S");
      newrow << p_x << p_i << p_a << p_b << p_s;

      // iii. Chi^2
      newrow << chi2;

      // iv.  Prepare for Z-score
      // vectofh.emplace_back(p_x);
      // vecalpha.emplace_back(p_a);
      // vecbeta.emplace_back(p_b);
      // vecsigma.emplace_back(p_s);
      vectofh[i] = p_x;
      vecalpha[i] = p_a;
      vecbeta[i] = p_b;
      vecsigma[i] = p_s;

      /*
      outbuf << setw(10) << setprecision(5) << chi2 << '\n';
      outbuf << setw(10) << hkl[0] << setw(10) << hkl[1] << setw(10) << hkl[2]
             << setw(10) << setprecision(5) << dh;
      outbuf << setw(10) << setprecision(5) << p_x
             << setw(10) << setprecision(5) << p_i
             << setw(10) << setprecision(5) << p_a
             << setw(10) << setprecision(5) << p_b
             << setw(10) << setprecision(5) << p_s;
      */
    }
  } // ENDFOR i(peak)

  // 3. Z-score table
  // i.   Calculate Z-scores
  vector<double> zcentres = Kernel::getZscore(vectofh);
  vector<double> zalphas = Kernel::getZscore(vecalpha);
  vector<double> zbetas = Kernel::getZscore(vecbeta);
  vector<double> zsigma = Kernel::getZscore(vecsigma);

  // ii.  Build table workspace for Z scores
  auto ztablewsptr = new TableWorkspace();
  TableWorkspace_sptr ztablews = TableWorkspace_sptr(ztablewsptr);

  ztablews->addColumn("int", "H");
  ztablews->addColumn("int", "K");
  ztablews->addColumn("int", "L");

  ztablews->addColumn("double", "d_h");
  ztablews->addColumn("double", "Z_TOF_h");
  ztablews->addColumn("double", "Z_Alpha");
  ztablews->addColumn("double", "Z_Beta");
  ztablews->addColumn("double", "Z_Sigma");

  // iii. Set values
  for (size_t i = 0; i < m_vecPeakFunctions.size(); ++i) {
    double chi2 = m_peakFitChi2[i];
    if (chi2 > 0) {
      // A good fit has chi^2 larger than 0
      double dh = m_vecPeakFunctions[i].first;
      vector<int> &hkl = m_vecPeakFunctions[i].second.first;

      TableRow newrow = ztablews->appendRow();
      newrow << hkl[0] << hkl[1] << hkl[2] << dh;

      // ii. Z scores
      double p_x = zcentres[i];
      double p_a = zalphas[i];
      double p_b = zbetas[i];
      double p_s = zsigma[i];

      newrow << p_x << p_a << p_b << p_s;
    }
  } // ENDFOR i(peak)

  return make_pair(tablews, ztablews);
}

//----------------------------------------------------------------------------------------------
/** Genearte peaks from input workspace;
 * Each peak within requirement will put into both (1) m_peaks and (2)
 * m_peaksmap
 */
void FitPowderDiffPeaks::genPeaksFromTable(const TableWorkspace_sptr &peakparamws) {
  // Check and clear input and output
  if (!peakparamws) {
    stringstream errss;
    errss << "Input tableworkspace for peak parameters is invalid!";
    g_log.error(errss.str());
    throw std::invalid_argument(errss.str());
  }

  m_vecPeakFunctions.clear();

  // Give name to peak parameters
  BackToBackExponential tempeak;
  tempeak.initialize();
  mPeakParameterNames = tempeak.getParameterNames();
  mPeakParameterNames.emplace_back("S2");

  // Parse TableWorkspace
  vector<map<std::string, double>> peakparametermaps;
  vector<map<std::string, int>> peakhkls;
  this->parseBraggPeakTable(peakparamws, peakparametermaps, peakhkls);

  // Create a map to convert the Bragg peak Table paramter name to Back to back
  // exponential+pseudo-voigt
  map<string, string> bk2bk2braggmap{{"A", "Alpha"},  {"B", "Beta"},  {"X0", "TOF_h"},
                                     {"I", "Height"}, {"S", "Sigma"}, {"S2", "Sigma2"}};

  // Generate Peaks
  size_t numrows = peakparamws->rowCount();
  for (size_t ir = 0; ir < numrows; ++ir) {
    double d_h;
    vector<int> hkl;
    bool good;
    BackToBackExponential_sptr newpeak = genPeak(peakhkls[ir], peakparametermaps[ir], bk2bk2braggmap, good, hkl, d_h);

    if (good) {
      m_vecPeakFunctions.emplace_back(d_h, make_pair(hkl, newpeak));
    }
  } // ENDFOR Each potential peak

  // Sort by peaks' centre in d-space
  sort(m_vecPeakFunctions.begin(), m_vecPeakFunctions.end());

  // Remove all peaks outside of tof_min and tof_max
  double tofmin = m_dataWS->x(m_wsIndex)[0];
  double tofmax = m_dataWS->x(m_wsIndex).back();

  stringstream dbss;
  dbss << "Specified range for peaks in TOF: " << tofmin << ", " << tofmax << "\n";

  vector<pair<double, pair<vector<int>, BackToBackExponential_sptr>>>::iterator deliter;
  for (deliter = m_vecPeakFunctions.begin(); deliter != m_vecPeakFunctions.end(); ++deliter) {
    double d_h = deliter->first;
    vector<int> &hkl = deliter->second.first;
    BackToBackExponential_sptr peak = deliter->second.second;
    double tofh = peak->getParameter("X0");

    dbss << "\tPeak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << ") @ d = " << d_h
         << " (calculated TOF) = " << tofh << " ";

    if (tofh < tofmin || tofh > tofmax) {
      // Remove peak from vector
      deliter = m_vecPeakFunctions.erase(deliter);
      dbss << ": Delete due to out of range. \n";
      if (deliter == m_vecPeakFunctions.end()) {
        break;
      }
    } else {
      dbss << ": Kept. \n";
    }
  }

  g_log.notice() << "[DBx453] " << dbss.str() << "\n";

  // Remove peaks lower than minimum
  if (m_minimumHKL.size() == 3) {
    // Only keep peaks from and above minimum HKL
    for (deliter = m_vecPeakFunctions.begin(); deliter != m_vecPeakFunctions.end(); ++deliter) {
      vector<int> hkl = deliter->second.first;
      if (hkl == m_minimumHKL) {
        break;
      }
    }
    if (deliter != m_vecPeakFunctions.end()) {
      // Find the real minum
      int indminhkl = static_cast<int>(deliter - m_vecPeakFunctions.begin());
      int ind1stpeak = indminhkl - m_numPeaksLowerToMin;
      if (ind1stpeak > 0) {
        deliter = m_vecPeakFunctions.begin() + ind1stpeak;
        m_vecPeakFunctions.erase(m_vecPeakFunctions.begin(), deliter);
      }
    } else {
      // Miminum HKL peak does not exist
      vector<int> hkl = m_minimumHKL;
      g_log.warning() << "Minimum peak " << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << " does not exit. \n";
    }
  }

  // Keep some input information
  stringstream dbout;
  for (deliter = m_vecPeakFunctions.begin(); deliter != m_vecPeakFunctions.end(); ++deliter) {
    vector<int> hkl = deliter->second.first;
    double d_h = deliter->first;
    double tof_h = deliter->second.second->centre();
    dbout << "Peak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << ") @ d = " << d_h << ", TOF = " << tof_h << '\n';
  }
  g_log.information() << "[DBx531] Peaks To Fit:  Number of peaks = " << m_vecPeakFunctions.size() << "\n"
                      << dbout.str();
}

//-----------------------------------------------------------------------------------------
/** Generate a peak
 *
 * @param hklmap: a map containing one (HKL) entry
 * @param parammap :: a map of parameters
 * @param bk2bk2braggmap :: bk2bk2braggmap
 * @param good :: (output) peak is valid
 * @param hkl   : (output) (HKL) of the peak generated.
 * @param d_h :: (output) d_h
 * @return      : BackToBackExponential peak
 */
BackToBackExponential_sptr FitPowderDiffPeaks::genPeak(map<string, int> hklmap, map<string, double> parammap,
                                                       map<string, string> bk2bk2braggmap, bool &good, vector<int> &hkl,
                                                       double &d_h) {
  // Generate a peak function
  BackToBackExponential_sptr newpeakptr = std::make_shared<BackToBackExponential>();
  newpeakptr->initialize();

  // Check miller index (HKL) is a valid value in a miller indexes pool (hklmap)
  good = getHKLFromMap(std::move(hklmap), hkl);
  if (!good) {
    // Ignore and return
    return newpeakptr;
  }

  // Set the peak parameters either by calculating or from table workspace
  string peakcalmode;
  if (m_genPeakStartingValue == HKLCALCULATION) {
    // a) Use Bragg peak table's (HKL) and calculate the peak parameters
    double alph0 = getParameter("Alph0");
    double alph1 = getParameter("Alph1");
    double alph0t = getParameter("Alph0t");
    double alph1t = getParameter("Alph1t");
    double beta0 = getParameter("Beta0");
    double beta1 = getParameter("Beta1");
    double beta0t = getParameter("Beta0t");
    double beta1t = getParameter("Beta1t");
    double sig0 = getParameter("Sig0");
    double sig1 = getParameter("Sig1");
    double sig2 = getParameter("Sig2");
    double tcross = getParameter("Tcross");
    double width = getParameter("Width");
    double dtt1 = getParameter("Dtt1");
    double dtt1t = getParameter("Dtt1t");
    double dtt2t = getParameter("Dtt2t");
    double zero = getParameter("Zero");
    double zerot = getParameter("Zerot");

    // b) Check validity and make choice
    if (tcross == EMPTY_DBL() || width == EMPTY_DBL() || dtt1 == EMPTY_DBL() || dtt1t == EMPTY_DBL() ||
        dtt2t == EMPTY_DBL() || zero == EMPTY_DBL() || zerot == EMPTY_DBL()) {
      stringstream errss;
      errss << "In input InstrumentParameterTable, one of the following is not "
               "given.  Unable to process. \n";
      errss << "Tcross = " << tcross << "; Width = " << width << ", Dtt1 = " << dtt1 << ", Dtt1t = " << dtt1t << '\n';
      errss << "Dtt2t = " << dtt2t << ", Zero = " << zero << ", Zerot = " << zerot;

      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    bool caltofonly = false;
    if (alph0 == EMPTY_DBL() || alph1 == EMPTY_DBL() || alph0t == EMPTY_DBL() || alph1t == EMPTY_DBL() ||
        beta0 == EMPTY_DBL() || beta1 == EMPTY_DBL() || beta0t == EMPTY_DBL() || beta1t == EMPTY_DBL() ||
        sig0 == EMPTY_DBL() || sig1 == EMPTY_DBL() || sig2 == EMPTY_DBL()) {
      caltofonly = true;
      g_log.warning() << "[DBx343] At least one of the input instrument-peak "
                         "profile parameters is not given. "
                      << "Use (HKL) only!\n";
      g_log.warning() << "Alph0 = " << alph0 << ", Alph1 = " << alph1 << ", Alph0t = " << alph0t
                      << ", Alph1t = " << alph1t << '\n';
      g_log.warning() << "Beta0 = " << beta0 << ", Beta1 = " << beta1 << ", Beta0t = " << beta0t
                      << ", Beta1t = " << beta1t << '\n';
      g_log.warning() << "Sig0 = " << sig0 << ", Sig1 = " << sig1 << ", Sig2 = " << sig2 << '\n';
    }

    if (caltofonly) {
      // c) Calcualte d->TOF only
      //    Calcualte d-spacing
      d_h = m_unitCell.d(hkl[0], hkl[1], hkl[2]);
      //                  d_h = calCubicDSpace(latticesize, hkl[0], hkl[1],
      //                  hkl[2]);
      if ((d_h != d_h) || (d_h < -DBL_MAX) || (d_h > DBL_MAX)) {
        stringstream warnss;
        warnss << "Peak with Miller Index = " << hkl[0] << ", " << hkl[1] << ", " << hkl[2]
               << " has unphysical d-spacing value = " << d_h;
        g_log.warning(warnss.str());
        good = false;
        return newpeakptr;
      }

      //   Calcualte TOF_h
      double tof_h = calThermalNeutronTOF(d_h, dtt1, dtt1t, dtt2t, zero, zerot, width, tcross);
      newpeakptr->setCentre(tof_h);
    } else {
      // d) Calculate a lot of peak parameters
      // Initialize the function
      ThermalNeutronBk2BkExpConvPVoigt tnb2bfunc;
      tnb2bfunc.initialize();
      tnb2bfunc.setMillerIndex(hkl[0], hkl[1], hkl[2]);

      vector<string> tnb2bfuncparnames = tnb2bfunc.getParameterNames();

      // Set peak parameters
      for (const auto &parname : tnb2bfuncparnames) {
        if (parname != "Height") {
          auto miter = m_instrumentParmaeters.find(parname);
          if (miter == m_instrumentParmaeters.end()) {
            stringstream errss;
            errss << "Cannot find peak parameter " << parname << " in input instrument parameter "
                  << "TableWorkspace.  This mode is unable to execute. Quit!";
            g_log.error(errss.str());
            throw runtime_error(errss.str());
          }
          double parvalue = miter->second;
          tnb2bfunc.setParameter(parname, parvalue);
        }
      }

      // Calculate peak parameters A, B, S, and X0
      tnb2bfunc.calculateParameters(false);
      d_h = tnb2bfunc.getPeakParameter("d_h");
      double alpha = tnb2bfunc.getPeakParameter("Alpha");
      double beta = tnb2bfunc.getPeakParameter("Beta");
      double sigma2 = tnb2bfunc.getPeakParameter("Sigma2");
      double tof_h = tnb2bfunc.centre();

      newpeakptr->setParameter("A", alpha);
      newpeakptr->setParameter("B", beta);
      newpeakptr->setParameter("S", sqrt(sigma2));
      newpeakptr->setParameter("X0", tof_h);
    }

    peakcalmode = "Calculate all parameters by thermal neutron peak function.";

  } // Calculate peak variable from Profile function
  else if (m_genPeakStartingValue == FROMBRAGGTABLE) {
    // e) Import from input table workspace
    vector<string>::iterator nameiter;
    for (nameiter = mPeakParameterNames.begin(); nameiter != mPeakParameterNames.end(); ++nameiter) {
      // BackToBackExponential parameter name
      string b2bexpname = *nameiter;
      // Map to instrument parameter
      map<string, string>::iterator refiter;
      refiter = bk2bk2braggmap.find(b2bexpname);
      if (refiter == bk2bk2braggmap.end())
        throw runtime_error("Programming error!");
      string instparname = refiter->second;

      // Search in Bragg peak table
      map<string, double>::iterator miter;
      miter = parammap.find(instparname);
      if (miter != parammap.end()) {
        // Parameter exist in input
        double parvalue = miter->second;
        if (b2bexpname == "S2") {
          newpeakptr->setParameter("S", sqrt(parvalue));
        } else {
          newpeakptr->setParameter(b2bexpname, parvalue);
        }
      }
    } // FOR PEAK TABLE CELL

    peakcalmode = "Import from Bragg peaks table";
  }

  // Debug output
  stringstream infoss;
  string peakinfo = getFunctionInfo(std::dynamic_pointer_cast<IFunction>(newpeakptr));

  infoss << "Generate Peak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2] << ") Of Mode " << peakcalmode << ".\n";
  g_log.notice(infoss.str());
  g_log.information() << "[DBx426B] " << peakinfo << ".\n";

  good = true;

  return newpeakptr;
}

//----------------------------------------------------------------------------------------------
/** Plot a single peak to output vector
 *
 * @param domain:       domain/region of the peak to plot.  It is very
 *localized.
 * @param peakfunction: function to plot
 * @param background:   background of the peak
 */
void FitPowderDiffPeaks::plotFunction(const IFunction_sptr &peakfunction, const BackgroundFunction_sptr &background,
                                      const FunctionDomain1DVector &domain) {
  // 1. Determine range
  const auto &vecX = m_dataWS->x(m_wsIndex);
  double x0 = domain[0];
  auto viter = lower_bound(vecX.cbegin(), vecX.cend(), x0);
  int ix0 = static_cast<int>(std::distance(vecX.cbegin(), viter));

  // Check boundary
  if ((static_cast<int>(domain.size()) + ix0) > static_cast<int>(m_peakData.size()))
    throw runtime_error("Plot single peak out of boundary error!");

  // 2. Calculation of peaks
  FunctionValues values1(domain);
  peakfunction->function(domain, values1);

  for (int i = 0; i < static_cast<int>(domain.size()); ++i)
    m_peakData[i + ix0] = values1[i];

  // 3.Calculation of background
  FunctionValues values2(domain);
  background->function(domain, values2);

  for (int i = 0; i < static_cast<int>(domain.size()); ++i)
    m_peakData[i + ix0] += values2[i];
}

//=====================================  Auxiliary Functions
//===================================
//----------------------------------------------------------------------------------------------
/** Get (HKL) from a map
 * Return false if the information is incomplete
 * @param intmap:  map as a pair of string and integer
 * @param hkl:     output integer vector for miller index
 */
bool FitPowderDiffPeaks::getHKLFromMap(map<string, int> intmap, vector<int> &hkl) {
  vector<string> strhkl(3);
  strhkl[0] = "H";
  strhkl[1] = "K";
  strhkl[2] = "L";

  hkl.clear();

  map<string, int>::iterator miter;
  for (size_t i = 0; i < 3; ++i) {
    string parname = strhkl[i];
    miter = intmap.find(parname);
    if (miter == intmap.end())
      return false;

    hkl.emplace_back(miter->second);
  }

  return true;
}

//----------------------------------------------------------------------------------------------
/** Crop data workspace: the original workspace will not be affected
 * @param tofmin:  minimum value for cropping
 * @param tofmax:  maximum value for cropping
 */
void FitPowderDiffPeaks::cropWorkspace(double tofmin, double tofmax) {
  auto cropalg = createChildAlgorithm("CropWorkspace", -1, -1, true);
  cropalg->initialize();

  cropalg->setProperty("InputWorkspace", m_dataWS);
  cropalg->setPropertyValue("OutputWorkspace", "MyData");
  cropalg->setProperty("XMin", tofmin);
  cropalg->setProperty("XMax", tofmax);

  bool cropstatus = cropalg->execute();

  std::stringstream errmsg;
  if (!cropstatus) {
    errmsg << "DBx309 Cropping workspace unsuccessful.  Fatal Error. Quit!";
    g_log.error() << errmsg.str() << '\n';
    throw std::runtime_error(errmsg.str());
  }

  m_dataWS = cropalg->getProperty("OutputWorkspace");
  if (!m_dataWS) {
    errmsg << "Unable to retrieve a Workspace2D object from ChildAlgorithm Crop.";
    g_log.error(errmsg.str());
    throw std::runtime_error(errmsg.str());
  } else {
    g_log.information() << "[DBx211] Cropped Workspace Range: " << m_dataWS->x(m_wsIndex)[0] << ", "
                        << m_dataWS->x(m_wsIndex).back() << '\n';
  }
}

//----------------------------------------------------------------------------------------------
/** Get parameter value from m_instrumentParameters
 * Exception: throw runtime error if there is no such parameter
 * @param parname:  parameter name to get from m_instrumentParameters
 */
double FitPowderDiffPeaks::getParameter(const string &parname) {
  map<string, double>::iterator mapiter;
  mapiter = m_instrumentParmaeters.find(parname);

  if (mapiter == m_instrumentParmaeters.end()) {
    stringstream errss;
    errss << "Instrument parameter map (having " << m_instrumentParmaeters.size() << " entries) "
          << "does not have parameter " << parname << ". ";
    g_log.debug(errss.str());
    return (EMPTY_DBL());
  }

  return mapiter->second;
}

//----------------------------------------------------------------------------------------------
/** Build a partial workspace from original data workspace
 * @param sourcews:  data workspace which the new workspace is built from
 * @param workspaceindex:  workspace index of the source spectrum in sourcews
 * @param leftbound:  lower boundary of the source data
 * @param rightbound: upper boundary of the source data
 */
Workspace2D_sptr FitPowderDiffPeaks::buildPartialWorkspace(const API::MatrixWorkspace_sptr &sourcews,
                                                           size_t workspaceindex, double leftbound, double rightbound) {
  // 1. Check
  const auto &X = sourcews->x(workspaceindex);
  const auto &Y = sourcews->y(workspaceindex);
  const auto &E = sourcews->e(workspaceindex);

  if (leftbound >= rightbound) {
    stringstream errmsg;
    errmsg << "[BuildPartialWorkspace] Input left boundary = " << leftbound << " is larger than input right boundary "
           << rightbound << ".  It is not allowed. ";
    throw std::invalid_argument(errmsg.str());
  }
  if (leftbound >= X.back() || rightbound <= X[0]) {
    throw std::invalid_argument("Boundary is out side of the input data set. ");
  }

  // 2. Determine the size of the "partial" workspace
  int ileft = static_cast<int>(std::lower_bound(X.begin(), X.end(), leftbound) - X.begin());
  if (ileft > 0)
    --ileft;
  int iright = static_cast<int>(std::lower_bound(X.begin(), X.end(), rightbound) - X.begin());
  if (iright >= static_cast<int>(X.size()))
    iright = static_cast<int>(X.size() - 1);

  auto wssize = static_cast<size_t>(iright - ileft + 1);

  // 3. Build the partial workspace
  size_t nspec = 6;
  Workspace2D_sptr partws = std::dynamic_pointer_cast<Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", nspec, wssize, wssize));

  // 4. Put data there
  // TODO: Try to use vector copier
  for (size_t iw = 0; iw < partws->getNumberHistograms(); ++iw) {
    MantidVec &nX = partws->dataX(iw);
    for (size_t i = 0; i < wssize; ++i) {
      nX[i] = X[i + ileft];
    }
  }
  auto &nY = partws->mutableY(0);
  auto &nE = partws->mutableE(0);
  for (size_t i = 0; i < wssize; ++i) {
    nY[i] = Y[i + ileft];
    nE[i] = E[i + ileft];
  }

  return partws;
}

//----------------------------------------------------------------------------------------------
/** Get function parameter values information and returned as a string
 * @param function:  function to have information written out
 */
string getFunctionInfo(const IFunction_sptr &function) {
  stringstream outss;
  vector<string> parnames = function->getParameterNames();
  size_t numpars = parnames.size();
  outss << "Number of Parameters = " << numpars << '\n';
  for (size_t i = 0; i < numpars; ++i)
    outss << parnames[i] << " = " << function->getParameter(i) << ", \t\tFitted = " << function->isActive(i) << '\n';

  return outss.str();
}

//----------------------------------------------------------------------------------------------
/** Estimate background for a pattern in a coarse mode
 * Assumption: the peak must be in the data range completely
 * Algorithm: use two end data points for a linear background
 * Output: dataws spectrum 3 (workspace index 2)
 * @param dataws:  data workspace to estimate background for
 * @param background:  background function
 * @param wsindexraw:  workspace index of spectrum holding raw data
 * @param wsindexbkgd: workspace index of spectrum holding background data
 * @param wsindexpeak: workspace index of spectrum holding pure peak data (with
 * background removed)
 */
void estimateBackgroundCoarse(const DataObjects::Workspace2D_sptr &dataws, const BackgroundFunction_sptr &background,
                              size_t wsindexraw, size_t wsindexbkgd, size_t wsindexpeak) {
  // 1. Get prepared
  if (dataws->getNumberHistograms() < 3) {
    stringstream errss;
    errss << "Function estimateBackgroundCoase() requires input Workspace2D "
             "has at least 3 spectra."
          << "Present input has " << dataws->getNumberHistograms() << " spectra.";
    throw runtime_error(errss.str());
  }
  const auto &X = dataws->x(wsindexraw);
  const auto &Y = dataws->y(wsindexraw);

  // TODO: This is a magic number!
  size_t numsamplepts = 2;
  if (X.size() <= 10) {
    // Make it at minimum to estimate background
    numsamplepts = 1;
  }

  // 2. Average the first and last three data points
  double y0 = 0;
  double x0 = 0;

  for (size_t i = 0; i < numsamplepts; ++i) {
    x0 += X[i];
    y0 += Y[i];
  }
  x0 = x0 / static_cast<double>(numsamplepts);
  y0 = y0 / static_cast<double>(numsamplepts);

  double xf = 0;
  double yf = 0;
  for (size_t i = X.size() - numsamplepts; i < X.size(); ++i) {
    xf += X[i];
    yf += Y[i];
  }
  xf = xf / static_cast<double>(numsamplepts);
  yf = yf / static_cast<double>(numsamplepts);

  // 3. Calculate B(x) = B0 + B1*x
  double b1 = (yf - y0) / (xf - x0);
  double b0 = yf - b1 * xf;

  background->setParameter("A0", b0);
  background->setParameter("A1", b1);

  // 4. Calcualte background
  FunctionDomain1DVector domain(X.rawData());
  FunctionValues values(domain);
  background->function(domain, values);

  dataws->mutableY(wsindexbkgd) = values.toVector();
  dataws->mutableY(wsindexpeak) = dataws->y(wsindexraw) - dataws->y(wsindexbkgd);
  dataws->mutableE(wsindexpeak) = dataws->e(wsindexraw);
}

//-----------------------------------------------------------------------------------------------------------
/** Estimate peak parameters;
 * Prerequisit:
 * (1) Background removed
 * (2) Peak is inside
 * Algorithm: From the top.  Get the maximum value. Calculate the half maximum
 * value.  Find the range of X
 * @param dataws:  data workspace containing peak data
 * @param wsindex :: index of a histogram
 * @param centre :: (output) peak centre
 * @param height :: (output) peak height
 * @param fwhm :: (output) peak FWHM
 * @param errmsg :: (output) error message
 */
bool observePeakParameters(const Workspace2D_sptr &dataws, size_t wsindex, double &centre, double &height, double &fwhm,
                           string &errmsg) {
  // 1. Get the value of the Max Height
  const auto &X = dataws->x(wsindex);
  const auto &Y = dataws->y(wsindex);

  // 2. The highest peak should be the centre
  size_t icentre = findMaxValue(Y.rawData());
  centre = X[icentre];
  height = Y[icentre];

  if (icentre <= 1 || icentre > X.size() - 2) {
    stringstream errss;
    errss << "Peak center = " << centre << " is at the edge of the input workspace [" << X[0] << ", " << X.back()
          << ". It is unable to proceed the estimate of FWHM.  Quit with error!.";
    errmsg = errss.str();
    return false;
  }
  if (height <= 0) {
    stringstream errss;
    errss << "Max height = " << height << " in input workspace [" << X[0] << ", " << X.back()
          << " is negative.  Fatal error is design of the algorithm.";
    errmsg = errss.str();
    return false;
  }

  // 3. Calculate FWHM
  double halfMax = height * 0.5;

  // a) Deal with left side
  bool continueloop = true;
  size_t index = icentre - 1;
  while (continueloop) {
    if (Y[index] <= halfMax) {
      // Located the data points
      continueloop = false;
    } else if (index == 0) {
      // Reach the end of the boundary, but haven't found.  return with error.
      stringstream errss;
      errss << "The peak is not complete (left side) in the given data range.";
      errmsg = errss.str();
      return false;
    } else {
      // Contineu to locate
      --index;
    }
  }
  double x0 = X[index];
  double xf = X[index + 1];
  double y0 = Y[index];
  double yf = Y[index + 1];

  // Formular for linear iterpolation: X = [(xf-x0)*Y - (xf*y0-x0*yf)]/(yf-y0)
  double xl = linearInterpolateX(x0, xf, y0, yf, halfMax);

  double lefthalffwhm = centre - xl;

  // 3. Deal with right side
  continueloop = true;
  index = icentre + 1;
  while (continueloop) {
    if (Y[index] <= halfMax) {
      // Located the data points
      continueloop = false;
    } else if (index == Y.size() - 1) {
      // Reach the end of the boundary, but haven't found.  return with error.
      stringstream errss;
      errss << "The peak is not complete (right side) in the given data range.";
      errmsg = errss.str();
      return false;
    } else {
      ++index;
    }
  }
  x0 = X[index - 1];
  xf = X[index];
  y0 = Y[index - 1];
  yf = Y[index];

  // Formula for linear iterpolation: X = [(xf-x0)*Y - (xf*y0-x0*yf)]/(yf-y0)
  double xr = linearInterpolateX(x0, xf, y0, yf, halfMax);

  double righthalffwhm = xr - centre;

  // Final
  fwhm = lefthalffwhm + righthalffwhm;

  return true;
}

//-----------------------------------------------------------------------------------------------------------
/** Find maximum value
 * @param Y :: vector to get maximum value from
 * @return index of the maximum value
 */
size_t findMaxValue(const std::vector<double> &Y) {

  auto maxIt = std::max_element(Y.begin(), Y.end());
  return std::distance(Y.begin(), maxIt);
}

//----------------------------------------------------------------------------------------------
/** Find maximum value
 * @param dataws :: a workspace to check
 * @param wsindex :: the spectrum to check
 * @param leftbound :: lower constraint for check
 * @param rightbound :: upper constraint for check
 * @return the index of the maximum value
 */
size_t findMaxValue(const MatrixWorkspace_sptr &dataws, size_t wsindex, double leftbound, double rightbound) {
  const auto &X = dataws->x(wsindex);
  const auto &Y = dataws->y(wsindex);

  // 1. Determine xmin, xmax range
  std::vector<double>::const_iterator viter;

  viter = std::lower_bound(X.begin(), X.end(), leftbound);
  size_t ixmin = size_t(viter - X.begin());
  if (ixmin != 0)
    --ixmin;
  viter = std::lower_bound(X.begin(), X.end(), rightbound);
  size_t ixmax = size_t(viter - X.begin());

  // 2. Search imax
  size_t imax = ixmin;
  double maxY = Y[ixmin];
  for (size_t i = ixmin + 1; i <= ixmax; ++i) {
    if (Y[i] > maxY) {
      maxY = Y[i];
      imax = i;
    }
  }

  return imax;
}

} // namespace Mantid::CurveFitting::Algorithms
