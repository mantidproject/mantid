//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FitPeaks.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/EstimatePolynomial.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"

#include "MantidAPI/MultiDomainFunction.h"

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/trim.hpp"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;
using namespace std;

const size_t MIN_EVENTS = 100;

namespace Mantid {
namespace Algorithms {

namespace FitPeaksAlgorithm {

//----------------------------------------------------------------------------------------------
/** Initiailization
 * @brief PeakFitResult::PeakFitResult
 * @param num_peaks
 * @param num_params
 */
PeakFitResult::PeakFitResult(size_t num_peaks, size_t num_params) {
  // check input
  if (num_peaks == 0 || num_params == 0)
    throw std::runtime_error("No peak or no parameter error.");
  function_parameters_number_ = num_params;

  //
  fitted_peak_positions.resize(num_peaks, -1);
  costs.resize(num_peaks, DBL_MAX);
  function_parameters_vector.resize(num_peaks);
  for (size_t ipeak = 0; ipeak < num_peaks; ++ipeak) {
    function_parameters_vector[ipeak].resize(num_params);
  }

  return;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief PeakFitResult::getNumberParameters
 * @return
 */
size_t PeakFitResult::getNumberParameters() {
  return function_parameters_number_;
}

double PeakFitResult::getParameterValue(size_t ipeak, size_t iparam) {
  return function_parameters_vector[ipeak][iparam];
}

//----------------------------------------------------------------------------------------------
/**
 * @brief PeakFitResult::getPeakPosition
 * @param ipeak
 * @return
 */
double PeakFitResult::getPeakPosition(size_t ipeak) {
  return fitted_peak_positions[ipeak];
}

//----------------------------------------------------------------------------------------------
/**
 * @brief PeakFitResult::getCost
 * @param ipeak
 * @return
 */
double PeakFitResult::getCost(size_t ipeak) { return costs[ipeak]; }

//----------------------------------------------------------------------------------------------
/** set the peak fitting record/parameter for one peak
 * @brief PeakFitResult::setRecord
 * @param ipeak
 * @param cost
 * @param peak_position
 * @param fit_functions
 */
void PeakFitResult::setRecord(size_t ipeak, const double cost,
                              const double peak_position,
                              FitFunction fit_functions) {
  // check input
  if (ipeak >= costs.size())
    throw std::runtime_error("Peak index is out of range.");

  // set the values
  costs[ipeak] = cost;

  // set peak position
  fitted_peak_positions[ipeak] = peak_position;

  // transfer from peak function to vector
  size_t peak_num_params = fit_functions.peakfunction->nParams();
  for (size_t ipar = 0; ipar < peak_num_params; ++ipar) {
    // peak function
    function_parameters_vector[ipeak][ipar] =
        fit_functions.peakfunction->getParameter(ipar);
  }
  for (size_t ipar = 0; ipar < fit_functions.bkgdfunction->nParams(); ++ipar) {
    // background function
    function_parameters_vector[ipeak][ipar + peak_num_params] =
        fit_functions.bkgdfunction->getParameter(ipar);
  }

  return;
}
}

//----------------------------------------------------------------------------------------------
/** Get an index of a value in a sorted vector.  The index should be the item
 * with value nearest to X
  */
size_t findXIndex(const std::vector<double> &vecx, double x) {
  size_t index;
  if (x <= vecx.front()) {
    index = 0;
  } else if (x >= vecx.back()) {
    index = vecx.size() - 1;
  } else {
    vector<double>::const_iterator fiter =
        lower_bound(vecx.begin(), vecx.end(), x);
    if (fiter == vecx.end())
      throw runtime_error("It seems impossible to have this value. ");

    index = static_cast<size_t>(fiter - vecx.begin());
    if (x - vecx[index - 1] < vecx[index] - x)
      --index;
  }

  return index;
}

enum PeakFitResult { NOSIGNAL, LOWPEAK, OUTOFBOUND, GOOD };

//----------------------------------------------------------------------------------------------
/** constructor
 * @brief FitPeaks::FitPeaks
 */
FitPeaks::FitPeaks()
    : m_fitPeaksFromRight(true), m_numPeaksToFit(0), m_minPeakHeight(20.),
      m_bkgdSimga(1.), m_peakPosTolCase234(false) {}

//----------------------------------------------------------------------------------------------
/** initialize the properties
 * @brief FitPeaks::init
 */
void FitPeaks::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace for peak fitting.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace containing peak centers for "
                  "fitting offset."
                  "The output workspace is point data."
                  "Each workspace index corresponds to a spectrum. "
                  "Each X value ranges from 0 to N-1, where N is the number of "
                  "peaks to fit. "
                  "Each Y value is the peak position obtained by peak fitting. "
                  "Negative value is used for error signals. "
                  "-1 for data is zero;  -2 for maximum value is smaller than "
                  "specified minimum value."
                  "and -3 for non-converged fitting.");

  // properties about fitting range and criteria
  declareProperty("StartWorkspaceIndex", EMPTY_INT(),
                  "Starting workspace index for fit");
  declareProperty("StopWorkspaceIndex", EMPTY_INT(),
                  "Last workspace index to fit (which is included)");

  // properties about peak positions to fit
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("PeakCenters"),
                  "List of peak centers to fit against.");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "PeakCentersWorkspace", "", Direction::Input, PropertyMode::Optional),
      "MatrixWorkspace containing peak centers");

  std::string peakcentergrp("Peak Positions");
  setPropertyGroup("PeakCenters", peakcentergrp);
  setPropertyGroup("PeakCentersWorkspace", peakcentergrp);

  // properties about peak profile
  std::vector<std::string> peakNames =
      FunctionFactory::Instance().getFunctionNames<API::IPeakFunction>();
  declareProperty("PeakFunction", "Gaussian",
                  boost::make_shared<StringListValidator>(peakNames));
  vector<string> bkgdtypes{"Flat", "Linear", "Quadratic"};
  declareProperty("BackgroundType", "Linear",
                  boost::make_shared<StringListValidator>(bkgdtypes),
                  "Type of Background.");

  std::string funcgroup("Function Types");
  setPropertyGroup("PeakFunction", funcgroup);
  setPropertyGroup("BackgroundType", funcgroup);

  // properties about peak range including fitting window and peak width
  // (percentage)
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("FitWindowBoundaryList"),
      "List of left boundaries of the peak fitting window corresponding to "
      "PeakCenters.");

  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "FitPeakWindowWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "MatrixWorkspace for of peak windows");

  auto min = boost::make_shared<BoundedValidator<double>>();
  min->setLower(1e-3);
  declareProperty("PeakWidthPercent", EMPTY_DBL(), min,
                  "The estimated peak width as a "
                  "percentage of the d-spacing "
                  "of the center of the peak.");

  std::string fitrangeegrp("Peak Range Setup");
  setPropertyGroup("PeakWidthPercent", fitrangeegrp);
  setPropertyGroup("FitWindowBoundaryList", fitrangeegrp);
  setPropertyGroup("FitPeakWindowWorkspace", fitrangeegrp);

  // properties about peak parameters' names and value
  declareProperty(
      Kernel::make_unique<ArrayProperty<std::string>>("PeakParameterNames"),
      "List of peak parameters' names");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("PeakParameterValues"),
      "List of peak parameters' value");
  declareProperty(Kernel::make_unique<WorkspaceProperty<TableWorkspace>>(
                      "PeakParameterValueTable", "", Direction::Input,
                      PropertyMode::Optional),
                  "Name of the an optional workspace, whose each column "
                  "corresponds to given peak parameter names"
                  ", and each row corresponds to a subset of spectra.");

  std::string startvaluegrp("Strting Parameters Setup");
  setPropertyGroup("PeakParameterNames", startvaluegrp);
  setPropertyGroup("PeakParameterValues", startvaluegrp);
  setPropertyGroup("PeakParameterValueTable", startvaluegrp);

  // optimization setup
  declareProperty("FitFromRight", true,
                  "Flag for the order to fit peaks.  If true, peaks are fitted "
                  "from rightmost;"
                  "Otherwise peaks are fitted from leftmost.");

  std::vector<std::string> minimizerOptions =
      API::FuncMinimizerFactory::Instance().getKeys();
  declareProperty("Minimizer", "Levenberg-Marquardt",
                  Kernel::IValidator_sptr(
                      new Kernel::StartsWithValidator(minimizerOptions)),
                  "Minimizer to use for fitting. Minimizers available are "
                  "\"Levenberg-Marquardt\", \"Simplex\","
                  "\"Conjugate gradient (Fletcher-Reeves imp.)\", \"Conjugate "
                  "gradient (Polak-Ribiere imp.)\", \"BFGS\", and "
                  "\"Levenberg-MarquardtMD\"");

  std::array<string, 2> costFuncOptions = {{"Least squares", "Rwp"}};
  declareProperty("CostFunction", "Least squares",
                  Kernel::IValidator_sptr(
                      new Kernel::ListValidator<std::string>(costFuncOptions)),
                  "Cost functions");

  std::string optimizergrp("Optimization Setup");
  setPropertyGroup("Minimizer", optimizergrp);
  setPropertyGroup("CostFunction", optimizergrp);

  // other helping information
  declareProperty(
      "FindBackgroundSigma", 1.0,
      "Multiplier of standard deviations of the variance for convergence of "
      "peak elimination.  Default is 1.0. ");

  declareProperty("HighBackground", true,
                  "Flag whether the data has high background comparing to "
                  "peaks' intensities. "
                  "For example, vanadium peaks usually have high background.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "EventNumberWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Name of an optional workspace, whose each spectrum corresponds to each "
      "spectrum "
      "in input workspace. "
      "It has 1 value of each spectrum, standing for the number of events of "
      "the corresponding spectrum.");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("PositionTolerance"),
      "List of tolerance on fitted peak positions against given peak positions."
      "If there is only one value given, then ");

  declareProperty("MinimumPeakHeight", EMPTY_DBL(),
                  "Minimum peak height such that all the fitted peaks with "
                  "height under this value will be excluded.");

  declareProperty(
      "ConstrainPeakPositions", true,
      "If true peak position will be constrained by estimated positions "
      "(highest Y value position) and "
      "the peak width either estimted by observation or calculate.");

  std::string helpgrp("Additional Information");

  setPropertyGroup("EventNumberWorkspace", helpgrp);

  // additional output for reviewing
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      "OutputPeakParametersWorkspace", "", Direction::Output),
                  "Name of workspace containing all fitted peak parameters.  "
                  "X-values are spectra/workspace index.");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "FittedPeaksWorkspace", "", Direction::Output,
          PropertyMode::Optional),
      "Name of the output matrix workspace with fitted peak. "
      "This output workspace have the same dimesion as the input workspace."
      "The Y values belonged to peaks to fit are replaced by fitted value. "
      "Values of estimated background are used if peak fails to be fit.");

  std::string addoutgrp("Analysis");
  setPropertyGroup("OutputPeakParametersWorkspace", addoutgrp);
  setPropertyGroup("FittedPeaksWorkspace", addoutgrp);

  return;
}

