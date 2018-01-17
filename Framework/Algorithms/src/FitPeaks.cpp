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
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;

using namespace std;

const size_t MIN_EVENTS = 100;

namespace Mantid {
namespace Algorithms {

//----------------------------------------------------------------------------------------------
/** Get an index of a value in a sorted vector.  The index should be the item
 * with value nearest to X
  */
size_t findXIndex(const HistogramX &vecx, double x) {
  size_t index;
  if (x <= vecx.front()) {
    index = 0;
  } else if (x >= vecx.back()) {
    index = vecx.size() - 1;
  } else {
    vector<double>::const_iterator fiter;
    fiter = lower_bound(vecx.begin(), vecx.end(), x);
    index = static_cast<size_t>(fiter - vecx.begin());
    if (index == 0)
      throw runtime_error("It seems impossible to have this value. ");
    if (x - vecx[index - 1] < vecx[index] - x)
      --index;
  }

  return index;
}

enum { NOSIGNAL, LOWPEAK, OUTOFBOUND, GOOD };

//----------------------------------------------------------------------------------------------
/** constructor
 * @brief FitPeaks::FitPeaks
 */
FitPeaks::FitPeaks()
    : fit_peaks_from_right_(true), m_numPeaksToFit(0), m_minPeakHeight(20.),
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

  std::string helpgrp("Additional Information");

  setPropertyGroup("EventNumberWorkspace", helpgrp);

  //  declareProperty(Kernel::make_unique<ArrayProperty<double>>("PeakRanges"),
  //                  "List of double for each peak's range.");

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
  GenerateOutputPeakPositionWS();
  GenerateFittedParametersValueWorkspace();
  GenerateCalculatedPeaksWS();

  // fit peaks
  fitPeaks();