//----------------------------------------------------------------------------------------------
/** main method to fit peaks
 * @brief FitPeaks::exec
 */
void FitPeaks::exec() {
  // process inputs
  processInputs();

  // create output workspaces
  generateOutputPeakPositionWS();
  generateFittedParametersValueWorkspace();
  generateCalculatedPeaksWS();

  // fit peaks
  fitPeaks();

  // set the output workspaces to properites
  processOutputs();
}

//----------------------------------------------------------------------------------------------
/** process inputs
 * @brief FitPeaks::processInputs
 */
void FitPeaks::processInputs() {
  // input workspaces
  m_inputMatrixWS = getProperty("InputWorkspace");
  std::string event_ws_name = getPropertyValue("EventNumberWorkspace");
  if (event_ws_name.empty())
    m_eventNumberWS = nullptr;
  else
    m_eventNumberWS = getProperty("EventNumberWorkspace");

  if (m_inputMatrixWS->getAxis(0)->unit()->unitID() == "dSpacing")
    m_inputIsDSpace = true;
  else
    m_inputIsDSpace = false;

  // spectra to fit
  int start_wi = getProperty("StartWorkspaceIndex");
  if (isEmpty(start_wi))
    m_startWorkspaceIndex = 0;
  else
    m_startWorkspaceIndex = static_cast<size_t>(start_wi);

  // last spectrum's workspace index, which is included
  int stop_wi = getProperty("StopWorkspaceIndex");
  if (isEmpty(stop_wi))
    m_stopWorkspaceIndex = m_inputMatrixWS->getNumberHistograms() - 1;
  else {
    m_stopWorkspaceIndex = static_cast<size_t>(stop_wi);
    if (m_stopWorkspaceIndex > m_inputMatrixWS->getNumberHistograms() - 1)
      m_stopWorkspaceIndex = m_inputMatrixWS->getNumberHistograms() - 1;
  }

  // optimizer, cost function and fitting scheme
  m_minimizer = getPropertyValue("Minimizer");
  m_costFunction = getPropertyValue("CostFunction");
  m_fitPeaksFromRight = getProperty("FitFromRight");
  m_constrainPeaksPosition = getProperty("ConstrainPeakPositions");

  // Peak centers, tolerance and fitting range
  processInputPeakCenters();
  // check
  if (m_numPeaksToFit == 0)
    throw std::runtime_error("number of peaks to fit is zero.");
  // about how to estimate the peak width
  m_peakDSpacePercentage = getProperty("PeakWidthPercent");
  if (isEmpty(m_peakDSpacePercentage))
    m_peakDSpacePercentage = -1;
  else if (m_peakDSpacePercentage <= 0)
    throw std::invalid_argument(
        "Peak D-spacing percentage cannot be negative or zero!");
  g_log.debug() << "DeltaD/D = " << m_peakDSpacePercentage << "\n";

  // set up background
  m_highBackground = getProperty("HighBackground");
  m_bkgdSimga = getProperty("FindBackgroundSigma");

  // Set up peak and background functions
  processInputFunctions();
  // about peak width and other peak parameter estimating method
  if (m_inputIsDSpace && m_peakDSpacePercentage > 0)
    m_peakWidthEstimateApproach = EstimatePeakWidth::InstrumentResolution;
  else if (m_peakFunction->name() == "Gaussian")
    m_peakWidthEstimateApproach = EstimatePeakWidth::Observation;
  else
    m_peakWidthEstimateApproach = EstimatePeakWidth::NoEstimation;
  g_log.debug() << "Process inputs [3] peak type: " << m_peakFunction->name()
                << ", background type: " << m_bkgdFunction->name() << "\n";

  processInputPeakTolerance();
  processInputFitRanges();

  return;
}

//----------------------------------------------------------------------------------------------
/** process inputs for peak profile and background
 * @brief FitPeaks::processInputFunctions
 */
void FitPeaks::processInputFunctions() {
  // peak functions
  std::string peakfunctiontype = getPropertyValue("PeakFunction");
  m_peakFunction = boost::dynamic_pointer_cast<IPeakFunction>(
      API::FunctionFactory::Instance().createFunction(peakfunctiontype));

  // background functions
  std::string bkgdfunctiontype = getPropertyValue("BackgroundType");
  std::string bkgdname;
  if (bkgdfunctiontype == "Linear")
    bkgdname = "LinearBackground";
  else if (bkgdfunctiontype == "Flat")
    bkgdname = "FlatBackground";
  else
    bkgdname = bkgdfunctiontype;
  m_bkgdFunction = boost::dynamic_pointer_cast<IBackgroundFunction>(
      API::FunctionFactory::Instance().createFunction(bkgdname));
  if (m_highBackground)
    m_linearBackgroundFunction =
        boost::dynamic_pointer_cast<IBackgroundFunction>(
            API::FunctionFactory::Instance().createFunction(
                "LinearBackground"));
  else
    m_linearBackgroundFunction = nullptr;

  // input peak parameters
  std::string partablename = getPropertyValue("PeakParameterValueTable");
  m_peakParamNames = getProperty("PeakParameterNames");
  if (partablename.empty() && (!m_peakParamNames.empty())) {
    // use uniform starting value of peak parameters
    m_initParamValues = getProperty("PeakParameterValues");
    // check whether given parameter names and initial values match
    if (m_peakParamNames.size() != m_initParamValues.size())
      throw std::invalid_argument("PeakParameterNames and PeakParameterValues "
                                  "have different number of items.");
    // convert the parameter name in string to parameter name in integer index
    convertParametersNameToIndex();
    // set the flag
    m_uniformProfileStartingValue = true;
  } else if ((!partablename.empty()) && m_peakParamNames.empty()) {
    // use non-uniform starting value of peak parameters
    m_uniformProfileStartingValue = false;
    m_profileStartingValueTable = getProperty(partablename);
  } else if ((!partablename.empty()) && m_peakParamNames.size() > 0) {
    // user specifies both of them causing confusion
    throw std::invalid_argument("Parameter value table and initial parameter "
                                "name/value vectors cannot be given "
                                "simultanenously.");
  } else {
    // user specifies nothing
    g_log.warning("Neither parameter value table nor initial "
                  "parameter name/value vectors is specified. Fitting might "
                  "not be reliable for peak profile other than Gaussian");
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** process and check for inputs about peak fitting range (i.e., window)
 * Note: What is the output of the method?
 * @brief FitPeaks::processInputFitRanges
 */
void FitPeaks::processInputFitRanges() {
  // get peak fit window
  std::vector<double> peakwindow = getProperty("FitWindowBoundaryList");
  std::string peakwindowname = getPropertyValue("FitPeakWindowWorkspace");

  // in most case, calculate window by instrument resolution is False
  m_calculateWindowInstrument = false;

  if ((!peakwindow.empty()) && peakwindowname.empty()) {
    // Peak windows are uniform among spectra: use vector for peak windows
    m_uniformPeakWindows = true;

    // check peak positions
    if (!m_uniformPeakPositions)
      throw std::invalid_argument(
          "Uniform peak range/window requires uniform peak positions.");
    // check size
    if (peakwindow.size() != m_numPeaksToFit * 2)
      throw std::invalid_argument(
          "Peak window vector must be twice as large as number of peaks.");

    // set up window to m_peakWindowVector
    m_peakWindowVector.resize(m_numPeaksToFit);
    for (size_t i = 0; i < m_numPeaksToFit; ++i) {
      std::vector<double> peakranges(2);
      peakranges[0] = peakwindow[i * 2];
      peakranges[1] = peakwindow[i * 2 + 1];
      // check peak window (range) against peak centers
      if ((peakranges[0] < m_peakCenters[i]) &&
          (m_peakCenters[i] < peakranges[1])) {
        // pass check: set
        m_peakWindowVector[i] = peakranges;
      } else {
        // failed
        std::stringstream errss;
        errss << "Peak " << i
              << ": user specifies an invalid range and peak center against "
              << peakranges[0] << " < " << m_peakCenters[i] << " < "
              << peakranges[1];
        throw std::invalid_argument(errss.str());
      }
    } // END-FOR
    // END for uniform peak window
  } else if (peakwindow.empty() && (!peakwindowname.empty())) {
    // use matrix workspace for non-uniform peak windows
    m_peakWindowWorkspace = getProperty("FitPeakWindowWorkspace");
    m_uniformPeakWindows = false;

    // check size
    if (m_peakWindowWorkspace->getNumberHistograms() ==
        m_inputMatrixWS->getNumberHistograms())
      m_partialWindowSpectra = false;
    else if (m_peakWindowWorkspace->getNumberHistograms() ==
             (m_stopWorkspaceIndex - m_startWorkspaceIndex + 1))
      m_partialWindowSpectra = true;
    else
      throw std::invalid_argument(
          "Peak window workspace has unmatched number of spectra");

    // check range for peak windows and peak positions
    size_t window_index_start(0);
    if (m_partialWindowSpectra)
      window_index_start = m_startWorkspaceIndex;
    size_t center_index_start(0);
    if (m_partialSpectra)
      center_index_start = m_startWorkspaceIndex;

    // check each spectrum whether the window is defined with the correct size
    for (size_t wi = 0; wi < m_peakWindowWorkspace->getNumberHistograms();
         ++wi) {
      // check size
      if (m_peakWindowWorkspace->y(wi).size() != m_numPeaksToFit * 2) {
        std::stringstream errss;
        errss << "Peak window workspace index " << wi
              << " has incompatible number of fit windows (x2) "
              << m_peakWindowWorkspace->y(wi).size()
              << "with the number of peaks " << m_numPeaksToFit << " to fit.";
        throw std::invalid_argument(errss.str());
      }

      // check window range against peak center
      size_t window_index = window_index_start + wi;
      size_t center_index = window_index - center_index_start;

      for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
        double left_w_bound = m_peakWindowWorkspace->y(wi)[ipeak * 2];
        double right_w_bound = m_peakWindowWorkspace->y(wi)[ipeak * 2 + 1];
        double center = m_peakCenterWorkspace->x(center_index)[ipeak];
        if (!(left_w_bound < center && center < right_w_bound)) {
          std::stringstream errss;
          errss << "Workspace index " << wi << " has incompatible peak window ("
                << left_w_bound << ", " << right_w_bound << ") with " << ipeak
                << "-th expected peak's center " << center;
          throw std::runtime_error(errss.str());
        }
      }
    }
  } else if (peakwindow.empty()) {
    // no peak window is defined, then the peak window will be estimated by
    // delta(D)/D
    if (m_inputIsDSpace && m_peakDSpacePercentage > 0)
      m_calculateWindowInstrument = true;
    else
      throw std::invalid_argument("Without definition of peak window, the "
                                  "input workspace must be in unit of dSpacing "
                                  "and Delta(D)/D must be given!");

  } else {
    // non-supported situation
    throw std::invalid_argument("One and only one of peak window array and "
                                "peak window workspace can be specified.");
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Processing peaks centers and fitting tolerance information from input.  the
 * parameters that are
 * set including
 * 1. m_peakCenters/m_peakCenterWorkspace/m_uniformPeakPositions
 * (bool)/m_partialSpectra (bool)
 * 2. m_peakPosTolerances (vector)
 * 3. m_numPeaksToFit
 * @brief FitPeaks::processInputPeakCenters
 */
void FitPeaks::processInputPeakCenters() {
  // peak centers
  m_peakCenters = getProperty("PeakCenters");
  std::string peakpswsname = getPropertyValue("PeakCentersWorkspace");
  if ((!m_peakCenters.empty()) && peakpswsname.empty()) {
    // peak positions are uniform among all spectra
    m_uniformPeakPositions = true;
    // number of peaks to fit!
    m_numPeaksToFit = m_peakCenters.size();
  } else if (m_peakCenters.empty() && (!peakpswsname.empty())) {
    // peak positions can be different among spectra
    m_uniformPeakPositions = false;
    m_peakCenterWorkspace = getProperty("PeakCentersWorkspace");
    // number of peaks to fit!
    m_numPeaksToFit = m_peakCenterWorkspace->x(0).size();

    // check matrix worksapce for peak positions
    const size_t numhist = m_peakCenterWorkspace->getNumberHistograms();
    if (numhist == m_inputMatrixWS->size())
      m_partialSpectra = false;
    else if (numhist == m_stopWorkspaceIndex - m_startWorkspaceIndex + 1)
      m_partialSpectra = true;
    else
      throw std::invalid_argument(
          "Input peak center workspace has wrong number of spectra.");

  } else {
    std::stringstream errss;
    errss << "One and only one in 'PeakCenters' (vector) and "
             "'PeakCentersWorkspace' shall be given. "
          << "'PeakCenters' has size " << m_peakCenters.size()
          << ", and name of peak center workspace "
          << "is " << peakpswsname;
    throw std::invalid_argument(errss.str());
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Processing peak fitting tolerance information from input.  The parameters
 * that are
 * set including
 * 2. m_peakPosTolerances (vector)
 * @brief FitPeaks::ProcessInputPeakTolerance
 */
void FitPeaks::processInputPeakTolerance() {
  // check code integrity
  if (m_numPeaksToFit == 0)
    throw std::runtime_error("ProcessInputPeakTolerance() must be called after "
                             "ProcessInputPeakCenters()");

  // peak tolerance
  m_peakPosTolerances = getProperty("PositionTolerance");

  if (m_peakPosTolerances.empty()) {
    // case 2, 3, 4
    m_peakPosTolerances.clear();
    m_peakPosTolCase234 = true;
  } else if (m_peakPosTolerances.size() == 1) {
    // only 1 uniform peak position tolerance is defined: expand to all peaks
    double peak_tol = m_peakPosTolerances[0];
    m_peakPosTolerances.resize(m_numPeaksToFit, peak_tol);
  } else if (m_peakPosTolerances.size() != m_numPeaksToFit) {
    // not uniform but number of peaks does not match
    g_log.error() << "number of peak position tolerance "
                  << m_peakPosTolerances.size()
                  << " is not same as number of peaks " << m_numPeaksToFit
                  << "\n";
    throw std::runtime_error("Number of peak position tolerances and number of "
                             "peaks to fit are inconsistent.");
  }

  // minimum peak height: set default to zero
  m_minPeakHeight = getProperty("MinimumPeakHeight");
  if (isEmpty(m_minPeakHeight))
    m_minPeakHeight = 0.;

  return;
}

//----------------------------------------------------------------------------------------------
/** Convert the input initial parameter name/value to parameter index/value for
 * faster access
 * according to the parameter name and peak profile function
 * @brief FitPeaks::ConvertParametersNameToIndex
 * Output: m_initParamIndexes will be set up
 */
void FitPeaks::convertParametersNameToIndex() {
  // get a map for peak profile parameter name and parameter index
  std::map<std::string, size_t> parname_index_map;
  for (size_t iparam = 0; iparam < m_peakFunction->nParams(); ++iparam)
    parname_index_map.insert(
        std::make_pair(m_peakFunction->parameterName(iparam), iparam));

  // define peak parameter names (class variable) if using table
  if (m_profileStartingValueTable)
    m_peakParamNames = m_profileStartingValueTable->getColumnNames();

  // map the input parameter names to parameter indexes
  for (const auto &paramName : m_peakParamNames) {
    std::map<std::string, size_t>::iterator locator =
        parname_index_map.find(paramName);
    if (locator != parname_index_map.end())
      m_initParamIndexes.push_back(locator->second);
    else {
      // a parameter name that is not defined in the peak profile function.  An
      // out-of-range index is thus set to this
      g_log.warning() << "Given peak parameter " << paramName
                      << " is not an allowed parameter of peak "
                         "function " << m_peakFunction->name() << "\n";
      m_initParamIndexes.push_back(m_peakFunction->nParams() * 10);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** main method to fit peaks among all
 * @brief FitPeaks::fitPeaks
 */
void FitPeaks::fitPeaks() {
  // cppcheck-suppress syntaxError
  PRAGMA_OMP(parallel for schedule(dynamic, 1) )
  for (int wi = static_cast<int>(m_startWorkspaceIndex);
       wi <= static_cast<int>(m_stopWorkspaceIndex); ++wi) {

    PARALLEL_START_INTERUPT_REGION

    // peaks to fit
    std::vector<double> expected_peak_centers =
        getExpectedPeakPositions(static_cast<size_t>(wi));

    // initialize output for this
    size_t numfuncparams =
        m_peakFunction->nParams() + m_bkgdFunction->nParams();
    boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result =
        boost::make_shared<FitPeaksAlgorithm::PeakFitResult>(m_numPeaksToFit,
                                                             numfuncparams);

    // check number of events
    bool noevents(false);
    if (m_eventNumberWS &&
        m_eventNumberWS->histogram(static_cast<size_t>(wi)).x()[0] < 1.0) {
      // no event with additional event number workspace
      noevents = true;
    } else if (m_inputEventWS &&
               m_inputEventWS->getNumberEvents() < MIN_EVENTS) {
      // too few events for peak fitting
      noevents = true;
    } else {
      // fit
      fitSpectrumPeaks(static_cast<size_t>(wi), expected_peak_centers,
                       fit_result);
      //    fitted_peak_centers, fitted_parameters, &peak_chi2_vec);
    }

    PARALLEL_CRITICAL(FindPeaks_WriteOutput) {
      writeFitResult(static_cast<size_t>(wi), expected_peak_centers, fit_result,
                     // fitted_peak_centers, fitted_parameters, peak_chi2_vec,
                     noevents);
    }

    PARALLEL_END_INTERUPT_REGION
  }

  PARALLEL_CHECK_INTERUPT_REGION
}

//----------------------------------------------------------------------------------------------
/** Fit peaks across one single spectrum
 * @brief FitPeaks::fitSpectrumPeaks
 * @param wi
 * @param expected_peak_centers
 * @param fit_result
 */
void FitPeaks::fitSpectrumPeaks(
    size_t wi, const std::vector<double> &expected_peak_centers,
    boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result) {
  // Set up sub algorithm Fit for peak and background
  IAlgorithm_sptr peak_fitter; // both peak and background (combo)
  try {
    peak_fitter = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    std::stringstream errss;
    errss << "The FitPeak algorithm requires the CurveFitting library";
    g_log.error(errss.str());
    throw std::runtime_error(errss.str());
  }

  // Clone the function
  IPeakFunction_sptr peakfunction =
      boost::dynamic_pointer_cast<API::IPeakFunction>(m_peakFunction->clone());
  IBackgroundFunction_sptr bkgdfunction =
      boost::dynamic_pointer_cast<API::IBackgroundFunction>(
          m_bkgdFunction->clone());
  CompositeFunction_sptr compfunc = boost::make_shared<CompositeFunction>();
  compfunc->addFunction(peakfunction);
  compfunc->addFunction(bkgdfunction);

  // high background to reduce
  API::IBackgroundFunction_sptr high_bkgd_func(nullptr);
  if (m_linearBackgroundFunction)
    high_bkgd_func = boost::dynamic_pointer_cast<API::IBackgroundFunction>(
        m_linearBackgroundFunction->clone());

  // set up properties of algorithm (reference) 'Fit'
  peak_fitter->setProperty("Minimizer", m_minimizer);
  peak_fitter->setProperty("CostFunction", m_costFunction);
  peak_fitter->setProperty("CalcErrors", true);

  for (size_t fit_index = 0; fit_index < m_numPeaksToFit; ++fit_index) {

    // convert fit index to peak index (in ascending order)
    size_t peak_index(fit_index);
    if (m_fitPeaksFromRight)
      peak_index = m_numPeaksToFit - fit_index - 1;

    // get expected peak position
    double expected_peak_pos = expected_peak_centers[peak_index];
    double x0 = m_inputMatrixWS->histogram(wi).x().front();
    double xf = m_inputMatrixWS->histogram(wi).x().back();
    double cost(DBL_MAX);
    if (expected_peak_pos <= x0 || expected_peak_pos >= xf) {
      // out of range and there won't be any fit
      peakfunction->setIntensity(0);
      peakfunction->setCentre(expected_peak_pos);
    } else {
      // find out the peak position to fit
      std::pair<double, double> peak_window_i =
          getPeakFitWindow(wi, peak_index);

      bool observe_peak_width =
          decideToEstimatePeakWidth(fit_index, peakfunction);

      // do fitting with peak and background function (no analysis at this
      // point)
      cost = fitIndividualPeak(wi, peak_fitter, expected_peak_pos,
                               peak_window_i, m_highBackground, high_bkgd_func,
                               observe_peak_width, peakfunction, bkgdfunction);
    }

    // process fitting result
    FitPeaksAlgorithm::FitFunction fit_function;
    fit_function.peakfunction = peakfunction;
    fit_function.bkgdfunction = bkgdfunction;

    processSinglePeakFitResult(wi, peak_index, cost, expected_peak_centers,
                               fit_function, fit_result);
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Decide whether to estimate peak width.  If not, then set the width related
 * peak parameters from
 * user specified starting value
 * @brief FitPeaks::DecideToEstimatePeakWidth
 * @param peak_index
 * @param peak_function
 * @return
 */
bool FitPeaks::decideToEstimatePeakWidth(
    size_t peak_index, API::IPeakFunction_sptr peak_function) {
  bool observe_peak_width(false);

  if (!m_initParamIndexes.empty()) {
    // user specifies starting value of peak parameters
    if (peak_index == 0) {
      // first peak.  using the user-specified value
      for (size_t i = 0; i < m_initParamIndexes.size(); ++i) {
        size_t param_index = m_initParamIndexes[i];
        double param_value = m_initParamValues[i];
        peak_function->setParameter(param_index, param_value);
      }
    } else {
      // using the fitted paramters from the previous fitting result
      // do noting
    }
  } else {
    // by observation
    observe_peak_width = true;
  }

  return observe_peak_width;
}

//----------------------------------------------------------------------------------------------
/** retrieve the fitted peak information from functions and set to output
 * vectors
 * @brief FitPeaks::processSinglePeakFitResult
 * @param wsindex
 * @param peakindex
 * @param cost
 * @param expected_peak_positions
 * @param fitfunction
 * @param fit_result
 */
void FitPeaks::processSinglePeakFitResult(
    size_t wsindex, size_t peakindex, const double cost,
    const std::vector<double> &expected_peak_positions,
    FitPeaksAlgorithm::FitFunction fitfunction,
    boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result) {
  // determine peak position tolerance
  double postol(DBL_MAX);
  bool case23(false);
  if (m_peakPosTolCase234) {
    // peak tolerance is not defined
    if (m_numPeaksToFit == 1) {
      // case (d) one peak only
      postol = m_inputMatrixWS->histogram(wsindex).x().back() -
               m_inputMatrixWS->histogram(wsindex).x().front();
    } else {
      // case b and c: more than 1 peaks without defined peak tolerance
      case23 = true;
    }
  } else {
    // user explicitly specified
    if (peakindex >= m_peakPosTolerances.size())
      throw std::runtime_error("Peak tolerance out of index");
    postol = m_peakPosTolerances[peakindex];
  }

  // get peak position and analyze the fitting is good or not by various
  // criteria
  double peak_pos = fitfunction.peakfunction->centre();
  bool good_fit(false);
  if ((cost < 0) || (cost >= DBL_MAX - 1.)) {
    // unphysical cost function value
    peak_pos = -4;
  } else if (fitfunction.peakfunction->height() < m_minPeakHeight) {
    // peak height is under minimum request
    peak_pos = -3;
  } else if (case23) {
    // case b and c to check peak position without defined peak tolerance
    std::pair<double, double> fitwindow = getPeakFitWindow(wsindex, peakindex);
    if (fitwindow.first < fitwindow.second) {
      // peak fit window is specified or calculated: use peak window as position
      // tolerance
      if (peak_pos < fitwindow.first || peak_pos > fitwindow.second) {
        // peak is out of fit window
        peak_pos = -2;
        g_log.debug() << "Peak position " << peak_pos << " is out of fit "
                      << "window boundary " << fitwindow.first << ", "
                      << fitwindow.second << "\n";
      } else
        good_fit = true;
    } else {
      // use the 1/2 distance to neiboring peak without defined peak window
      double left_bound(-1);
      if (peakindex > 0)
        left_bound = 0.5 * (expected_peak_positions[peakindex] -
                            expected_peak_positions[peakindex - 1]);
      double right_bound(-1);
      if (peakindex < m_numPeaksToFit - 1)
        right_bound = 0.5 * (expected_peak_positions[peakindex + 1] -
                             expected_peak_positions[peakindex]);
      if (left_bound < 0)
        left_bound = right_bound;
      if (right_bound < left_bound)
        right_bound = left_bound;
      if (left_bound < 0 || right_bound < 0)
        throw std::runtime_error("Code logic error such that left or right "
                                 "boundary of peak position is negative.");
      if (peak_pos < left_bound || peak_pos > right_bound)
        peak_pos = -2.5;
      else
        good_fit = true;
    }
  } else if (fabs(fitfunction.peakfunction->centre() -
                  expected_peak_positions[peakindex]) > postol) {
    // peak center is not within tolerance
    peak_pos = -5;
    g_log.debug() << "Peak position difference "
                  << fabs(fitfunction.peakfunction->centre() -
                          expected_peak_positions[peakindex])
                  << " is out of range of tolerance: " << postol << "\n";
  } else {
    // all criteria are passed
    good_fit = true;
  }

  // set cost function to DBL_MAX if fitting is bad
  double adjust_cost(cost);
  if (!good_fit) {
    // set the cost function value to DBL_MAX
    adjust_cost = DBL_MAX;
  }

  // reset cost
  if (adjust_cost > DBL_MAX - 1) {
    fitfunction.peakfunction->setIntensity(0);
  }

  // chi2
  fit_result->setRecord(peakindex, adjust_cost, peak_pos, fitfunction);

  return;
}

//----------------------------------------------------------------------------------------------
// TODO/NOW - Implement such that it can be parallelized
/** calculate fitted peaks with background in the output workspace
 * @brief FitPeaks::calculateFittedPeaks
 */
void FitPeaks::calculateFittedPeaks() {
  // check
  if (!m_fittedParamTable)
    throw std::runtime_error("No parameters");

  size_t num_peakfunc_params = m_peakFunction->nParams();
  size_t num_bkgdfunc_params = m_bkgdFunction->nParams();

  // TODO/LATER - Implement OpenMP parallelizatoin
  for (size_t iws = m_startWorkspaceIndex; iws <= m_stopWorkspaceIndex; ++iws) {
    // TODO/LATER - Parallelization macro shall be put here

    // get a copy of peak function and background function
    IPeakFunction_sptr peak_function =
        boost::dynamic_pointer_cast<IPeakFunction>(m_peakFunction->clone());
    IBackgroundFunction_sptr bkgd_function =
        boost::dynamic_pointer_cast<IBackgroundFunction>(
            m_bkgdFunction->clone());

    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      // get and set the peak function parameters
      size_t row_index =
          (iws - m_startWorkspaceIndex) * m_numPeaksToFit + ipeak;
      for (size_t ipar = 0; ipar < num_peakfunc_params; ++ipar) {
        double value_i = m_fittedParamTable->cell<double>(row_index, 2 + ipar);
        peak_function->setParameter(ipar, value_i);
      }

      // check whether the peak has a fit or not
      if (fabs(peak_function->height()) < 1.E-20)
        continue;

      // get and set the background function parameters
      for (size_t ipar = 0; ipar < num_bkgdfunc_params; ++ipar) {
        double value_i = m_fittedParamTable->cell<double>(
            row_index, 2 + num_peakfunc_params + ipar);
        bkgd_function->setParameter(ipar, value_i);
      }

      // use domain and function to calcualte
      // get the range of start and stop to construct a function domain
      auto vec_x = m_fittedPeakWS->x(iws);
      std::pair<double, double> peakwindow = getPeakFitWindow(iws, ipeak);
      std::vector<double>::const_iterator start_x_iter =
          std::lower_bound(vec_x.begin(), vec_x.end(), peakwindow.first);
      std::vector<double>::const_iterator stop_x_iter =
          std::lower_bound(vec_x.begin(), vec_x.end(), peakwindow.second);

      if (start_x_iter == stop_x_iter)
        throw std::runtime_error("Range size is zero");

      FunctionDomain1DVector domain(start_x_iter, stop_x_iter);
      FunctionValues values(domain);
      CompositeFunction_sptr comp_func =
          boost::make_shared<API::CompositeFunction>();
      comp_func->addFunction(peak_function);
      comp_func->addFunction(bkgd_function);
      comp_func->function(domain, values);

      // copy over the values
      size_t istart = static_cast<size_t>(start_x_iter - vec_x.begin());
      size_t istop = static_cast<size_t>(stop_x_iter - vec_x.begin());
      for (size_t yindex = istart; yindex < istop; ++yindex)
        m_fittedPeakWS->dataY(iws)[yindex] =
            values.getCalculated(yindex - istart);
    } // END-FOR (ipeak)
  }   // END-FOR (iws)

  return;
}

//----------------------------------------------------------------------------------------------
/**  Estimate background: There are two methods that will be tried.
 * First, algorithm FindPeakBackground will be tried;
 * If it fails, then a linear background estimator will be called.
 */
void FitPeaks::estimateBackground(const Histogram &histogram,
                                  const std::pair<double, double> &peak_window,
                                  API::IBackgroundFunction_sptr bkgd_function) {
  if (peak_window.first >= peak_window.second)
    throw std::runtime_error("Invalid peak window");

  // use the simple way to find linear background
  double bkgd_a0, bkgd_a1;
  this->estimateLinearBackground(histogram, peak_window.first,
                                 peak_window.second, bkgd_a0, bkgd_a1);

  // set result
  // FIXME - this is not flexible for background other than
  // flat/linear/quadratic
  bkgd_function->setParameter(0, bkgd_a0);
  if (bkgd_function->nParams() > 1)
    bkgd_function->setParameter(1, bkgd_a1);
  if (bkgd_function->nParams() > 2)
    bkgd_function->setParameter(2, 0.);

  return;
}

//----------------------------------------------------------------------------------------------
/**  Estimate peak profile's parameters values via observation
 * including
 * (1) peak center (2) peak intensity  (3) peak width depending on peak type
 * @brief FitPeaks::EstimatePeakParameters
 * @param dataws
 * @param wi
 * @param peak_window
 * @param peakfunction
 * @param bkgdfunction
 * @param observe_peak_width
 * @return
 */
int FitPeaks::estimatePeakParameters(
    API::MatrixWorkspace_sptr dataws, size_t wi,
    const std::pair<double, double> &peak_window,
    API::IPeakFunction_sptr peakfunction,
    API::IBackgroundFunction_sptr bkgdfunction, bool observe_peak_width) {
  // get the range of start and stop to construct a function domain
  auto vector_x = dataws->x(wi);
  std::vector<double>::const_iterator start_iter =
      std::lower_bound(vector_x.begin(), vector_x.end(), peak_window.first);
  std::vector<double>::const_iterator stop_iter =
      std::lower_bound(vector_x.begin(), vector_x.end(), peak_window.second);
  size_t start_index = static_cast<size_t>(start_iter - vector_x.begin());
  size_t stop_index = static_cast<size_t>(stop_iter - vector_x.begin());

  // calculate background
  if (start_index == stop_index)
    throw std::runtime_error("Range size is zero");

  FunctionDomain1DVector domain(start_iter, stop_iter);
  FunctionValues bkgd_values(domain);
  bkgdfunction->function(domain, bkgd_values);

  const auto vector_y = dataws->y(wi);

  // Estimate peak center
  double peak_center, peak_height;
  size_t peak_center_index;
  int result = observePeakCenter(vector_x, vector_y, bkgd_values, start_index,
                                 stop_index, peak_center, peak_center_index,
                                 peak_height);

  // set the peak center
  if (result == GOOD) {
    // use values from background to locate FWHM
    peakfunction->setCentre(peak_center);
    peakfunction->setHeight(peak_height);
  } else {
    g_log.debug() << "Observation result is NOT good but " << result << "\n";
  }

  // Estimate FHWM (peak width)
  if (result == GOOD && observe_peak_width &&
      m_peakWidthEstimateApproach != EstimatePeakWidth::NoEstimation) {
    // observe peak width
    double peak_width =
        observePeakWidth(vector_x, vector_y, bkgd_values, peak_height,
                         peak_center_index, start_index, stop_index);
    if (peak_width > 0) {
      peakfunction->setFwhm(peak_width);
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/// Guess/estimate peak center and thus height by observation
int FitPeaks::observePeakCenter(const HistogramX &vector_x,
                                const HistogramY &vector_y,
                                FunctionValues &bkgd_values, size_t start_index,
                                size_t stop_index, double &peak_center,
                                size_t &peak_center_index,
                                double &peak_height) {
  // initialize paramters
  double peak_bkgd_max = 0;
  peak_height = 0;
  peak_center_index = -1;
  peak_center = 0;

  // locate highest pure peakk position
  size_t num_pts = min(stop_index - start_index, bkgd_values.size());
  for (size_t i = 0; i < num_pts; ++i) {
    // get index in vector X and y; do the check
    size_t curr_index =
        i + start_index; // current index in full vector X. remember that
    if (curr_index > vector_x.size())
      throw std::logic_error(
          "It is not possible to go out of boundary of vector X");

    // get Y value, reduce by background and check against max Y value
    double y = vector_y[curr_index] - bkgd_values.getCalculated(i);
    if (y > peak_height) {
      peak_height = y;
      peak_center = vector_x[curr_index];
      peak_center_index = curr_index;
    }
    if (vector_y[curr_index] > peak_bkgd_max)
      peak_bkgd_max = vector_y[curr_index];
  }

  // check peak height and determine the case integer to return
  // any case other than GOOD will lead to a non-peak-fit
  const size_t MAGIC3(3);
  int result(0);
  if (peak_bkgd_max < 1.0E-10) {
    // none-event, but no signal within region
    result = NOSIGNAL;
  } else if (peak_height < m_minPeakHeight) {
    // peak too low
    result = LOWPEAK;
  } else if ((peak_center_index - start_index) < MAGIC3 ||
             (stop_index - peak_center_index) < MAGIC3) {
    // peak not at center
    result = OUTOFBOUND;
  } else {
    result = GOOD;
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::ObservePeakWidth
 * @param vector_x
 * @param vector_y
 * @param bkgd_values
 * @param peak_height
 * @param ipeak
 * @param istart
 * @param istop
 * @return peak width as double
 */
double FitPeaks::observePeakWidth(const HistogramX &vector_x,
                                  const HistogramY &vector_y,
                                  FunctionValues &bkgd_values,
                                  double peak_height, size_t ipeak,
                                  size_t istart, size_t istop) {
  double peak_width(-0.);

  if (m_peakWidthEstimateApproach == EstimatePeakWidth::InstrumentResolution) {
    // width from guessing from delta(D)/D
    double peak_center = vector_x[ipeak];
    peak_width = peak_center * m_peakDSpacePercentage;
  } else if (m_peakWidthEstimateApproach == EstimatePeakWidth::Observation) {
    // observe peak width by estimating FWHM from observation
    // the approach is to locate the first bin with intensity larger than half
    // peak height from left and right
    // check input
    if (istart >= ipeak || ipeak >= istop)
      throw std::runtime_error(
          "Input error for index of peak, left and right side of fit window");
    if (peak_height <= 0.)
      throw std::runtime_error(
          "It is not possible to have sub zero peak height region");

    // search from the left half maximum
    size_t ileft_max = istart;
    size_t num_pts = min(istop - istart, bkgd_values.size());
    for (size_t i = 0; i <= ipeak; ++i) {
      // get index in vector X and y; do the check
      size_t curr_index =
          i + istart; // current index in full vector X. remember that
      if (curr_index > vector_x.size())
        throw std::logic_error(
            "It is not possible to go out of boundary of vector X");

      // get Y value, reduce by background and check against max Y value
      double y = vector_y[curr_index] - bkgd_values.getCalculated(i);
      if (y >= 0.5 * peak_height) {
        ileft_max = curr_index;
        break;
      } else if (curr_index == ipeak)
        throw std::runtime_error("Found peak center point is the only data "
                                 "point with intensity larger than half "
                                 "maximum");
    }

    // search for the right half maximum
    size_t iright_max = istop;
    for (size_t i = num_pts - 1; i >= ipeak - istart; --i) {
      size_t curr_index =
          i + istart; // current index in full vector X. remember that
      if (curr_index > vector_x.size())
        throw std::logic_error(
            "It is not possible to go out of boundary of vector X");

      // get Y value, reduce by background and check against max Y value
      double y = vector_y[curr_index] - bkgd_values.getCalculated(i);
      // first data point larger than half height from right
      if (y >= 0.5 * peak_height) {
        iright_max = curr_index;
        break;
      } else if (curr_index == ipeak)
        throw std::runtime_error("Found peak center point is the only data "
                                 "point with intensity larger than half "
                                 "maximum");
    }

    peak_width = vector_x[iright_max] - vector_x[ileft_max];
  } else {
    // get from last peak or from input!
    throw std::runtime_error(
        "This case for obsering peak width is not supported.");
  }

  return peak_width;
}

//----------------------------------------------------------------------------------------------
/** Fit background function
 * @brief FitPeaks::FitBackground
 * @param ws_index
 * @param fit_window
 * @param expected_peak_pos
 * @param bkgd_func
 * @return
 */
bool FitPeaks::fitBackground(const size_t &ws_index,
                             const std::pair<double, double> &fit_window,
                             const double &expected_peak_pos,
                             API::IBackgroundFunction_sptr bkgd_func) {

  // find out how to fit background
  const auto &points = m_inputMatrixWS->histogram(ws_index).points();
  size_t start_index = findXIndex(points.rawData(), fit_window.first);
  size_t stop_index = findXIndex(points.rawData(), fit_window.second);
  size_t expected_peak_index = findXIndex(points.rawData(), expected_peak_pos);

  // treat 5 as a magic number - TODO explain why
  bool good_fit(false);
  if (expected_peak_index - start_index > 10 && // TODO explain why 10
      stop_index - expected_peak_index - stop_index > 10) {
    // enough data points left for multi-domain fitting
    // set a smaller fit window
    std::vector<double> vec_min(2);
    std::vector<double> vec_max(2);

    vec_min[0] = fit_window.first;
    vec_max[0] = points[expected_peak_index - 5];
    vec_min[1] = points[expected_peak_index + 5];
    vec_max[1] = fit_window.second;

    // reset background function value
    bkgd_func->setParameter(0, 0.);
    if (bkgd_func->nParams() > 1)
      bkgd_func->setParameter(1, 0.);

    double chi2 =
        fitFunctionMD(bkgd_func, m_inputMatrixWS, ws_index, vec_min, vec_max);

    // process
    if (chi2 < DBL_MAX - 1) {
      good_fit = true;
    }

  } else {
    // fit as a single domain function.  check whether the result is good or bad

    // TODO FROM HERE!
    g_log.error("Don't know what to do with background fitting with single "
                "domain function!");
  }

  return good_fit;
}

//----------------------------------------------------------------------------------------------
/** Fit an individual peak
 * @brief FitPeaks::FitIndividualPeak
 * @param wi
 * @param fitter
 * @param expected_peak_center
 * @param fitwindow
 * @param high
 * @param high_background_function
 * @param observe_peak_width:: flag to estimate peak width (by observation) or
 * not
 * @param peakfunction
 * @param bkgdfunc
 * @return
 */
double FitPeaks::fitIndividualPeak(
    size_t wi, API::IAlgorithm_sptr fitter, const double expected_peak_center,
    const std::pair<double, double> &fitwindow, const bool high,
    API::IBackgroundFunction_sptr high_background_function,
    const bool observe_peak_width, API::IPeakFunction_sptr peakfunction,
    API::IBackgroundFunction_sptr bkgdfunc) {
  double cost(DBL_MAX);

  if (high) {
    // fit peak with high background!
    cost = fitFunctionHighBackground(
        fitter, fitwindow, wi, expected_peak_center, observe_peak_width,
        peakfunction, bkgdfunc, high_background_function);
  } else {
    // fit peak and background
    cost = fitFunctionSD(fitter, peakfunction, bkgdfunc, m_inputMatrixWS, wi,
                         fitwindow.first, fitwindow.second,
                         expected_peak_center, observe_peak_width, true);
  }

  return cost;
}

//----------------------------------------------------------------------------------------------
/** Fit function in single domain (mostly applied for fitting peak + background)
 * with estimating peak parameters
 * This is the core fitting algorithm to deal with the simplest situation
 * @exception :: Fit.isExecuted is false (cannot be executed)
 * @brief FitPeaks::FitFunctionSD
 * @param fit
 * @param peak_function
 * @param bkgd_function
 * @param dataws
 * @param wsindex
 * @param xmin
 * @param xmax
 * @param expected_peak_center
 * @param observe_peak_width
 * @param estimate_background
 * @return
 */
double FitPeaks::fitFunctionSD(IAlgorithm_sptr fit,
                               API::IPeakFunction_sptr peak_function,
                               API::IBackgroundFunction_sptr bkgd_function,
                               API::MatrixWorkspace_sptr dataws, size_t wsindex,
                               double xmin, double xmax,
                               const double &expected_peak_center,
                               bool observe_peak_width,
                               bool estimate_background) {

  // generate peak window
  std::pair<double, double> peak_window = std::make_pair(xmin, xmax);

  // Estimate background
  if (estimate_background) {
    const auto &histogram = dataws->histogram(wsindex);
    estimateBackground(histogram, peak_window, bkgd_function);
  } else {
    for (size_t n = 0; n < bkgd_function->nParams(); ++n)
      bkgd_function->setParameter(n, 0);
  }

  // Estimate peak profile parameter
  int result =
      estimatePeakParameters(dataws, wsindex, peak_window, peak_function,
                             bkgd_function, observe_peak_width);
  if (result != GOOD)
    peak_function->setCentre(expected_peak_center);

  // Create the composition function
  CompositeFunction_sptr comp_func =
      boost::make_shared<API::CompositeFunction>();
  comp_func->addFunction(peak_function);
  comp_func->addFunction(bkgd_function);
  IFunction_sptr fitfunc = boost::dynamic_pointer_cast<IFunction>(comp_func);

  // Set the properties
  fit->setProperty("Function", fitfunc);
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("MaxIterations", 50); // magic number
  fit->setProperty("StartX", xmin);
  fit->setProperty("EndX", xmax);

  if (m_constrainPeaksPosition) {
    // set up a constraint on peak position
    double peak_center = peak_function->centre();
    double peak_width = peak_function->fwhm();
    std::stringstream peak_center_constraint;
    peak_center_constraint << (peak_center - 0.5 * peak_width) << " < f0."
                           << peak_function->getCentreParameterName() << " < "
                           << (peak_center + 0.5 * peak_width);

    // set up a constraint on peak height

    fit->setProperty("Constraints", peak_center_constraint.str());
  }

  // Execute fit and get result of fitting background
  // m_sstream << "FitSingleDomain: " << fit->asString() << ".\n";
  g_log.debug() << "[E1201] FitSingleDomain Before fitting, Fit function: "
                << fit->asString() << "\n";

  fit->executeAsChildAlg();

  g_log.debug() << "[E1202] FitSingleDomain After fitting, Fit function: "
                << fit->asString() << "\n";

  if (!fit->isExecuted()) {
    g_log.error("Fitting peak SD (single domain) failed to execute.");
    throw std::runtime_error(
        "Fitting peak SD (single domain) failed to execute.");
  }

  // Retrieve result
  std::string fitStatus = fit->getProperty("OutputStatus");
  double chi2 = DBL_MAX;
  if (fitStatus == "success") {
    chi2 = fit->getProperty("OutputChi2overDoF");
    fitfunc = fit->getProperty("Function");
  }

  return chi2;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::FitFunctionMD
 * @param fit_function :: function to fit
 * @param dataws :: matrix workspace to fit with
 * @param wsindex ::  workspace index of the spectrum in matrix workspace
 * @param vec_xmin :: minimin values of domains
 * @param vec_xmax :: maximim values of domains
 * @return
 */
double FitPeaks::fitFunctionMD(API::IFunction_sptr fit_function,
                               API::MatrixWorkspace_sptr dataws, size_t wsindex,
                               std::vector<double> &vec_xmin,
                               std::vector<double> &vec_xmax) {
  // Validate
  if (vec_xmin.size() != vec_xmax.size())
    throw runtime_error("Sizes of xmin and xmax (vectors) are not equal. ");

  // Note: after testing it is found that multi-domain Fit cannot be reused
  API::IAlgorithm_sptr fit;
  try {
    fit = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    std::stringstream errss;
    errss << "The FitPeak algorithm requires the CurveFitting library";
    throw std::runtime_error(errss.str());
  }
  // set up background fit instance
  fit->setProperty("Minimizer", m_minimizer);
  fit->setProperty("CostFunction", m_costFunction);
  fit->setProperty("CalcErrors", true);

  // This use multi-domain; but does not know how to set up IFunction_sptr
  // fitfunc,
  boost::shared_ptr<MultiDomainFunction> md_function =
      boost::make_shared<MultiDomainFunction>();

  // Set function first
  md_function->addFunction(fit_function);

  //  set domain for function with index 0 covering both sides
  md_function->clearDomainIndices();
  std::vector<size_t> ii(2);
  ii[0] = 0;
  ii[1] = 1;
  md_function->setDomainIndices(0, ii);

  // Set the properties
  fit->setProperty("Function",
                   boost::dynamic_pointer_cast<IFunction>(md_function));
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("StartX", vec_xmin[0]);
  fit->setProperty("EndX", vec_xmax[0]);
  fit->setProperty("InputWorkspace_1", dataws);
  fit->setProperty("WorkspaceIndex_1", static_cast<int>(wsindex));
  fit->setProperty("StartX_1", vec_xmin[1]);
  fit->setProperty("EndX_1", vec_xmax[1]);
  fit->setProperty("MaxIterations", 50);

  // Execute
  fit->execute();
  if (!fit->isExecuted()) {
    throw runtime_error("Fit is not executed on multi-domain function/data. ");
  }

  // Retrieve result
  std::string fitStatus = fit->getProperty("OutputStatus");

  double chi2 = DBL_MAX;
  if (fitStatus == "success") {
    chi2 = fit->getProperty("OutputChi2overDoF");
  }

  return chi2;
}

//----------------------------------------------------------------------------------------------
/** Fit peak with high background
 * @brief FitPeaks::FitFunctionHighBackground
 * @param fit
 * @param fit_window
 * @param ws_index
 * @param expected_peak_center
 * @param observe_peak_width
 * @param peakfunction
 * @param bkgdfunc
 * @param high_bkgd_function
 * @return
 */
double FitPeaks::fitFunctionHighBackground(
    IAlgorithm_sptr fit, const std::pair<double, double> &fit_window,
    const size_t &ws_index, const double &expected_peak_center,
    bool observe_peak_width, API::IPeakFunction_sptr peakfunction,
    API::IBackgroundFunction_sptr bkgdfunc,
    API::IBackgroundFunction_sptr high_bkgd_function) {
  // Fit the background first if there is enough data points
  fitBackground(ws_index, fit_window, expected_peak_center, high_bkgd_function);

  // Get partial of the data
  std::vector<double> vec_x, vec_y, vec_e;
  getRangeData(ws_index, fit_window, vec_x, vec_y, vec_e);

  // Reduce the background
  reduceBackground(high_bkgd_function, vec_x, vec_y);

  // Create a new workspace
  API::MatrixWorkspace_sptr reduced_bkgd_ws =
      createMatrixWorkspace(vec_x, vec_y, vec_e);

  // Fit peak with background
  double cost = fitFunctionSD(fit, peakfunction, bkgdfunc, reduced_bkgd_ws, 0,
                              vec_x.front(), vec_x.back(), expected_peak_center,
                              observe_peak_width, false);

  // add the reduced background back
  bkgdfunc->setParameter(0, bkgdfunc->getParameter(0) +
                                high_bkgd_function->getParameter(0));
  bkgdfunc->setParameter(1, bkgdfunc->getParameter(1) +
                                high_bkgd_function->getParameter(1));

  return cost;
}

//----------------------------------------------------------------------------------------------
/** Create a single spectrum workspace for fitting
 * @brief FitPeaks::CreateMatrixWorkspace
 * @param vec_x
 * @param vec_y
 * @param vec_e
 * @return
 */
API::MatrixWorkspace_sptr
FitPeaks::createMatrixWorkspace(const std::vector<double> &vec_x,
                                const std::vector<double> &vec_y,
                                const std::vector<double> &vec_e) {
  size_t size = vec_x.size();
  size_t ysize = vec_y.size();

  MatrixWorkspace_sptr matrix_ws =
      WorkspaceFactory::Instance().create("Workspace2D", 1, size, ysize);

  auto &dataX = matrix_ws->mutableX(0);
  auto &dataY = matrix_ws->mutableY(0);
  auto &dataE = matrix_ws->mutableE(0);

  dataX.assign(vec_x.cbegin(), vec_x.cend());
  dataY.assign(vec_y.cbegin(), vec_y.cend());
  dataE.assign(vec_e.cbegin(), vec_e.cend());

  return matrix_ws;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::generateOutputWorkspaces
 */
void FitPeaks::generateOutputPeakPositionWS() {
  // create output workspace for peak positions: can be partial spectra to input
  // workspace
  size_t num_hist = m_stopWorkspaceIndex - m_startWorkspaceIndex + 1;
  m_outputPeakPositionWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", num_hist, m_numPeaksToFit, m_numPeaksToFit);
  // set default
  for (size_t wi = 0; wi < num_hist; ++wi) {
    // TODO - Parallization OpenMP

    // convert to workspace index of input data workspace
    size_t inp_wi = wi + m_startWorkspaceIndex;
    std::vector<double> expected_position = getExpectedPeakPositions(inp_wi);
    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      m_outputPeakPositionWorkspace->dataX(wi)[ipeak] =
          expected_position[ipeak];
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::GenerateFittedParametersValueWorkspace
 */
void FitPeaks::generateFittedParametersValueWorkspace() {
  // peak parameter workspace
  std::string param_table_name =
      getPropertyValue("OutputPeakParametersWorkspace");

  // check whether it is not asked to create such table workspace
  if (param_table_name.size() == 0) {
    // Skip if it is not specified
    m_fittedParamTable = nullptr;
    return;
  }

  // create
  m_fittedParamTable =
      WorkspaceFactory::Instance().createTable("TableWorkspace");
  // add columns
  m_fittedParamTable->addColumn("int", "wsindex");
  m_fittedParamTable->addColumn("int", "peakindex");
  for (size_t iparam = 0; iparam < m_peakFunction->nParams(); ++iparam)
    m_fittedParamTable->addColumn("double",
                                  m_peakFunction->parameterName(iparam));
  for (size_t iparam = 0; iparam < m_bkgdFunction->nParams(); ++iparam)
    m_fittedParamTable->addColumn("double",
                                  m_bkgdFunction->parameterName(iparam));
  m_fittedParamTable->addColumn("double", "chi2");

  // add rows
  for (size_t iws = m_startWorkspaceIndex; iws <= m_stopWorkspaceIndex; ++iws) {
    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      size_t row_index = m_fittedParamTable->rowCount();
      m_fittedParamTable->appendRow();
      m_fittedParamTable->cell<int>(row_index, static_cast<size_t>(0)) =
          static_cast<int>(iws);
      m_fittedParamTable->cell<int>(row_index, 1) = static_cast<int>(ipeak);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate the output MatrixWorkspace for calculated peaks
 * @brief FitPeaks::GenerateCalculatedPeaksWS
 */
void FitPeaks::generateCalculatedPeaksWS() {
  // matrix workspace contained calculated peaks from fitting
  std::string fit_ws_name = getPropertyValue("FittedPeaksWorkspace");
  if (fit_ws_name.size() == 0) {
    // skip if user does not specify
    m_fittedPeakWS = nullptr;
    return;
  }

  // create
  m_fittedPeakWS = API::WorkspaceFactory::Instance().create(m_inputMatrixWS);
  for (size_t iws = 0; iws < m_fittedPeakWS->getNumberHistograms(); ++iws) {
    auto out_vecx = m_fittedPeakWS->histogram(iws).x();
    auto in_vecx = m_inputMatrixWS->histogram(iws).x();
    for (size_t j = 0; j < out_vecx.size(); ++j) {
      m_fittedPeakWS->dataX(iws)[j] = in_vecx[j];
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** set up output workspaces
 * @brief FitPeaks::setOutputProperties
 */
void FitPeaks::processOutputs() {
  setProperty("OutputWorkspace", m_outputPeakPositionWorkspace);

  // optional
  if (m_fittedParamTable)
    setProperty("OutputPeakParametersWorkspace", m_fittedParamTable);

  // optional
  if (m_fittedPeakWS && m_fittedParamTable) {
    g_log.debug("about to calcualte fitted peaks");
    calculateFittedPeaks();
    setProperty("FittedPeaksWorkspace", m_fittedPeakWS);
  }
}

//----------------------------------------------------------------------------------------------
/** Get the expected peak's position
 * @brief FitPeaks::getExpectedPeakPositions
 * @param wi
 * @return
 */
std::vector<double> FitPeaks::getExpectedPeakPositions(size_t wi) {
  // check
  if (wi < m_startWorkspaceIndex || wi > m_stopWorkspaceIndex) {
    std::stringstream errss;
    errss << "Workspace index " << wi << " is out of range ["
          << m_startWorkspaceIndex << ", " << m_stopWorkspaceIndex << "]";
    throw std::runtime_error(errss.str());
  }

  // initialize output array
  std::vector<double> exp_centers(m_numPeaksToFit);

  if (m_uniformPeakPositions) {
    // uniform peak centers among spectra: simple copy
    exp_centers = m_peakCenters;
  } else {
    // no uniform peak center.  locate the input workspace index
    // in the peak center workspace peak in the workspae

    // get the relative workspace index in input peak position workspace
    size_t peak_wi = wi - m_startWorkspaceIndex;
    // get values
    exp_centers = m_peakCenterWorkspace->y(peak_wi).rawData();
  }

  return exp_centers;
}

//----------------------------------------------------------------------------------------------
/** get the peak fit window
 * @brief FitPeaks::getPeakFitWindow
 * @param wi
 * @param ipeak :: index of peak
 * @return
 */
std::pair<double, double> FitPeaks::getPeakFitWindow(size_t wi, size_t ipeak) {
  // check workspace index
  if (wi < m_startWorkspaceIndex || wi > m_stopWorkspaceIndex) {
    std::stringstream errss;
    errss << "Workspace index " << wi << " is out of range ["
          << m_startWorkspaceIndex << ", " << m_stopWorkspaceIndex << "]";
    throw std::runtime_error(errss.str());
  }

  // check peak index
  if (ipeak >= m_numPeaksToFit) {
    std::stringstream errss;
    errss << "Peak index " << ipeak << " is out of range (" << m_numPeaksToFit
          << ")";
    throw std::runtime_error(errss.str());
  }

  double left(0), right(0);
  if (m_calculateWindowInstrument) {
    // calcualte peak window by delta(d)/d
    double peak_pos = getExpectedPeakPositions(wi)[ipeak];
    // calcalate expected peak width
    double estimate_peak_width = peak_pos * m_peakDSpacePercentage;
    // using a MAGIC number to estimate the peak window
    double MAGIC = 3.0;
    left = peak_pos - estimate_peak_width * MAGIC;
    right = peak_pos + estimate_peak_width * MAGIC;
  } else if (m_uniformPeakWindows) {
    // uniform peak fit window
    assert(m_peakWindowVector.size() > 0); // peak fit window must be given!

    left = m_peakWindowVector[ipeak][0];
    right = m_peakWindowVector[ipeak][1];
  } else if (m_peakWindowWorkspace) {
    // no uniform peak fit window.  locate peak in the workspace
    // get workspace index in m_peakWindowWorkspace
    size_t window_wi = wi - m_startWorkspaceIndex;

    left = m_peakWindowWorkspace->y(window_wi)[ipeak * 2];
    right = m_peakWindowWorkspace->y(window_wi)[ipeak * 2 + 1];
  } else {
    throw std::runtime_error("Unhandled case for get peak fit window!");
  }

  return std::make_pair(left, right);
}

//----------------------------------------------------------------------------------------------
/** get vector X, Y and E in a given range
 * @brief FitPeaks::GetRangeData
 * @param iws
 * @param fit_window
 * @param vec_x
 * @param vec_y
 * @param vec_e
 */
void FitPeaks::getRangeData(size_t iws,
                            const std::pair<double, double> &fit_window,
                            std::vector<double> &vec_x,
                            std::vector<double> &vec_y,
                            std::vector<double> &vec_e) {

  // get the original vector of X and determine the start and end index
  const vector<double> orig_x = m_inputMatrixWS->histogram(iws).x().rawData();
  size_t left_index =
      std::lower_bound(orig_x.begin(), orig_x.end(), fit_window.first) -
      orig_x.begin();
  size_t right_index =
      std::lower_bound(orig_x.begin(), orig_x.end(), fit_window.second) -
      orig_x.begin();
  if (left_index >= right_index) {
    std::stringstream err_ss;
    err_ss << "Unable to get subset of histogram from given fit window. "
           << "Fit window: " << fit_window.first << ", " << fit_window.second
           << ". Vector X's range is " << orig_x.front() << ", "
           << orig_x.back();
    throw std::runtime_error(err_ss.str());
  }

  // copy X, Y and E
  size_t num_elements = right_index - left_index;
  vec_x.resize(num_elements);
  std::copy(orig_x.begin() + left_index, orig_x.begin() + right_index,
            vec_x.begin());

  // modify right_index if it is at the end
  if (m_inputMatrixWS->isHistogramData() && right_index == orig_x.size() - 1) {
    right_index -= 1;
    if (right_index == left_index)
      throw std::runtime_error("Histogram workspace have same left and right "
                               "boundary index for Y and E.");
    num_elements -= 1;
  }

  // get original vector of Y and E
  const std::vector<double> orig_y =
      m_inputMatrixWS->histogram(iws).y().rawData();
  const std::vector<double> orig_e =
      m_inputMatrixWS->histogram(iws).e().rawData();
  vec_y.resize(num_elements);
  vec_e.resize(num_elements);
  std::copy(orig_y.begin() + left_index, orig_y.begin() + right_index,
            vec_y.begin());
  std::copy(orig_e.begin() + left_index, orig_e.begin() + right_index,
            vec_e.begin());

  return;
}

//----------------------------------------------------------------------------------------------
// find 2 local minima: draw a line as background to reduce
// find 1 local minima: a flat background
void FitPeaks::reduceBackground(API::IBackgroundFunction_sptr bkgd_func,
                                const std::vector<double> &vec_x,
                                std::vector<double> &vec_y) {

  // find out all local minima
  std::vector<size_t> local_min_indices;
  if ((vec_y)[0] <= (vec_y)[1])
    local_min_indices.push_back(0);
  for (size_t i = 1; i < vec_y.size() - 1; ++i) {
    if ((vec_y)[i] <= (vec_y)[i - 1] && (vec_y)[i] <= (vec_y)[i + 1])
      local_min_indices.push_back(i);
  }
  size_t lastindex = vec_y.size() - 1;
  if ((vec_y)[lastindex] <= (vec_y)[lastindex - 1])
    local_min_indices.push_back(lastindex);

  if (local_min_indices.size() == 0)
    throw std::runtime_error(
        "It is not possible to have less than 0 local minimum for a peak");

  FunctionDomain1DVector vectorx(vec_x.begin(), vec_x.end());
  FunctionValues vector_bkgd(vectorx);
  bkgd_func->function(vectorx, vector_bkgd);

  // Reduce the background from the calculated background
  for (size_t i = 0; i < vec_y.size(); ++i) {
    (vec_y)[i] -= vector_bkgd[i];
    // it is better not to mess up with E here
  }

  return;
}

void FitPeaks::estimateLinearBackground(const Histogram &histogram,
                                        double left_window_boundary,
                                        double right_window_boundary,
                                        double &bkgd_a0, double &bkgd_a1) {
  const auto &vecX = histogram.points();
  size_t istart = findXIndex(vecX.rawData(), left_window_boundary);
  size_t istop = findXIndex(vecX.rawData(), right_window_boundary);

  double bg2, chisq;
  // TODO explain why 3 is the right number
  HistogramData::estimateBackground(1, histogram, istart, istop, istart + 3,
                                    istop - 3, bkgd_a0, bkgd_a1, bg2, chisq);
}

//----------------------------------------------------------------------------------------------
/**  Write result of peak fit per spectrum to output analysis workspaces
 * @brief FitPeaks::writeFitResult
 * @param wi
 * @param expected_positions
 * @param fit_result
 * @param noevents
 */
void FitPeaks::writeFitResult(
    size_t wi, const std::vector<double> &expected_positions,
    boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result,
    bool noevents) {

  // convert to
  size_t out_wi = wi - m_startWorkspaceIndex;
  if (out_wi >= m_outputPeakPositionWorkspace->getNumberHistograms()) {
    g_log.error() << "workspace index " << wi
                  << " is out of output peak position workspace "
                  << "range of spectra, which contains "
                  << m_outputPeakPositionWorkspace->getNumberHistograms()
                  << " spectra"
                  << "\n";
    throw std::runtime_error(
        "Out of boundary to set output peak position workspace");
  }

  // Fill the output peak position workspace
  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
    double exp_peak_pos(expected_positions[ipeak]);
    double fitted_peak_pos(-1); // default for no event or no signal
    double peak_chi2(-1E20);    // use negative number for NO fit
    if (!noevents) {
      fitted_peak_pos =
          fit_result->getPeakPosition(ipeak); // fitted_positions[ipeak];
      peak_chi2 = fit_result->getCost(ipeak); // peak_chi2_vec[ipeak];
    }

    m_outputPeakPositionWorkspace->mutableX(out_wi)[ipeak] = exp_peak_pos;
    m_outputPeakPositionWorkspace->mutableY(out_wi)[ipeak] = fitted_peak_pos;
    m_outputPeakPositionWorkspace->mutableE(out_wi)[ipeak] = peak_chi2;
  }

  // return if it is not asked to write fitted peak parameters
  if (!m_fittedParamTable)
    return;

  // Output the peak parameters to the table workspace
  // check vector size

  // last column of the table is for chi2
  size_t chi2_index = m_fittedParamTable->columnCount() - 1;
  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
    // get row number
    size_t row_index = out_wi * m_numPeaksToFit + ipeak;
    // check again with the column size versus peak parameter values
    if (fit_result->getNumberParameters() !=
        m_fittedParamTable->columnCount() - 3) {
      g_log.error() << "Peak " << ipeak << " has "
                    << fit_result->getNumberParameters()
                    << " parameters.  Parameter table shall have 3 more "
                       "columns.  But not it has "
                    << m_fittedParamTable->columnCount() << " columns\n";
      throw std::runtime_error(
          "Peak parameter vector for one peak has different sizes to output "
          "table workspace");
    }

    if (noevents) {
      // no signals: just pass
      ;
    } else {
      // case for fit peak with signals
      for (size_t iparam = 0;
           iparam <
               fit_result
                   ->getNumberParameters(); // peak_parameters[ipeak].size();
           ++iparam) {
        size_t col_index = iparam + 2;
        if (col_index >= m_fittedParamTable->columnCount()) {
          stringstream err_ss;
          err_ss << "Try to access FittedParamTable's " << col_index
                 << "-th column, which is out of range [0, "
                 << m_fittedParamTable->columnCount() << ")";
          const std::vector<std::string> &col_names =
              m_fittedParamTable->getColumnNames();
          for (const auto &name : col_names)
            err_ss << name << "  ";
          throw std::runtime_error(err_ss.str());
        }
        m_fittedParamTable->cell<double>(row_index, col_index) =
            fit_result->getParameterValue(ipeak, iparam);
      } // end for (iparam)
      // set chi2
      m_fittedParamTable->cell<double>(row_index, chi2_index) =
          fit_result->getCost(ipeak);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::getPeakHeightParameterName
 * @param peak_function
 * @return
 */
std::string FitPeaks::getPeakHeightParameterName(
    API::IPeakFunction_const_sptr peak_function) {
  std::string height_name("");

  std::vector<std::string> peak_parameters = peak_function->getParameterNames();
  for (const auto &name : peak_parameters) {
    if (name == "Height") {
      height_name = "Height";
      break;
    } else if (name == "I") {
      height_name = "I";
      break;
    } else if (name == "Intensity") {
      height_name = "Intensity";
      break;
    }
  }

  if (height_name.empty())
    throw std::runtime_error("Peak height parameter name cannot be found.");

  return height_name;
}

DECLARE_ALGORITHM(FitPeaks)

} // namespace Algorithms
} // namespace Mantid