  // set the output workspaces to properites
  ProcessOutputs();
}

//----------------------------------------------------------------------------------------------
/** process inputs
 * @brief FitPeaks::processInputs
 */
void FitPeaks::processInputs() {
  // input workspaces
  m_inputMatrixWS = getProperty("InputWorkspace");
  std::string event_ws_name = getPropertyValue("EventNumberWorkspace");
  if (event_ws_name.size() > 0)
    m_eventNumberWS = getProperty("EventNumberWorkspace");
  else
    m_eventNumberWS = 0;
  if (m_inputMatrixWS->getAxis(0)->unit()->unitID() == "dSpacing")
    is_d_space_ = true;
  else
    is_d_space_ = false;

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

  g_log.notice() << "[DB] Process inputs [2] Start/Stop ws index = "
                 << m_startWorkspaceIndex << ", " << m_stopWorkspaceIndex
                 << "\n";

  // Set up peak and background functions
  processInputFunctions();
  g_log.notice() << "[DB] Process inputs [3] peak type: "
                 << m_peakFunction->name()
                 << ", background type: " << m_bkgdFunction->name() << "\n";

  // optimizer, cost function and fitting scheme
  m_minimizer = getPropertyValue("Minimizer");
  m_costFunction = getPropertyValue("CostFunction");
  fit_peaks_from_right_ = getProperty("FitFromRight");

  // Peak centers, tolerance and fitting range
  ProcessInputPeakCenters();
  // check
  if (m_numPeaksToFit == 0)
    throw std::runtime_error("number of peaks to fit is zero.");
  // about how to estimate the peak width
  m_peakDSpacePercentage = getProperty("PeakWidthPercent");
  if (isEmpty(m_peakDSpacePercentage))
    m_peakDSpacePercentage = -1;
  else if (m_peakDSpacePercentage < 0)
    throw std::invalid_argument(
        "Peak D-spacing percentage cannot be negative!");
  g_log.notice() << "[DB] DeltaD/D = " << m_peakDSpacePercentage << "\n";
  ProcessInputPeakTolerance();
  processInputFitRanges();

  // set up background
  m_highBackground = getProperty("HighBackground");
  m_bkgdSimga = getProperty("FindBackgroundSigma");

  // about peak width and other peak parameter estimating method
  observe_peak_width_ = false;
  g_log.notice() << "[DB...BAT] peak function name: " << m_peakFunction->name()
                 << "\n";
  if (m_peakFunction->name().compare("Gaussian") == 0) {
    if (!(is_d_space_ && m_peakDSpacePercentage > 0))
      observe_peak_width_ = true;
  }
  g_log.notice("[DB] Process inputs [OVER]");

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
  if (bkgdfunctiontype.compare("Linear") == 0)
    bkgdname = "LinearBackground";
  else if (bkgdfunctiontype.compare("Flat") == 0)
    bkgdname = "FlatBackground";
  else
    bkgdname = bkgdfunctiontype;
  m_bkgdFunction = boost::dynamic_pointer_cast<IBackgroundFunction>(
      API::FunctionFactory::Instance().createFunction(bkgdname));

  // input peak parameters
  std::string partablename = getPropertyValue("PeakParameterValueTable");
  m_peakParamNames = getProperty("PeakParameterNames");
  if (partablename.size() == 0 && m_peakParamNames.size() > 0) {
    // use uniform starting value of peak parameters
    m_initParamValues = getProperty("PeakParameterValues");
    // check whether given parameter names and initial values match
    if (m_peakParamNames.size() != m_initParamValues.size())
      throw std::invalid_argument("PeakParameterNames and PeakParameterValues "
                                  "have different number of items.");
    // convert the parameter name in string to parameter name in integer index
    ConvertParametersNameToIndex();
    // set the flag
    m_uniformProfileStartingValue = true;
  } else if (partablename.size() > 0 && m_peakParamNames.size() == 0) {
    // use non-uniform starting value of peak parameters
    m_uniformProfileStartingValue = false;
    m_profileStartingValueTable = getProperty(partablename);
  } else if (partablename.size() > 0 && m_peakParamNames.size() > 0) {
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
  calculate_window_instrument_ = false;

  if (peakwindow.size() > 0 && peakwindowname.size() == 0) {
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
  } else if (peakwindow.size() == 0 && peakwindowname.size() > 0) {
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
  } else if (peakwindow.size() == 0) {
    // no peak window is defined, then the peak window will be estimated by
    // delta(D)/D
    if (is_d_space_ && m_peakDSpacePercentage > 0)
      calculate_window_instrument_ = true;
    else
      throw std::invalid_argument("Without definition of peak window, the "
                                  "input workspace must be in unit of dSpacing "
                                  "and Delta(D)/D must be given!");

  } else {
    // non-supported situation
    throw std::invalid_argument("One and only one of peak window array and "
                                "peak window workspace can be specified.");
  }

  g_log.notice() << "[DB] End of processInputFitRange: "
                 << "is_d_space = " << is_d_space_
                 << ", delta(d)/d = " << m_peakDSpacePercentage << "\n";

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
void FitPeaks::ProcessInputPeakCenters() {
  // peak centers
  m_peakCenters = getProperty("PeakCenters");
  std::string peakpswsname = getPropertyValue("PeakCentersWorkspace");
  if (m_peakCenters.size() > 0 && peakpswsname.size() == 0) {
    // peak positions are uniform among all spectra
    m_uniformPeakPositions = true;
    // number of peaks to fit!
    m_numPeaksToFit = m_peakCenters.size();
  } else if (m_peakCenters.size() == 0 && peakpswsname.size() > 0) {
    // peak positions can be different among spectra
    m_uniformPeakPositions = false;
    m_peakCenterWorkspace = getProperty("PeakCentersWorkspace");
    // number of peaks to fit!
    m_numPeaksToFit = m_peakCenterWorkspace->x(0).size();

    // check matrix worksapce for peak positions
    size_t numhist = m_peakCenterWorkspace->getNumberHistograms();
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
void FitPeaks::ProcessInputPeakTolerance() {
  // check code integrity
  if (m_numPeaksToFit == 0)
    throw std::runtime_error("ProcessInputPeakTolerance() must be called after "
                             "ProcessInputPeakCenters()");

  // peak tolerance
  m_peakPosTolerances = getProperty("PositionTolerance");

  if (m_peakPosTolerances.size() == 0) {
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
void FitPeaks::ConvertParametersNameToIndex() {
  // get a map for peak profile parameter name and parameter index
  std::map<std::string, size_t> parname_index_map;
  for (size_t iparam = 0; iparam < m_peakFunction->nParams(); ++iparam)
    parname_index_map.insert(
        std::make_pair(m_peakFunction->parameterName(iparam), iparam));

  // define peak parameter names (class variable) if using table
  if (m_profileStartingValueTable)
    m_peakParamNames = m_profileStartingValueTable->getColumnNames();

  // map the input parameter names to parameter indexes
  for (size_t i = 0; i < m_peakParamNames.size(); ++i) {
    std::map<std::string, size_t>::iterator locator =
        parname_index_map.find(m_peakParamNames[i]);
    if (locator != parname_index_map.end())
      m_initParamIndexes.push_back(locator->second);
    else {
      // a parameter name that is not defined in the peak profile function.  An
      // out-of-range index is thus set to this
      g_log.warning() << "Given peak parameter " << m_peakParamNames[i]
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
  g_log.notice() << "[DB] Start WS Index = " << m_startWorkspaceIndex
                 << "; (inclusive) Stop WS Index = " << m_stopWorkspaceIndex
                 << "\n";

  // cppcheck-suppress syntaxError
  PRAGMA_OMP(parallel for schedule(dynamic, 1) )
  for (size_t wi = m_startWorkspaceIndex; wi <= m_stopWorkspaceIndex; ++wi) {

    PARALLEL_START_INTERUPT_REGION

    // peaks to fit
    std::vector<double> expected_peak_centers = GetExpectedPeakPositions(wi);

    // initialize output for this
    size_t numfuncparams =
        m_peakFunction->nParams() + m_bkgdFunction->nParams();
    // main output: center
    std::vector<double> fitted_peak_centers(m_numPeaksToFit, -1);
    // others
    std::vector<std::vector<double>> fitted_parameters(
        m_numPeaksToFit); // peak+background
    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      std::vector<double> peak_i(numfuncparams);
      fitted_parameters[ipeak] = peak_i;
    }
    // goodness of fitting
    std::vector<double> peak_chi2_vec(m_numPeaksToFit, DBL_MAX);

    // check number of events
    bool noevents(false);
    if (m_eventNumberWS && m_eventNumberWS->histogram(wi).x()[0] < 1.0) {
      // no event with additional event number workspace
      noevents = true;
    } else if (m_inputEventWS &&
               m_inputEventWS->getNumberEvents() < MIN_EVENTS) {
      // too few events for peak fitting
      noevents = true;
    } else {
      // fit
      fitSpectrumPeaks(wi, expected_peak_centers, fitted_peak_centers,
                       fitted_parameters, &peak_chi2_vec);
    }

    PARALLEL_CRITICAL(FindPeaks_WriteOutput) {
      writeFitResult(wi, expected_peak_centers, fitted_peak_centers,
                     fitted_parameters, peak_chi2_vec, noevents);
    }

    PARALLEL_END_INTERUPT_REGION
  }

  PARALLEL_CHECK_INTERUPT_REGION

  g_log.notice("[DB] End of fitPeaks");
}

//----------------------------------------------------------------------------------------------
/** Fit peaks across one single spectrum
 * @brief FitPeaks::fitSpectrumPeaks
 * @param wi
 * @param expected_peak_centers
 * @param fitted_peak_centers
 * @param fitted_function_parameters
 * @param peak_chi2_vec
 */
void FitPeaks::fitSpectrumPeaks(
    size_t wi, const std::vector<double> &expected_peak_centers,
    std::vector<double> &fitted_peak_centers,
    std::vector<std::vector<double>> &fitted_function_parameters,
    std::vector<double> *peak_chi2_vec) {

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

  // set up properties of algorithm (reference) 'Fit'
  peak_fitter->setProperty("Minimizer", m_minimizer);
  peak_fitter->setProperty("CostFunction", m_costFunction);
  peak_fitter->setProperty("CalcErrors", true);

  // background fitter
  IAlgorithm_sptr bkgd_fitter;
  if (m_highBackground) {
    try {
      bkgd_fitter = createChildAlgorithm("Fit", -1, -1, false);
    } catch (Exception::NotFoundError &) {
      std::stringstream errss;
      errss << "The FitPeak algorithm requires the CurveFitting library";
      throw std::runtime_error(errss.str());
    }
    // set up background fit instance
    bkgd_fitter->setProperty("Minimizer", m_minimizer);
    bkgd_fitter->setProperty("CostFunction", m_costFunction);
    bkgd_fitter->setProperty("CalcErrors", true);
  }

  for (size_t fit_index = 0; fit_index < m_numPeaksToFit; ++fit_index) {

    // convert fit index to peak index (in ascending order)
    size_t peak_index(fit_index);
    if (fit_peaks_from_right_)
      peak_index = m_numPeaksToFit - fit_index - 1;

    g_log.notice() << "[DB] Fit ws-index = " << wi
                   << ", fit-index = " << fit_index << ": expeted "
                   << peak_index << "-th peak @ "
                   << expected_peak_centers[peak_index] << "\n";

    // find out the peak position to fit
    std::pair<double, double> peak_window_i = GetPeakFitWindow(wi, peak_index);

    bool observe_peak_width =
        DecideToEstimatePeakWidth(peak_index, peakfunction);

    // do fitting with peak and background function (no analysis at this point)
    double cost = FitIndividualPeak(wi, peak_fitter, bkgd_fitter, expected_peak_centers[peak_index], peak_window_i,
                                    m_highBackground, observe_peak_width,
                                    peakfunction, bkgdfunction);

    // process fitting result
    ProcessSinglePeakFitResult(
        wi, peak_index, expected_peak_centers, peakfunction, bkgdfunction, cost,
        fitted_peak_centers, fitted_function_parameters, peak_chi2_vec);
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Decided whether to estimate peak width.  If not, then set the width related
 * peak parameters from
 * user specified starting value
 * @brief FitPeaks::DecideToEstimatePeakWidth
 * @param peak_index
 * @param peak_function
 * @return
 */
bool FitPeaks::DecideToEstimatePeakWidth(
    size_t peak_index, API::IPeakFunction_sptr peak_function) {
  bool observe_peak_width(false);

  if (m_initParamIndexes.size() > 0) {
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
      // TODO FIXME - Disable to output fitted result after debugging
      std::stringstream dbss;
      for (size_t i = 0; i < peak_function->nParams(); ++i)
        dbss << peak_function->getParameterNames()[i] << " = "
             << peak_function->getParameter(i) << ", ";
      g_log.notice() << "[DB...BAT] Last fit parameters: " << dbss.str()
                     << "\n";
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
 * @param peakindex
 * @param peakbkgdfunction
 * @param peakfunction
 * @param bkgdfunction
 * @param chi2
 * @param fitted_peak_positions
 * @param peak_params_vector
 * @param peak_chi2_vec
 */
void FitPeaks::ProcessSinglePeakFitResult(
    size_t wsindex, size_t peakindex,
    const std::vector<double> &expected_peak_positions,
    API::IPeakFunction_sptr peakfunction,
    API::IBackgroundFunction_sptr bkgdfunction, double cost,
    std::vector<double> &fitted_peak_positions,
    std::vector<std::vector<double>> &function_parameters_vector,
    std::vector<double> *peak_chi2_vec) {
  // check input
  if (peakindex >= fitted_peak_positions.size() ||
      peakindex >= function_parameters_vector.size() ||
      peakindex >= peak_chi2_vec->size()) {
    throw std::runtime_error("peak index size is out of boundary for fitted "
                             "peaks positions, peak parameters or chi2s");
  }

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
  double peak_pos = peakfunction->centre();
  g_log.notice() << "[DB...Report] Fitted Peak function: , height = " << peakfunction->height()
                 << " position = " << peak_pos
                 << " of position tolerance case234 = " << m_peakPosTolCase234
                 << ", tolerance case23 = " << case23 << "\n";
  bool good_fit(false);

  if ((cost < 0) || (cost > DBL_MAX - 1.)) {
    // unphysical cost function value
    peak_pos = -4;
  } else if (peakfunction->height() < m_minPeakHeight) {
    // peak height is under minimum request
    peak_pos = -3;
  } else if (case23) {
    // case b and c to check peak position without defined peak tolerance
    std::pair<double, double> fitwindow = GetPeakFitWindow(wsindex, peakindex);
    if (fitwindow.first < fitwindow.second) {
      // peak fit window is specified or calculated: use peak window as position
      // tolerance
      if (peak_pos < fitwindow.first || peak_pos > fitwindow.second) {
        // peak is out of fit window
        peak_pos = -2;
        g_log.notice() << "[DB...INFO] Peak position " << peak_pos << " is out of fit "
                       << "window boundary " << fitwindow.first << ", " << fitwindow.second << "\n";
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
      if (right_bound < 0)
        right_bound = left_bound;
      if (left_bound < 0 || right_bound < 0)
        throw std::runtime_error("Code logic error such that left or right "
                                 "boundary of peak position is negative.");
      if (peak_pos < left_bound || peak_pos > right_bound)
        peak_pos = -2.5;
      else
        good_fit = true;
    }
  } else if (fabs(peakfunction->centre() - expected_peak_positions[peakindex]) >
             postol) {
    // peak center is not within tolerance
    peak_pos = -5;
    g_log.notice() << "[DB...INFO] position difference " << fabs(peakfunction->centre() - expected_peak_positions[peakindex])
                   << " is out of tolerance: " << postol << "\n";
  } else {
    // all criteria are passed
    good_fit = true;
  }

  // set cost function to DBL_MAX if fitting is bad
  if (good_fit) {
    // convert fitting result to analysis data structure
    g_log.notice("[DB...BAT  GOOD FIT]");
  } else {
    // set the cost function value to DBL_MAX
    cost = DBL_MAX;
    g_log.notice("[DB...BAT  BAD FIT with big cost");
  }

  // chi2
  (*peak_chi2_vec)[peakindex] = cost;

  double peak_position = peak_pos;
  if (cost > DBL_MAX - 1) {
    peakfunction->setIntensity(0);
  }

  // set peak position
  g_log.notice() << "[DB......BAT] Processed Fitted peak " << peakindex
                 << ": position = " << peak_position << "\n";
  fitted_peak_positions[peakindex] = peak_position;

  // transfer from peak function to vector
  size_t peak_num_params = m_peakFunction->nParams();
  for (size_t ipar = 0; ipar < peak_num_params; ++ipar) {
    // peak function
    function_parameters_vector[peakindex][ipar] =
        peakfunction->getParameter(ipar);
  }
  for (size_t ipar = 0; ipar < m_bkgdFunction->nParams(); ++ipar) {
    // background function
    function_parameters_vector[peakindex][ipar + peak_num_params] =
        bkgdfunction->getParameter(ipar);
  }

  return;
}

//----------------------------------------------------------------------------------------------
// TODO/NOW - Implement such that it can be parallelized
/** calculate fitted peaks with background in the output workspace
 * @brief FitPeaks::calculateFittedPeaks
 */
void FitPeaks::CalculateFittedPeaks() {
  // check
  if (!m_fittedParamTable)
    throw std::runtime_error("No parameters");

  size_t num_peakfunc_params = m_peakFunction->nParams();
  size_t num_bkgdfunc_params = m_bkgdFunction->nParams();

  g_log.notice() << "[DB] Fitted parameter table has "
                 << m_fittedParamTable->rowCount() << " rows. "
                 << "There are " << m_fittedPeakWS->getNumberHistograms()
                 << " spectra, and " << m_numPeaksToFit << " peaks to fit.\n";

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
      // get and set the background function parameters
      for (size_t ipar = 0; ipar < num_bkgdfunc_params; ++ipar) {
        double value_i = m_fittedParamTable->cell<double>(
            row_index, 2 + num_peakfunc_params + ipar);
        bkgd_function->setParameter(ipar, value_i);
      }

      // use domain and function to calcualte
      // get the range of start and stop to construct a function domain
      auto vec_x = m_fittedPeakWS->x(iws);
      std::pair<double, double> peakwindow = GetPeakFitWindow(iws, ipeak);
      std::vector<double>::const_iterator start_x_iter =
          std::lower_bound(vec_x.begin(), vec_x.end(), peakwindow.first);
      std::vector<double>::const_iterator stop_x_iter =
          std::lower_bound(vec_x.begin(), vec_x.end(), peakwindow.second);

      g_log.notice() << "[DB Calculate Fitted Peaks] Domain Range (index): "
                     << start_x_iter - vec_x.begin() << ", "
                     << stop_x_iter - vec_x.begin() << "\n";
      if (start_x_iter == stop_x_iter)
        throw std::runtime_error("Range size is zero");

      FunctionDomain1DVector domain(start_x_iter, stop_x_iter);
      FunctionValues values(domain);
      CompositeFunction_sptr comp_func =
          boost::make_shared<API::CompositeFunction>();
      comp_func->addFunction(peak_function);
      comp_func->addFunction(bkgd_function);
      comp_func->function(domain, values);

      g_log.notice()
          << "[DB Calcuate fitted peaks | check] peak function: height = "
          << peak_function->height() << ", center = " << peak_function->centre()
          << ", width = " << peak_function->fwhm() << "\n";

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
/** Estimate background: There are two methods that will be tried.
 * First, algorithm FindPeakBackground will be tried;
 * If it fails, then a linear background estimator will be called.
 * @brief estimateBackground
 * @param wi
 * @param peak_window
 * @param function
 * @param fitter
 */
void FitPeaks::EstimateBackground(size_t wi,
                                  const std::pair<double, double> &peak_window,
                                  API::IBackgroundFunction_sptr bkgd_function) {
  // call algorithm FindPeakBackground
  std::vector<size_t> peak_min_max_indexes;
  std::vector<double> vector_bkgd(3);

  // peak window: if it is not valid, then use an empty peak window
  std::vector<double> peak_window_v(2);
  peak_window_v[0] = peak_window.first;
  peak_window_v[1] = peak_window.second;
  if (peak_window_v[0] >= peak_window_v[1])
    peak_window_v.clear();

  Mantid::Algorithms::FindPeakBackground bkgd_finder;
  // set values
  bkgd_finder.setFitWindow(peak_window_v);
  bkgd_finder.setBackgroundOrder(2);
  bkgd_finder.setSigma(m_bkgdSimga);
  // find fit window indexes
  size_t l0, n;
  auto histogram = m_inputMatrixWS->histogram(wi);
  bkgd_finder.findWindowIndex(histogram, l0, n);
  // find background
  int find_bkgd = bkgd_finder.findBackground(histogram, l0, n,
                                             peak_min_max_indexes, vector_bkgd);

  g_log.notice()
      << "[DB] Find peak background (Algorithm FindPeakBackground): ws-index = "
      << wi << ", histogram range: (" << histogram.x().front() << ", "
      << histogram.x().back() << "), result = " << find_bkgd << ", X[" << l0
      << ", " << n << "] = " << histogram.x()[l0] << ", " << histogram.x()[n]
      << "\n";
  for (size_t i = 0; i < vector_bkgd.size(); ++i)
    g_log.notice() << "[DB] Background order " << i << " : " << vector_bkgd[i]
                   << "\n";

  // use the simple way to find linear background
  if (find_bkgd <= 0 || true) {
    double bkgd_a1, bkgd_a0;
    this->estimateLinearBackground(wi, peak_window.first, peak_window.second,
                                   bkgd_a1, bkgd_a0);
    vector_bkgd[0] = bkgd_a0;
    vector_bkgd[1] = bkgd_a1;
    vector_bkgd[2] = 0;
  }

  // set result
  // FIXME - this is not flexible for background other than
  // flat/linear/quadratic
  bkgd_function->setParameter(0, vector_bkgd[0]);
  if (bkgd_function->nParams() > 1)
    bkgd_function->setParameter(1, vector_bkgd[1]);
  if (bkgd_function->nParams() > 2)
    bkgd_function->setParameter(2, vector_bkgd[2]);

  for (size_t i = 0; i < vector_bkgd.size(); ++i)
    g_log.notice() << "[DB] Background order " << i << " : " << vector_bkgd[i]
                   << "\n";

  return;
}

//----------------------------------------------------------------------------------------------
/** Estimate peak profile's parameters values via observation
 * including
 * (1) peak center (2) peak intensity  (3) peak width depending on peak type
 * @brief FitPeaks::esitmatePeakParameters
 * @param wi
 * @param peak_window
 * @param peakfunction
 * @param bgkdfunction
 * @param flag to observe peak width or not
 */
int FitPeaks::EstimatePeakParameters(
    size_t wi, const std::pair<double, double> &peak_window,
    API::IPeakFunction_sptr peakfunction,
    API::IBackgroundFunction_sptr bkgdfunction, bool observe_peak_width) {
  // get the range of start and stop to construct a function domain
  auto vector_x = m_inputMatrixWS->x(wi);
  std::vector<double>::const_iterator start_iter =
      std::lower_bound(vector_x.begin(), vector_x.end(), peak_window.first);
  std::vector<double>::const_iterator stop_iter =
      std::lower_bound(vector_x.begin(), vector_x.end(), peak_window.second);
  size_t start_index = static_cast<size_t>(start_iter - vector_x.begin());
  size_t stop_index = static_cast<size_t>(stop_iter - vector_x.begin());

  // calculate background
  g_log.notice() << "[DB] Estimate Peak Parameters: Domain Range (index): "
                 << start_index << ", " << stop_index << "\n";
  if (start_index == stop_index)
    throw std::runtime_error("Range size is zero");

  FunctionDomain1DVector domain(start_iter, stop_iter);
  FunctionValues bkgd_values(domain);
  bkgdfunction->function(domain, bkgd_values);

  auto vector_y = m_inputMatrixWS->y(wi);

  // Estimate peak center
  double peak_center, peak_height;
  size_t peak_center_index;
  g_log.notice("Observe peak center starts...");
  int result = ObservePeakCenter(vector_x, vector_y, bkgd_values, start_index,
                                 stop_index, &peak_center, &peak_center_index,
                                 &peak_height);

  g_log.notice() << "[DB EstimatePeakParameter] Peak center found operation is "
                 << result << " @ x = " << peak_center
                 << ", height = " << peak_height << "\n";

  // set the peak center
  if (result == GOOD) {
    // use values from background to locate FWHM
    peakfunction->setCentre(peak_center);
    peakfunction->setHeight(peak_height);
  }

  // Estimate FHWM (peak width)
  if (result == GOOD && observe_peak_width) {
    // observe peak width
    double peak_width =
        ObservePeakWidth(vector_x, vector_y, bkgd_values, peak_height,
                         peak_center_index, start_index, stop_index);
    if (peak_width > 0) {
      peakfunction->setFwhm(peak_width);
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Guess/estimate peak center and thus height by observation
 * @brief FitPeaks::ObservePeakCenter
 * @param vector_x
 * @param vector_y
 * @param bkgd_values
 * @param start_index
 * @param stop_index
 * @param peak_center
 * @param peak_center_index
 * @param peak_height
 * @return
 */
int FitPeaks::ObservePeakCenter(HistogramData::HistogramX &vector_x,
                                HistogramData::HistogramY &vector_y,
                                FunctionValues &bkgd_values, size_t start_index,
                                size_t stop_index, double *peak_center,
                                size_t *peak_center_index,
                                double *peak_height) {
  // initialize paramters
  double peak_bkgd_max = 0;
  *peak_height = 0;
  *peak_center_index = -1;
  *peak_center = 0;

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
    if (y > *peak_height) {
      *peak_height = y;
      *peak_center = vector_x[curr_index];
      *peak_center_index = curr_index;
    }
    if (vector_y[curr_index] > peak_bkgd_max)
      peak_bkgd_max = vector_y[curr_index];
  }

  g_log.notice() << "[DB Observe peak center/height in "
                 << vector_x[start_index] << ", " << vector_x[stop_index]
                 << ": center = " << *peak_center << ","
                 << " height = " << *peak_height << "\n"
                 << "\t\tmin height = " << m_minPeakHeight
                 << ", peak+bkgd max = " << peak_bkgd_max << "\n";

  // check peak height and determine the case integer to return
  // any case other than GOOD will lead to a non-peak-fit
  const size_t MAGIC3(3);
  int result(0);
  if (peak_bkgd_max < 1.0E-10) {
    // none-event, but no signal within region
    result = NOSIGNAL;
  } else if (*peak_height < m_minPeakHeight) {
    // peak too low
    result = LOWPEAK;
  } else if ((*peak_center_index - start_index) < MAGIC3 ||
             (stop_index - *peak_center_index) < MAGIC3) {
    // peak not at center
    result = OUTOFBOUND;
  } else {
    result = GOOD;
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Observe peak width
 * @brief FitPeaks::ObservePeakWidth
 * @param vector_x
 * @param vector_y:
 * @param peak_center
 * @param ipeak
 * @param istart
 * @param istop
 * @return
 */
double FitPeaks::ObservePeakWidth(HistogramData::HistogramX &vector_x,
                                  HistogramData::HistogramY &vector_y,
                                  FunctionValues &bkgd_values,
                                  double peak_height, size_t ipeak,
                                  size_t istart, size_t istop) {
  // case 1:

  double peak_width;
  if (is_d_space_ && m_peakDSpacePercentage > 0) {
    // width from guessing from delta(D)/D
    double peak_center = vector_x[ipeak];
    peak_width = peak_center * m_peakDSpacePercentage;
  } else if (observe_peak_width_) {
    // observe peak width by estimating FWHM from observation
    // the approach is to locate the first bin with intensity larger than half
    // peak height
    // from left and right
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

    g_log.notice() << "[DB] Observing peak FWHM: "
                   << "center = " << vector_x[ipeak] << "; "
                   << "window = " << vector_x[istart] << ", " << vector_x[istop]
                   << ". "
                   << "find half maximum at " << vector_x[ileft_max] << ", "
                   << vector_x[iright_max] << "\n";

  } else {
    // get from last peak or from input!
    throw std::runtime_error(
        "This case for obsering peak width is not supported.");
    ;
  }

  return peak_width;
}

//----------------------------------------------------------------------------------------------
bool FitPeaks::FitBackground(API::IAlgorithm_sptr md_fitter,
                             const size_t &ws_index,
                             const std::pair<double, double> &fit_window,
                             const double &expected_peak_pos,
                             API::IBackgroundFunction_sptr bkgd_func) {

  // find out how to fit background
  size_t start_index =
      findXIndex(m_inputMatrixWS->histogram(ws_index).x(), fit_window.first);
  size_t stop_index =
      findXIndex(m_inputMatrixWS->histogram(ws_index).x(), fit_window.second);
  size_t expected_peak_index =
      findXIndex(m_inputMatrixWS->histogram(ws_index).x(), expected_peak_pos);

  // treat 5 as a magic number
  if (expected_peak_index - start_index > 10 &&
      stop_index - expected_peak_index - stop_index > 10) {
    // enough data points left for multi-domain fitting
    // set a smaller fit window
    std::vector<double> vec_min(2);
    std::vector<double> vec_max(2);

    vec_min[0] = fit_window.first;
    vec_max[0] =
        m_inputMatrixWS->histogram(ws_index).x()[expected_peak_index - 5];

    vec_min[1] =
        m_inputMatrixWS->histogram(ws_index).x()[expected_peak_index + 5];
    vec_max[1] = fit_window.second;

    double chi2 = FitFunctionMD(md_fitter, bkgd_func, m_inputMatrixWS, ws_index,
                                vec_min, vec_max);

    // process
    bool good_fit(false);
    if (chi2 < DBL_MAX - 1) {
      good_fit = true;
    }

    return good_fit;

  } else {
    // fit as a single domain function.  check whether the result is good or bad

    // TODO FROM HERE!
  }

  return false;
}

//----------------------------------------------------------------------------------------------
/**  Fit a specific peak with estimated peak and background parameters
 * @brief FitPeaks::fitIndividualPeak
 * @param wi
 * @param fitter
 * @param bkgd_fitter :: child algorithm 'Fit' to fit pure background
 * @param exppeakcenter
 * @param fitwindow
 * @param high
 * @param observe_peak_width: flag to estimate peak width (by observation) or
 * not
 * @param peakbkgdfunc
 * @param peakfunction
 * @param bkgdfunc
 * @return
 */
double FitPeaks::FitIndividualPeak(size_t wi, API::IAlgorithm_sptr fitter,
                                   API::IAlgorithm_sptr bkgd_fitter,
                                   const double &expected_peak_center,
                                   const std::pair<double, double> &fitwindow,
                                   const bool &high,
                                   const bool &observe_peak_width,
                                   API::IPeakFunction_sptr peakfunction,
                                   API::IBackgroundFunction_sptr bkgdfunc) {

  double cost(DBL_MAX);

  if (high) {
    // fit peak with high background!
    cost =
        FitFunctionHighBackground(fitter, bkgd_fitter, fitwindow, wi, expected_peak_center,
                                  peakfunction, bkgdfunc, observe_peak_width);
  } else {
    // fit peak and background
    cost = FitFunctionSD(fitter, peakfunction, bkgdfunc, m_inputMatrixWS, wi,
                         fitwindow.first, fitwindow.second, observe_peak_width);
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
 * @param peak_function :: in/out
 * @param bkgd_function :: in/out
 * @param dataws
 * @param wsindex
 * @param xmin
 * @param xmax
 * @param observe_peak_width::flag to 'observe' peak width. Otherwise, use width
 * related parameter set in function
 * @return chi^2 or Rwp depending on input.  If fit is not SUCCESSFUL,
 * return DBL_MAX
 */
double FitPeaks::FitFunctionSD(IAlgorithm_sptr fit,
                               API::IPeakFunction_sptr peak_function,
                               API::IBackgroundFunction_sptr bkgd_function,
                               API::MatrixWorkspace_sptr dataws, size_t wsindex,
                               double xmin, double xmax,
                               bool observe_peak_width) {

  // generate peak window
  std::pair<double, double> peak_window = std::make_pair(xmin, xmax);
  g_log.notice() << "[DB] FitFunctionSD: peak window: " << xmin << ", " << xmax
                 << "\n";

  // Estimate background
  EstimateBackground(wsindex, peak_window, bkgd_function);

  // Estimate peak profile parameter
  EstimatePeakParameters(wsindex, peak_window, peak_function, bkgd_function,
                         observe_peak_width);

  // Create the composition function
  CompositeFunction_sptr comp_func =
      boost::make_shared<API::CompositeFunction>();
  comp_func->addFunction(peak_function);
  comp_func->addFunction(bkgd_function);
  IFunction_sptr fitfunc = boost::dynamic_pointer_cast<IFunction>(comp_func);

  // Set the properties
  g_log.notice() << "[DB...Before Fit()] Spectrum: " << wsindex
                 << ": Estimated center = " << peak_function->centre()
                 << ", height = " << peak_function->height()
                 << ", width = " << peak_function->fwhm() << " in range "
                 << xmin << ", " << xmax << "\n";

  fit->setProperty("Function", fitfunc);
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("MaxIterations", 50); // magic number
  fit->setProperty("StartX", xmin);
  fit->setProperty("EndX", xmax);

  // Execute fit and get result of fitting background
  // m_sstream << "FitSingleDomain: " << fit->asString() << ".\n";

  fit->executeAsChildAlg();
  if (!fit->isExecuted()) {
    g_log.error("Fitting peak SD (single domain) failed to execute.");
    throw std::runtime_error(
        "Fitting peak SD (single domain) failed to execute.");
  }

  // Retrieve result
  std::string fitStatus = fit->getProperty("OutputStatus");
  double chi2 = EMPTY_DBL();
  if (fitStatus == "success") {
    chi2 = fit->getProperty("OutputChi2overDoF");
    fitfunc = fit->getProperty("Function");
  }

  // Debug information
  m_sstream << "[F1201] FitSingleDomain Fitted-Function " << fitfunc->asString()
            << ": Fit-status = " << fitStatus << ", chi^2 = " << chi2 << ".\n";
  g_log.notice() << "[DB F1201] FitSingleDomain Fitted-Function "
                 << fitfunc->asString() << ": Fit-status = " << fitStatus
                 << ", chi^2 = " << chi2 << ".\n";

  return chi2;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::FitFunctionMD
 * @param fit
 * @param fit_function :: function to fit
 * @param dataws :: matrix workspace to fit with
 * @param wsindex ::  workspace index of the spectrum in matrix workspace
 * @param vec_xmin :: minimin values of domains
 * @param vec_xmax :: maximim values of domains
 * @return
 */
double FitPeaks::FitFunctionMD(
    API::IAlgorithm_sptr fit,
    // boost::shared_ptr<API::MultiDomainFunction> mdfunction,
    API::IFunction_sptr fit_function, API::MatrixWorkspace_sptr dataws,
    size_t wsindex, std::vector<double> &vec_xmin,
    std::vector<double> &vec_xmax) {
  // Validate
  if (vec_xmin.size() != vec_xmax.size())
    throw runtime_error("Sizes of xmin and xmax (vectors) are not equal. ");

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

  m_sstream << "FitMultiDomain: Funcion " << md_function->name() << ": "
            << "Range: (" << vec_xmin[0] << ", " << vec_xmax[0] << ") and ("
            << vec_xmin[1] << ", " << vec_xmax[1] << "); "
            << md_function->asString() << "\n";

  // Execute
  fit->execute();
  if (!fit->isExecuted()) {
    throw runtime_error("Fit is not executed on multi-domain function/data. ");
  }

  // Retrieve result
  std::string fitStatus = fit->getProperty("OutputStatus");
  m_sstream << "[DB] Multi-domain fit status: " << fitStatus << ".\n";

  double chi2 = EMPTY_DBL();
  if (fitStatus == "success") {
    chi2 = fit->getProperty("OutputChi2overDoF");
    m_sstream << "FitMultidomain: Successfully-Fitted Function "
              << md_function->asString() << ", Chi^2 = " << chi2 << "\n";
  }

  return chi2;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::FitFunctionHighBackground
 * @param fit
 * @param exp_center
 * @param fit_window
 * @param ws_index
 * @param peakbkgdfunc
 * @param peakfunction
 * @param bkgdfunc
 */
double FitPeaks::FitFunctionHighBackground(
    IAlgorithm_sptr fit, API::IAlgorithm_sptr bkgd_fitter,
    const std::pair<double, double> &fit_window, const size_t &ws_index,
    const double &expected_peak_center,
    API::IPeakFunction_sptr peakfunction,
    API::IBackgroundFunction_sptr bkgdfunc, bool observe_peak_width) {
  // Fit the background first if there is enough data points
  FitBackground(bkgd_fitter, ws_index, fit_window, expected_peak_center, bkgdfunc);

  // Get partial of the data
  std::vector<double> vec_x, vec_y, vec_e;
  GetRangeData(ws_index, fit_window, &vec_x, &vec_y, &vec_e);

  // Reduce the background
  double high_a0, high_a1;
  ReduceBackground(vec_x, &vec_y, &vec_e, &high_a0, &high_a1);

  // Create a new workspace
  API::MatrixWorkspace_sptr reduced_bkgd_ws =
      CreateMatrixWorkspace(vec_x, vec_y, vec_e);

  // Fit background first
  // TODO ASAP - Implement this with full debug output
  // double cost = FitBackgroundMD(fit_bkgd, bkgdfunc, more);

  // Fit peak with background
  double cost = FitFunctionSD(fit, peakfunction, bkgdfunc, reduced_bkgd_ws, 0,
                              vec_x.front(), vec_x.back(), observe_peak_width);

  // add the reduced background back
  bkgdfunc->setParameter(0, bkgdfunc->getParameter(0) + high_a0);
  bkgdfunc->setParameter(1, bkgdfunc->getParameter(1) + high_a1);

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
FitPeaks::CreateMatrixWorkspace(const std::vector<double> &vec_x,
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
void FitPeaks::GenerateOutputPeakPositionWS() {
  // create output workspace for peak positions: can be partial spectra to input
  // workspace
  size_t num_hist = m_stopWorkspaceIndex - m_startWorkspaceIndex + 1;
  output_peak_position_workspace_ = WorkspaceFactory::Instance().create(
      "Workspace2D", num_hist, m_numPeaksToFit, m_numPeaksToFit);
  // set default
  for (size_t wi = 0; wi < num_hist; ++wi) {
    // TODO - Parallization OpenMP

    // convert to workspace index of input data workspace
    size_t inp_wi = wi + m_startWorkspaceIndex;
    std::vector<double> expected_position = GetExpectedPeakPositions(inp_wi);
    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      output_peak_position_workspace_->dataX(wi)[ipeak] =
          expected_position[ipeak];
    }
  }
  g_log.notice() << "[DB] Main output workspace: num histogram = "
                 << output_peak_position_workspace_->getNumberHistograms()
                 << ", size (x) and (y) are "
                 << output_peak_position_workspace_->histogram(0).x().size()
                 << ", "
                 << output_peak_position_workspace_->histogram(0).y().size()
                 << "\n";
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::GenerateFittedParametersValueWorkspace
 */
void FitPeaks::GenerateFittedParametersValueWorkspace() {
  // peak parameter workspace
  std::string param_table_name =
      getPropertyValue("OutputPeakParametersWorkspace");

  // check whether it is not asked to create such table workspace
  if (param_table_name.size() == 0) {
    // Skip if it is not specified
    m_fittedParamTable = 0;
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
  g_log.notice() << "[DB] (inclusive) Stop worksapce index = "
                 << m_stopWorkspaceIndex << "\n";
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
void FitPeaks::GenerateCalculatedPeaksWS() {
  // matrix workspace contained calculated peaks from fitting
  std::string fit_ws_name = getPropertyValue("FittedPeaksWorkspace");
  if (fit_ws_name.size() == 0) {
    // skip if user does not specify
    m_fittedPeakWS = 0;
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
void FitPeaks::ProcessOutputs() {
  setProperty("OutputWorkspace", output_peak_position_workspace_);

  // optional
  if (m_fittedParamTable)
    setProperty("OutputPeakParametersWorkspace", m_fittedParamTable);

  // optional
  if (m_fittedPeakWS && m_fittedParamTable) {
    g_log.debug("about to calcualte fitted peaks");
    CalculateFittedPeaks();
    setProperty("FittedPeaksWorkspace", m_fittedPeakWS);
  }
}

//----------------------------------------------------------------------------------------------
/** Get the expected peak's position
 * @brief FitPeaks::getExpectedPeakPositions
 * @param wi
 * @param ipeak
 * @return
 */
std::vector<double> FitPeaks::GetExpectedPeakPositions(size_t wi) {
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
std::pair<double, double> FitPeaks::GetPeakFitWindow(size_t wi, size_t ipeak) {
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
  if (calculate_window_instrument_) {
    // calcualte peak window by delta(d)/d
    double peak_pos = GetExpectedPeakPositions(wi)[ipeak];
    // calcalate expected peak width
    double estimate_peak_width = peak_pos * m_peakDSpacePercentage;
    // using a MAGIC number to estimate the peak window
    double MAGIC = 3.0;
    left = peak_pos - estimate_peak_width * MAGIC;
    right = peak_pos + estimate_peak_width * MAGIC;

    g_log.notice() << "[DB] Expected = " << peak_pos << ": estimated peak window = " << left << ", " << right
                   << " with estimated peak width " << estimate_peak_width
                   << "\n";
  } else if (m_uniformPeakWindows) {
    // uniform peak fit window
    assert(m_peakWindowVector.size() > 0); // peak fit window must be given!

    left = m_peakWindowVector[ipeak][0];
    right = m_peakWindowVector[ipeak][1];
    g_log.notice() << "[DB] Uniform window of " << ipeak << "-th peak: " << left
                   << ", " << right << "\n";
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
void FitPeaks::GetRangeData(size_t iws,
                            const std::pair<double, double> &fit_window,
                            std::vector<double> *vec_x,
                            std::vector<double> *vec_y,
                            std::vector<double> *vec_e) {

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
  vec_x->resize(num_elements);
  std::copy(orig_x.begin() + left_index, orig_x.begin() + right_index,
            vec_x->begin());

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
  vec_y->resize(num_elements);
  vec_e->resize(num_elements);
  std::copy(orig_y.begin() + left_index, orig_y.begin() + right_index,
            vec_y->begin());
  std::copy(orig_e.begin() + left_index, orig_e.begin() + right_index,
            vec_e->begin());

  return;
}

//----------------------------------------------------------------------------------------------
// find 2 local minima: draw a line as background to reduce
// find 1 local minima: a flat background
void FitPeaks::ReduceBackground(const std::vector<double> &vec_x,
                                std::vector<double> *vec_y,
                                std::vector<double> *vec_e, double *a0,
                                double *a1) {

  // find out all local minima
  std::vector<size_t> local_min_indices;
  if ((*vec_y)[0] <= (*vec_y)[1])
    local_min_indices.push_back(0);
  for (size_t i = 1; i < vec_y->size() - 1; ++i) {
    if ((*vec_y)[i] <= (*vec_y)[i - 1] && (*vec_y)[i] <= (*vec_y)[i + 1])
      local_min_indices.push_back(i);
  }
  size_t lastindex = vec_y->size() - 1;
  if ((*vec_y)[lastindex] <= (*vec_y)[lastindex - 1])
    local_min_indices.push_back(lastindex);

  if (local_min_indices.size() == 0)
    throw std::runtime_error(
        "It is not possible to have less than 0 local minimum for a peak");

  // find background a and b: a * x + b
  if (local_min_indices.size() == 1) {
    // single minimum: flat backgrounds
    *a1 = 0;
    *a0 = (*vec_y)[local_min_indices[0]];
  } else {
    // more than one minima: find 2 with least area
    // calculate the area under the curve
    double orgi_area = 0;
    for (size_t i = 1; i < vec_y->size(); ++i) {
      double y_0 = (*vec_y)[i - 1];
      double y_f = (*vec_y)[i];
      double dx = vec_x[i] - vec_x[i - 1];
      orgi_area += 0.5 * (y_0 + y_f) * dx;
    }

    // loop around to find the pair of 2 lowest local minima
    double min_area = DBL_MAX;
    double min_bkgd_a, min_bkgd_b;
    double x_0 = vec_x[0];
    double x_f = vec_x.back();
    double y_0 = vec_y->front();
    double y_f = vec_y->back();

    for (size_t i = 0; i < local_min_indices.size(); ++i) {
      size_t index_i = local_min_indices[i];
      double x_i = vec_x[index_i];
      double y_i = (*vec_y)[index_i];
      for (size_t j = i + 1; j < local_min_indices.size(); ++j) {
        // get x and y
        size_t index_j = local_min_indices[j];
        double x_j = vec_x[index_j];
        double y_j = (*vec_y)[index_j];

        // calculate a and b
        double a_ij = (y_i - y_j) / (x_i - x_j);
        double b_ij = (y_i * x_j - y_j * x_j) / (x_j - x_i);

        // verify no other local minimum being negative after background removed
        bool all_non_negative = true;
        for (size_t ilm = 0; ilm < local_min_indices.size(); ++ilm) {
          if (ilm == index_j || ilm == index_j)
            continue;

          double y_no_bkgd = (*vec_y)[ilm] - (a_ij * vec_x[ilm] + b_ij);
          if (y_no_bkgd < -0.) {
            all_non_negative = false;
            break;
          }
        }

        // not all local minima are non-negative with this background removed
        if (!all_non_negative)
          continue;

        // calculate background area
        double area_no_bkgd = (y_0 - (a_ij * x_0 + b_ij) + y_f -
                               (a_ij * x_f + b_ij) * (x_f - x_0)) *
                              0.5;

        // update record if it is the minimum
        if (area_no_bkgd < min_area) {
          min_area = area_no_bkgd;
          min_bkgd_a = a_ij;
          min_bkgd_b = b_ij;
        }
      }
    }

    // check
    if (min_area > DBL_MAX - 1)
      throw std::runtime_error("It is impossible not to find any background");

    // FIXME TODO ASAP : give apparent wront values to min_bkgd_a/b and check here!
    *a1 = min_bkgd_a;
    *a0 = min_bkgd_b;
  }

  g_log.notice() << "[DB...Reduce Background] a0 = " << *a0 << ", a1 = " << *a1 << "\n";

  // Reduce the background from the calculated background
  for (size_t i = 0; i < vec_y->size(); ++i) {
    (*vec_y)[i] -= *a1 * vec_x[i] + *a0;
    double e_sq = max((*vec_y)[i], 1.0);
    (*vec_e)[i] = std::sqrt(e_sq);
  }

  // TODO ASAP : debug output

  return;
}

//----------------------------------------------------------------------------------------------
/** Get index of X value in a given spectrum
 * @brief FitPeaks::getXIndex
 * @param wi
 * @param x
 * @return
 */
size_t FitPeaks::GetXIndex(size_t wi, double x) {
  // check input
  if (wi >= m_inputMatrixWS->getNumberHistograms()) {
    g_log.error() << "getXIndex(): given workspace index " << wi
                  << " is out of range [0, "
                  << m_inputMatrixWS->getNumberHistograms() << ")"
                  << "\n";
    throw std::runtime_error(
        "getXIndex() is given an out-of-range workspace index");
  }

  // get value
  auto vec_x = m_inputMatrixWS->histogram(wi).x();
  auto finditer = std::lower_bound(vec_x.begin(), vec_x.end(), x);
  size_t index = static_cast<size_t>(finditer - vec_x.begin());
  return index;
}

//----------------------------------------------------------------------------------------------
///**
// * @brief FitPeaks::fitSinglePeak
// * @param fitfunc
// * @param dataws
// * @param wsindex
// * @param xmin
// * @param xmax
// * @return
// */
///*
// *FitPeak(InputWorkspace='diamond_high_res_d', OutputWorkspace='peak0_19999',
//   * ParameterTableWorkspace='peak0_19999_Param', WorkspaceIndex=19999,
//   * PeakFunctionType='BackToBackExponential',
//   PeakParameterNames='I,A,B,X0,S',
//   * PeakParameterValues='2.5e+06,5400,1700,1.07,0.000355',
//   *
//   FittedPeakParameterValues='129.407,-1.82258e+06,-230935,1.06065,-0.0154214',
//   * BackgroundParameterNames='A0,A1', BackgroundParameterValues='0,0',
//   * FittedBackgroundParameterValues='3694.92,-3237.13',
//   FitWindow='1.05,1.14',
// *PeakRange='1.06,1.09',
//   * MinGuessedPeakWidth=10, MaxGuessedPeakWidth=20, GuessedPeakWidthStep=1,
// *PeakPositionTolerance=0.02)
// *
// */
// double FitPeaks::fitSinglePeakX(size_t wsindex, size_t peakindex,
//                                const std::vector<double> &init_peak_values,
//                                const std::vector<double> &init_bkgd_values,
//                                const std::vector<double> &fit_window,
//                                const std::vector<double> &peak_range,
//                                std::vector<double> &fitted_params_values,
//                                std::vector<double> &fitted_params_errors,
//                                std::vector<double> &fitted_window,
//                                std::vector<double> &fitted_data) {
//  // Set up sub algorithm fit
//  IAlgorithm_sptr fit_peak;
//  try {
//    fit_peak = createChildAlgorithm("FitPeak", -1, -1, false);
//    fit_peak->initialize();
//  } catch (Exception::NotFoundError &) {
//    std::stringstream errss;
//    errss << "The FitPeak algorithm requires the CurveFitting library";
//    g_log.error(errss.str());
//    throw std::runtime_error(errss.str());
//  }

//  std::stringstream namess;
//  namess << m_inputWS->getName() << "_" << wsindex << "_" << peakindex;
//  std::string outwsname = namess.str();
//  namess << "_param";
//  std::string paramwsname = namess.str();

//  fit_peak->setPropertyValue("InputWorkspace", m_inputWS->getName());
//  fit_peak->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
//  fit_peak->setPropertyValue("OutputWorkspace", outwsname);
//  fit_peak->setPropertyValue("ParameterTableWorkspace", paramwsname);
//  // fit_peak->setProperty("PeakFunctionType", "BackToBackExponential");
//  fit_peak->setProperty("PeakFunctionType", mPeakProfile);
//  // TODO/FIXME - from here! fit_peak->setProperty("PeakParameterNames", );
//  // fit_peak->setProperty("PeakParameterNames", "I,A,B,X0,S");
//  fit_peak->setProperty("PeakParameterValues", init_peak_values);
//  fit_peak->setProperty("BackgroundParameterNames", "A0, A1");
//  fit_peak->setProperty("BackgroundParameterValues", init_bkgd_values);
//  fit_peak->setProperty("FitWindow", fit_window);
//  fit_peak->setProperty("PeakRange", peak_range);
//  fit_peak->setProperty("MinGuessedPeakWidth", 10);
//  fit_peak->setProperty("MaxGuessedPeakWidth", 20);
//  fit_peak->setProperty("GuessedPeakWidthStep", 1);
//  fit_peak->setProperty("PeakPositionTolerance", 0.02);

//  fit_peak->executeAsChildAlg();

//  double chi2 = -1;
//  if (!fit_peak->isExecuted()) {
//    std::stringstream errss;
//    errss << "Unable to fit peak of workspace index " << wsindex << "'s "
//          << peakindex << "-th peak";
//    g_log.error(errss.str());
//    return chi2;
//  }

//  // get the information back
//  fitted_params_values.resize(7, 0.);
//  fitted_params_errors.resize(7, 0.);

//  DataObjects::TableWorkspace_sptr param_table =
//      fit_peak->getProperty("ParameterTableWorkspace");
//  if (!param_table) {
//    g_log.information() << "Unable to get fitted parameters\n";
//    return chi2;
//  } else {
//    g_log.information() << "Good to have fitted data\n";

//    chi2 = processFitResult(param_table, fitted_params_values,
//                            fitted_params_errors);
//    //    g_log.notice() << "Number of fitted parameters = " <<
//    //    fitted_params_values.size() << "\n";
//    //    for (size_t i = 0; i < fitted_params_values.size(); ++i)
//    //        g_log.notice() << "Fitted parameter " << i << " = " <<
//    //        fitted_params_values[i] << "\n";

//    MatrixWorkspace_const_sptr out_ws_i =
//        fit_peak->getProperty("OutputWorkspace");
//    auto vecx = out_ws_i->histogram(1).x();
//    //    g_log.notice() << "[DB] Output workspace from " << vecx.front() <<
//    ",
//    //    "
//    //                   << vecx.back() << ", number of points = " <<
//    //                   vecx.size()
//    //                   << "\n";

//    fitted_window.resize(2);
//    fitted_window[0] = vecx.front();
//    fitted_window[1] = vecx.back();

//    auto vecy = out_ws_i->histogram(1).y();
//    fitted_data.resize(vecy.size());
//    for (size_t i = 0; i < vecy.size(); ++i)
//      fitted_data[i] = vecy[i];
//  }

//  return chi2;
//}

void FitPeaks::estimateLinearBackground(size_t wi, double left_window_boundary,
                                        double right_window_boundary,
                                        double &bkgd_a1, double &bkgd_a0) {

  bkgd_a0 = 0.;
  bkgd_a1 = 0.;

  //  g_log.notice() << "[DB] Estimate background between " <<
  //  left_window_boundary
  //                 << " to " << right_window_boundary << "\n";

  auto &vecX = m_inputMatrixWS->x(wi);
  auto &vecY = m_inputMatrixWS->y(wi);
  size_t istart = findXIndex(vecX, left_window_boundary);
  size_t istop = findXIndex(vecX, right_window_boundary);

  double left_x = 0.;
  double left_y = 0.;
  double right_x = 0.;
  double right_y = 0.;
  for (size_t i = 0; i < 3; ++i) {
    left_x += vecX[istart + i] / 3.;
    left_y += vecY[istart + i] / 3.;
    right_x += vecX[istop - i] / 3.;
    right_y += vecY[istop - 1] / 3.;
  }

  bkgd_a1 = (left_y - right_y) / (left_x - right_x);
  bkgd_a0 = (left_y * right_x - right_y * left_x) / (right_x - left_x);

  return;
}

////----------------------------------------------------------------------------------------------
///** Peak position tolerance: there could be three cases for how the peak
// position tolerance is specified.
//  a) specified by users
//  b) defined by peak windows
//  c) half distance to the neighboring peak (if not (a) and not (b))
//  d) whole range of X-axis (if there is one and only one peak in a spectrum)

//  case (b), (c) and (d) will be dealt at the scence to use tolerance to judge
// whether a fitted
//  peak position is acceptible or not

// * @brief FitPeaks::setPeakPosTolerance
// */
// void FitPeaks::setPeakPosTolerance(
//    const std::vector<double> &peak_pos_tolerances) {

//  if (peak_pos_tolerances.size() == 0) {
//    // case 2, 3, 4
//    m_peakPosTolerances.clear();
//    m_peakPosTolCase234 = true;
//  } else if (peak_pos_tolerances.size() == 1) {
//    // only 1 uniform peak position tolerance is defined
//    m_peakPosTolerances.resize(m_numPeaksToFit, peak_pos_tolerances[0]);
//  } else {
//    // a vector is defined
//    m_peakPosTolerances = peak_pos_tolerances;
//  }

//  // final check: in case not being (b) (c) or (d)
//  if (!m_peakPosTolCase234 && (m_peakPosTolerances.size() != m_numPeaksToFit))
//  {
//    g_log.error() << "number of peak position tolerance "
//                  << m_peakPosTolerances.size()
//                  << " is not same as number of peaks " << m_numPeaksToFit
//                  << "\n";
//    throw std::runtime_error("Number of peak position tolerances and number of
//    "
//                             "peaks to fit are inconsistent.");
//  }

//  return;
//}

/// convert peak window from value to index
// std::vector<size_t> getRange(size_t wi,
//                             const std::vector<double> &peak_window) {
//  if (peak_window.size() != 2)
//    throw std::runtime_error("Invalid peak window size");

//  auto vecX = m_inputWS->histogram(wi).x();
//  size_t istart = findXIndex(vecX, peak_window[0]);
//  size_t istop = findXIndex(vecX, peak_window[1]);

//  std::vector<size_t> range_index_window(2);
//  range_index_window[0] = istart;
//  range_index_window[1] = istop;

//  return range_index_window;
//}

//----------------------------------------------------------------------------------------------
/** Write result of peak fit per spectrum to output analysis workspaces
 * @brief FitPeaks::writeFitResult
 * @param wi
 * @param peak_positions
 * @param peak_parameters
 * @param fitted_peaks
 * @param fitted_peaks_windows
 * @param noevents
 */
void FitPeaks::writeFitResult(size_t wi,
                              const std::vector<double> &expected_positions,
                              std::vector<double> &fitted_positions,
                              std::vector<std::vector<double>> &peak_parameters,
                              std::vector<double> &peak_chi2_vec,
                              bool noevents) {

  // check inputs
  if (fitted_positions.size() != expected_positions.size() ||
      fitted_positions.size() != m_numPeaksToFit)
    throw std::runtime_error("Coding logic error such that the number of peaks "
                             "of expected and fitted peak positions "
                             "are not equal.");

  // convert to
  size_t out_wi = wi - m_startWorkspaceIndex;
  if (out_wi >= output_peak_position_workspace_->getNumberHistograms())
  {
    g_log.error() << "workspace index " << wi << " is out of output peak position workspace "
                  << "range of spectra, which contains " << output_peak_position_workspace_->getNumberHistograms()
                  << " spectra" << "\n";
    throw std::runtime_error("Out of boundary to set output peak position workspace");
  }

  // Fill the output peak position workspace
  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {  
    double exp_peak_pos(expected_positions[ipeak]);
    double fitted_peak_pos(-1); // default for no event or no signal
    double peak_chi2(-1E20);    // use negative number for NO fit
    if (!noevents) {
      fitted_peak_pos = fitted_positions[ipeak];
      peak_chi2 = peak_chi2_vec[ipeak];
    }

    output_peak_position_workspace_->mutableX(out_wi)[ipeak] = exp_peak_pos;
    output_peak_position_workspace_->mutableY(out_wi)[ipeak] = fitted_peak_pos;
    output_peak_position_workspace_->mutableE(out_wi)[ipeak] = peak_chi2;
  }

  // return if it is not asked to write fitted peak parameters
  if (!m_fittedParamTable)
    return;

  // Output the peak parameters to the table workspace
  // check vector size
  if (peak_parameters.size() != m_numPeaksToFit)
    throw std::runtime_error("Size of peak parameters vector is not equal to "
                             "number of peaks to fit.");

  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
    // get row number
    size_t row_index = out_wi * m_numPeaksToFit + ipeak;
    // check again with the column size versus peak parameter values
    if (peak_parameters[ipeak].size() !=
        m_fittedParamTable->columnCount() - 3) {
      g_log.error() << "Peak " << ipeak << " has "
                    << peak_parameters[ipeak].size()
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
      for (size_t iparam = 0; iparam < peak_parameters[ipeak].size();
           ++iparam) {
        size_t col_index = iparam + 2;
        if (col_index >= m_fittedParamTable->columnCount()) {
          stringstream err_ss;
          err_ss << "Try to access FittedParamTable's " << col_index
                 << "-th column, which is out of range [0, "
                 << m_fittedParamTable->columnCount() << ")";
          const std::vector<std::string> &col_names =
              m_fittedParamTable->getColumnNames();
          for (auto iter = col_names.begin(); iter != col_names.end(); ++iter)
            err_ss << *iter << "  ";
          throw std::runtime_error(err_ss.str());
        }
        m_fittedParamTable->cell<double>(row_index, col_index) =
            peak_parameters[ipeak][iparam];
      }
    }
  }

  return;
}

DECLARE_ALGORITHM(FitPeaks)

} // namespace Algorithms
} // namespace Mantid
