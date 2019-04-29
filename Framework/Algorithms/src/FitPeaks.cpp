// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/EstimatePolynomial.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/trim.hpp"
#include <limits>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using Mantid::HistogramData::Histogram;
using namespace std;

namespace Mantid {
namespace Algorithms {

namespace FitPeaksAlgorithm {

//----------------------------------------------------------------------------------------------
/// Holds all of the fitting information for a single spectrum
PeakFitResult::PeakFitResult(size_t num_peaks, size_t num_params)
    : m_function_parameters_number(num_params) {
  // check input
  if (num_peaks == 0 || num_params == 0)
    throw std::runtime_error("No peak or no parameter error.");

  //
  m_fitted_peak_positions.resize(num_peaks,
                                 std::numeric_limits<double>::quiet_NaN());
  m_costs.resize(num_peaks, DBL_MAX);
  m_function_parameters_vector.resize(num_peaks);
  m_function_errors_vector.resize(num_peaks);
  for (size_t ipeak = 0; ipeak < num_peaks; ++ipeak) {
    m_function_parameters_vector[ipeak].resize(
        num_params, std::numeric_limits<double>::quiet_NaN());
    m_function_errors_vector[ipeak].resize(
        num_params, std::numeric_limits<double>::quiet_NaN());
  }

  return;
}

//----------------------------------------------------------------------------------------------
size_t PeakFitResult::getNumberParameters() const {
  return m_function_parameters_number;
}

size_t PeakFitResult::getNumberPeaks() const {
  return m_function_parameters_vector.size();
}

//----------------------------------------------------------------------------------------------
/** get the fitting error of a particular parameter
 * @param ipeak :: index of the peak in given peak position vector
 * @param iparam :: index of the parameter in its corresponding peak profile
 * function
 * @return :: fitting error/uncertain of the specified parameter
 */
double PeakFitResult::getParameterError(size_t ipeak, size_t iparam) const {
  return m_function_errors_vector[ipeak][iparam];
}

//----------------------------------------------------------------------------------------------
/** get the fitted value of a particular parameter
 * @param ipeak :: index of the peak in given peak position vector
 * @param iparam :: index of the parameter in its corresponding peak profile
 * function
 * @return :: fitted value of the specified parameter
 */
double PeakFitResult::getParameterValue(size_t ipeak, size_t iparam) const {
  return m_function_parameters_vector[ipeak][iparam];
}

//----------------------------------------------------------------------------------------------
double PeakFitResult::getPeakPosition(size_t ipeak) const {
  return m_fitted_peak_positions[ipeak];
}

//----------------------------------------------------------------------------------------------
double PeakFitResult::getCost(size_t ipeak) const { return m_costs[ipeak]; }

//----------------------------------------------------------------------------------------------
/// set the peak fitting record/parameter for one peak
void PeakFitResult::setRecord(size_t ipeak, const double cost,
                              const double peak_position,
                              const FitFunction fit_functions) {
  // check input
  if (ipeak >= m_costs.size())
    throw std::runtime_error("Peak index is out of range.");

  // set the values
  m_costs[ipeak] = cost;

  // set peak position
  m_fitted_peak_positions[ipeak] = peak_position;

  // transfer from peak function to vector
  size_t peak_num_params = fit_functions.peakfunction->nParams();
  for (size_t ipar = 0; ipar < peak_num_params; ++ipar) {
    // peak function
    m_function_parameters_vector[ipeak][ipar] =
        fit_functions.peakfunction->getParameter(ipar);
    m_function_errors_vector[ipeak][ipar] =
        fit_functions.peakfunction->getError(ipar);
  }
  for (size_t ipar = 0; ipar < fit_functions.bkgdfunction->nParams(); ++ipar) {
    // background function
    m_function_parameters_vector[ipeak][ipar + peak_num_params] =
        fit_functions.bkgdfunction->getParameter(ipar);
    m_function_errors_vector[ipeak][ipar + peak_num_params] =
        fit_functions.bkgdfunction->getError(ipar);
  }
}

//----------------------------------------------------------------------------------------------
/** The peak postition should be negative and indicates what went wrong
 * @param ipeak :: index of the peak in user-specified peak position vector
 * @param peak_position :: bad peak position indicating reason of bad fit
 */
void PeakFitResult::setBadRecord(size_t ipeak, const double peak_position) {
  // check input
  if (ipeak >= m_costs.size())
    throw std::runtime_error("Peak index is out of range");
  if (peak_position >= 0.)
    throw std::runtime_error("Can only set negative postion for bad record");

  // set the values
  m_costs[ipeak] = DBL_MAX;

  // set peak position
  m_fitted_peak_positions[ipeak] = peak_position;

  // transfer from peak function to vector
  for (size_t ipar = 0; ipar < m_function_parameters_number; ++ipar) {
    m_function_parameters_vector[ipeak][ipar] = 0.;
    m_function_errors_vector[ipeak][ipar] =
        std::numeric_limits<double>::quiet_NaN();
  }
}
} // namespace FitPeaksAlgorithm

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
FitPeaks::FitPeaks()
    : m_fitPeaksFromRight(true), m_fitIterations(50), m_numPeaksToFit(0),
      m_minPeakHeight(20.), m_bkgdSimga(1.), m_peakPosTolCase234(false) {}

//----------------------------------------------------------------------------------------------
/** initialize the properties
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
  // min->setUpper(1.); TODO make this a limit
  declareProperty("PeakWidthPercent", EMPTY_DBL(), min,
                  "The estimated peak width as a "
                  "percentage of the d-spacing "
                  "of the center of the peak. Value must be less than 1.");

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

  std::string startvaluegrp("Starting Parameters Setup");
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

  auto min_max_iter = boost::make_shared<BoundedValidator<int>>();
  min_max_iter->setLower(49);
  declareProperty("MaxFitIterations", 50, min_max_iter,
                  "Maximum number of function fitting iterations.");

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
      Kernel::make_unique<ArrayProperty<double>>("PositionTolerance"),
      "List of tolerance on fitted peak positions against given peak positions."
      "If there is only one value given, then ");

  declareProperty("MinimumPeakHeight", 0.,
                  "Minimum peak height such that all the fitted peaks with "
                  "height under this value will be excluded.");

  declareProperty(
      "ConstrainPeakPositions", true,
      "If true peak position will be constrained by estimated positions "
      "(highest Y value position) and "
      "the peak width either estimted by observation or calculate.");

  // additional output for reviewing
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "FittedPeaksWorkspace", "", Direction::Output,
          PropertyMode::Optional),
      "Name of the output matrix workspace with fitted peak. "
      "This output workspace have the same dimesion as the input workspace."
      "The Y values belonged to peaks to fit are replaced by fitted value. "
      "Values of estimated background are used if peak fails to be fit.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
          "OutputPeakParametersWorkspace", "", Direction::Output),
      "Name of table workspace containing all fitted peak parameters.");

  // Optional output table workspace for each individual parameter's fitting
  // error
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
          "OutputParameterFitErrorsWorkspace", "", Direction::Output,
          PropertyMode::Optional),
      "Name of workspace containing all fitted peak parameters' fitting error."
      "It must be used along with FittedPeaksWorkspace and RawPeakParameters "
      "(True)");

  declareProperty("RawPeakParameters", true,
                  "false generates table with effective centre/width/height "
                  "parameters. true generates a table with peak function "
                  "parameters");

  std::string addoutgrp("Analysis");
  setPropertyGroup("OutputPeakParametersWorkspace", addoutgrp);
  setPropertyGroup("FittedPeaksWorkspace", addoutgrp);
  setPropertyGroup("OutputParameterFitErrorsWorkspace", addoutgrp);
  setPropertyGroup("RawPeakParameters", addoutgrp);

  return;
}

//----------------------------------------------------------------------------------------------
/** Validate inputs
 */
std::map<std::string, std::string> FitPeaks::validateInputs() {
  map<std::string, std::string> issues;

  // check that the peak parameters are in parallel properties
  bool haveCommonPeakParameters(false);
  std::vector<string> suppliedParameterNames =
      getProperty("PeakParameterNames");
  std::vector<double> peakParamValues = getProperty("PeakParameterValues");
  if ((!suppliedParameterNames.empty()) || (!peakParamValues.empty())) {
    haveCommonPeakParameters = true;
    if (suppliedParameterNames.size() != peakParamValues.size()) {
      issues["PeakParameterNames"] =
          "must have same number of values as PeakParameterValues";
      issues["PeakParameterValues"] =
          "must have same number of values as PeakParameterNames";
    }
  }

  // get the information out of the table
  std::string partablename = getPropertyValue("PeakParameterValueTable");
  if (!partablename.empty()) {
    if (haveCommonPeakParameters) {
      const std::string msg = "Parameter value table and initial parameter "
                              "name/value vectors cannot be given "
                              "simultanenously.";
      issues["PeakParameterValueTable"] = msg;
      issues["PeakParameterNames"] = msg;
      issues["PeakParameterValues"] = msg;
    } else {
      m_profileStartingValueTable = getProperty("PeakParameterValueTable");
      suppliedParameterNames = m_profileStartingValueTable->getColumnNames();
    }
  }

  // check that the suggested peak parameter names exist in the peak function
  if (!suppliedParameterNames.empty()) {
    std::string peakfunctiontype = getPropertyValue("PeakFunction");
    m_peakFunction = boost::dynamic_pointer_cast<IPeakFunction>(
        API::FunctionFactory::Instance().createFunction(peakfunctiontype));

    // put the names in a vector
    std::vector<string> functionParameterNames;
    for (size_t i = 0; i < m_peakFunction->nParams(); ++i)
      functionParameterNames.push_back(m_peakFunction->parameterName(i));
    // check that the supplied names are in the function
    // it is acceptable to be missing parameters
    bool failed = false;
    for (const auto &name : suppliedParameterNames) {
      if (std::find(functionParameterNames.begin(),
                    functionParameterNames.end(),
                    name) == functionParameterNames.end()) {
        failed = true;
        break;
      }
    }
    if (failed) {
      std::string msg = "Specified invalid parameter for peak function";
      if (haveCommonPeakParameters)
        issues["PeakParameterNames"] = msg;
      else
        issues["PeakParameterValueTable"] = msg;
    }
  }

  // check inputs for uncertainty (fitting error)
  const std::string error_table_name =
      getPropertyValue("OutputParameterFitErrorsWorkspace");
  if (!error_table_name.empty()) {
    const bool use_raw_params = getProperty("RawPeakParameters");
    if (!use_raw_params) {
      const std::string msg =
          "FitPeaks must output RAW peak parameters if fitting error "
          "is chosen to be output";
      issues["RawPeakParameters"] = msg;
    }
  }

  return issues;
}

//----------------------------------------------------------------------------------------------
void FitPeaks::exec() {
  // process inputs
  processInputs();

  // create output workspaces
  generateOutputPeakPositionWS();

  // generateFittedParametersValueWorkspace();
  generateFittedParametersValueWorkspaces();

  generateCalculatedPeaksWS();

  // fit peaks
  auto fit_results = fitPeaks();

  // set the output workspaces to properites
  processOutputs(fit_results);
}

//----------------------------------------------------------------------------------------------
void FitPeaks::processInputs() {
  // input workspaces
  m_inputMatrixWS = getProperty("InputWorkspace");

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
  m_fitIterations = getProperty("MaxFitIterations");

  // Peak centers, tolerance and fitting range
  processInputPeakCenters();
  // check
  if (m_numPeaksToFit == 0)
    throw std::runtime_error("number of peaks to fit is zero.");
  // about how to estimate the peak width
  m_peakWidthPercentage = getProperty("PeakWidthPercent");
  if (isEmpty(m_peakWidthPercentage))
    m_peakWidthPercentage = -1;
  if (m_peakWidthPercentage >= 1.) // TODO
    throw std::runtime_error("PeakWidthPercent must be less than 1");
  g_log.debug() << "peak width/value = " << m_peakWidthPercentage << "\n";

  // set up background
  m_highBackground = getProperty("HighBackground");
  m_bkgdSimga = getProperty("FindBackgroundSigma");

  // Set up peak and background functions
  processInputFunctions();

  // about peak width and other peak parameter estimating method
  if (m_peakWidthPercentage > 0.)
    m_peakWidthEstimateApproach = EstimatePeakWidth::InstrumentResolution;
  else if (isObservablePeakProfile((m_peakFunction->name())))
    m_peakWidthEstimateApproach = EstimatePeakWidth::Observation;
  else
    m_peakWidthEstimateApproach = EstimatePeakWidth::NoEstimation;
  //  m_peakWidthEstimateApproach = EstimatePeakWidth::NoEstimation;
  g_log.debug() << "Process inputs [3] peak type: " << m_peakFunction->name()
                << ", background type: " << m_bkgdFunction->name() << "\n";

  processInputPeakTolerance();
  processInputFitRanges();

  return;
}

//----------------------------------------------------------------------------------------------
/** process inputs for peak profile and background
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

  // TODO check that both parameter names and values exist
  // input peak parameters
  std::string partablename = getPropertyValue("PeakParameterValueTable");
  m_peakParamNames = getProperty("PeakParameterNames");

  if (partablename.empty() && (!m_peakParamNames.empty())) {
    // use uniform starting value of peak parameters
    m_initParamValues = getProperty("PeakParameterValues");
    // convert the parameter name in string to parameter name in integer index
    convertParametersNameToIndex();
    m_uniformProfileStartingValue = true;
  } else if ((!partablename.empty()) && m_peakParamNames.empty()) {
    // use non-uniform starting value of peak parameters
    m_uniformProfileStartingValue = false;
    m_profileStartingValueTable = getProperty(partablename);
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
 */
void FitPeaks::processInputFitRanges() {
  // get peak fit window
  std::vector<double> peakwindow = getProperty("FitWindowBoundaryList");
  std::string peakwindowname = getPropertyValue("FitPeakWindowWorkspace");
  API::MatrixWorkspace_const_sptr peakwindowws =
      getProperty("FitPeakWindowWorkspace");

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
  } else if (peakwindow.empty() && peakwindowws != nullptr) {
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
              << " with the number of peaks " << m_numPeaksToFit << " to fit.";
        throw std::invalid_argument(errss.str());
      }
      const auto &peakWindowX = m_peakWindowWorkspace->x(wi);

      // check window range against peak center
      size_t window_index = window_index_start + wi;
      size_t center_index = window_index - center_index_start;
      const auto &peakCenterX = m_peakCenterWorkspace->x(center_index);

      for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
        double left_w_bound = peakWindowX[ipeak * 2]; // TODO getting on y
        double right_w_bound = peakWindowX[ipeak * 2 + 1];
        double center = peakCenterX[ipeak];
        if (!(left_w_bound < center && center < right_w_bound)) {
          std::stringstream errss;
          errss << "Workspace index " << wi
                << " has incompatible peak window (" // <<<<<<< HERE!!!!!!!!!
                << left_w_bound << ", " << right_w_bound << ") with " << ipeak
                << "-th expected peak's center " << center;
          throw std::runtime_error(errss.str());
        }
      }
    }
  } else if (peakwindow.empty()) {
    // no peak window is defined, then the peak window will be estimated by
    // delta(D)/D
    if (m_inputIsDSpace && m_peakWidthPercentage > 0)
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
 */
void FitPeaks::processInputPeakCenters() {
  // peak centers
  m_peakCenters = getProperty("PeakCenters");
  API::MatrixWorkspace_const_sptr peakcenterws =
      getProperty("PeakCentersWorkspace");
  if (!peakcenterws)
    g_log.error("There is no peak center workspace");

  std::string peakpswsname = getPropertyValue("PeakCentersWorkspace");
  if ((!m_peakCenters.empty()) && peakcenterws == nullptr) {
    // peak positions are uniform among all spectra
    m_uniformPeakPositions = true;
    // number of peaks to fit!
    m_numPeaksToFit = m_peakCenters.size();
  } else if (m_peakCenters.empty() && peakcenterws != nullptr) {
    // peak positions can be different among spectra
    m_uniformPeakPositions = false;
    m_peakCenterWorkspace = getProperty("PeakCentersWorkspace");
    // number of peaks to fit!
    m_numPeaksToFit = m_peakCenterWorkspace->x(0).size();
    g_log.warning() << "Input peak center workspace: "
                    << m_peakCenterWorkspace->x(0).size() << ", "
                    << m_peakCenterWorkspace->y(0).size() << "\n";

    // check matrix worksapce for peak positions
    const size_t peak_center_ws_spectra_number =
        m_peakCenterWorkspace->getNumberHistograms();
    if (peak_center_ws_spectra_number ==
        m_inputMatrixWS->getNumberHistograms()) {
      // full spectra
      m_partialSpectra = false;
    } else if (peak_center_ws_spectra_number ==
               m_stopWorkspaceIndex - m_startWorkspaceIndex + 1) {
      // partial spectra
      m_partialSpectra = true;
    } else {
      // a case indicating programming error
      g_log.error() << "Peak center workspace has "
                    << peak_center_ws_spectra_number << " spectra;"
                    << "Input workspace has "
                    << m_inputMatrixWS->getNumberHistograms() << " spectra;"
                    << "User specifies to fit peaks from "
                    << m_startWorkspaceIndex << " to " << m_stopWorkspaceIndex
                    << ".  They are mismatched to each other.\n";
      throw std::invalid_argument("Input peak center workspace has mismatched "
                                  "number of spectra to selected spectra to "
                                  "fit.");
    }

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
  if (isEmpty(m_minPeakHeight) || m_minPeakHeight < 0.)
    m_minPeakHeight = 0.;

  return;
}

//----------------------------------------------------------------------------------------------
/** Convert the input initial parameter name/value to parameter index/value for
 * faster access
 * according to the parameter name and peak profile function
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
                         "function "
                      << m_peakFunction->name() << "\n";
      m_initParamIndexes.push_back(m_peakFunction->nParams() * 10);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** main method to fit peaks among all
 */
std::vector<boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult>>
FitPeaks::fitPeaks() {
  API::Progress prog(this, 0., 1.,
                     m_stopWorkspaceIndex - m_startWorkspaceIndex);

  /// Vector to record all the FitResult (only containing specified number of
  /// spectra. shift is expected)
  size_t num_fit_result = m_stopWorkspaceIndex - m_startWorkspaceIndex + 1;
  std::vector<boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult>>
      fit_result_vector(num_fit_result);

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

    fitSpectrumPeaks(static_cast<size_t>(wi), expected_peak_centers,
                     fit_result);

    PARALLEL_CRITICAL(FindPeaks_WriteOutput) {
      writeFitResult(static_cast<size_t>(wi), expected_peak_centers,
                     fit_result);
      fit_result_vector[wi - m_startWorkspaceIndex] = fit_result;
    }
    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }

  PARALLEL_CHECK_INTERUPT_REGION

  return fit_result_vector;
}

namespace {
/// Supported peak profiles for observation
std::vector<std::string> supported_peak_profiles{"Gaussian", "Lorentzian",
                                                 "PseudoVoigt", "Voigt"};

double numberCounts(const Histogram &histogram) {
  double total = 0.;
  for (const auto &value : histogram.y())
    total += std::fabs(value);
  return total;
}

//----------------------------------------------------------------------------------------------
/** Get number of counts in a specified range of a histogram
 * @param histogram :: histogram instance
 * @param xmin :: left boundary
 * @param xmax :: right boundary
 * @return :: counts
 */
double numberCounts(const Histogram &histogram, const double xmin,
                    const double xmax) {
  const auto &vector_x = histogram.points();

  // determine left boundary
  std::vector<double>::const_iterator start_iter = vector_x.begin();
  if (xmin > vector_x.front())
    start_iter = std::lower_bound(vector_x.begin(), vector_x.end(), xmin);
  if (start_iter == vector_x.end())
    return 0.; // past the end of the data means nothing to integrate
  // determine right boundary
  std::vector<double>::const_iterator stop_iter = vector_x.end();
  if (xmax < vector_x.back()) // will set at end of vector if too large
    stop_iter = std::lower_bound(start_iter, stop_iter, xmax);

  // convert to indexes to sum over y
  size_t start_index = static_cast<size_t>(start_iter - vector_x.begin());
  size_t stop_index = static_cast<size_t>(stop_iter - vector_x.begin());

  // integrate
  double total = 0.;
  for (size_t i = start_index; i < stop_index; ++i)
    total += std::fabs(histogram.y()[i]);
  return total;
}
} // namespace

//----------------------------------------------------------------------------------------------
/** Fit peaks across one single spectrum
 */
void FitPeaks::fitSpectrumPeaks(
    size_t wi, const std::vector<double> &expected_peak_centers,
    boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result) {
  if (numberCounts(m_inputMatrixWS->histogram(wi)) <= m_minPeakHeight) {
    for (size_t i = 0; i < fit_result->getNumberPeaks(); ++i)
      fit_result->setBadRecord(i, -1.);
    return; // don't do anything
  }

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

  // set up properties of algorithm (reference) 'Fit'
  peak_fitter->setProperty("Minimizer", m_minimizer);
  peak_fitter->setProperty("CostFunction", m_costFunction);
  peak_fitter->setProperty("CalcErrors", true);

  // store the peak fit parameters once one works
  bool foundAnyPeak = false;
  std::vector<double> lastGoodPeakParameters(peakfunction->nParams(), 0.);

  const double x0 = m_inputMatrixWS->histogram(wi).x().front();
  const double xf = m_inputMatrixWS->histogram(wi).x().back();
  for (size_t fit_index = 0; fit_index < m_numPeaksToFit; ++fit_index) {
    // convert fit index to peak index (in ascending order)
    size_t peak_index(fit_index);
    if (m_fitPeaksFromRight)
      peak_index = m_numPeaksToFit - fit_index - 1;

    // reset the background function
    for (size_t i = 0; i < bkgdfunction->nParams(); ++i)
      bkgdfunction->setParameter(0, 0.);

    // set the peak parameters from last good fit - override peak center
    for (size_t i = 0; i < lastGoodPeakParameters.size(); ++i)
      peakfunction->setParameter(i, lastGoodPeakParameters[i]);
    double expected_peak_pos = expected_peak_centers[peak_index];
    peakfunction->setCentre(expected_peak_pos);

    double cost(DBL_MAX);
    if (expected_peak_pos <= x0 || expected_peak_pos >= xf) {
      // out of range and there won't be any fit
      peakfunction->setIntensity(0);
    } else {
      // find out the peak position to fit
      std::pair<double, double> peak_window_i =
          getPeakFitWindow(wi, peak_index);

      bool observe_peak_width_flag =
          decideToEstimatePeakWidth(!foundAnyPeak, peakfunction);

      if (observe_peak_width_flag &&
          m_peakWidthEstimateApproach == EstimatePeakWidth::NoEstimation) {
        g_log.warning(
            "Peak width can be estimated as ZERO.  The result can be wrong");
      }

      // do fitting with peak and background function (no analysis at this
      // point)
      cost = fitIndividualPeak(wi, peak_fitter, expected_peak_pos,
                               peak_window_i, observe_peak_width_flag,
                               peakfunction, bkgdfunction);
      if (cost < 1e7) { // assume it worked and save out the result
        foundAnyPeak = true;
        for (size_t i = 0; i < lastGoodPeakParameters.size(); ++i)
          lastGoodPeakParameters[i] = peakfunction->getParameter(i);
      }
    }
    foundAnyPeak = true;

    // process fitting result
    FitPeaksAlgorithm::FitFunction fit_function;
    fit_function.peakfunction = peakfunction;
    fit_function.bkgdfunction = bkgdfunction;

    processSinglePeakFitResult(wi, peak_index, cost, expected_peak_centers,
                               fit_function, fit_result); // sets the record
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Decide whether to estimate peak width.  If not, then set the width related
 * peak parameters from user specified starting value
 * @param firstPeakInSpectrum :: flag whether the given peak is the first peak
 * in the spectrum
 * @param peak_function :: peak function to set parameter values to
 * @return :: flag whether the peak width shall be observed
 */
bool FitPeaks::decideToEstimatePeakWidth(
    const bool firstPeakInSpectrum, API::IPeakFunction_sptr peak_function) {
  bool observe_peak_width(false);

  if (!m_initParamIndexes.empty()) {
    // user specifies starting value of peak parameters
    if (firstPeakInSpectrum) {
      // set the parameter values in a vector and loop over it
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
    // no previously defined peak parameters: observation is thus required
    observe_peak_width = true;
  }

  return observe_peak_width;
}

//----------------------------------------------------------------------------------------------
/** retrieve the fitted peak information from functions and set to output
 * vectors
 * @param wsindex :: workspace index
 * @param peakindex :: index of peak in given peak position vector
 * @param cost :: cost function value (i.e., chi^2)
 * @param expected_peak_positions :: vector of the expected peak positions
 * @param fitfunction :: pointer to function to retrieve information from
 * @param fit_result :: (output) PeakFitResult instance to set the fitting
 * result to
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
  if ((cost < 0) || (cost >= DBL_MAX - 1.) || std::isnan(cost)) {
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
/** calculate fitted peaks with background in the output workspace
 * The current version gets the peak parameters and background parameters from
 * fitted parameter
 * table
 */
void FitPeaks::calculateFittedPeaks(
    std::vector<boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult>>
        fit_results) {
  // check
  if (!m_fittedParamTable)
    throw std::runtime_error("No parameters");

  size_t num_peakfunc_params = m_peakFunction->nParams();
  size_t num_bkgdfunc_params = m_bkgdFunction->nParams();

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_fittedPeakWS))
  for (int64_t iws = static_cast<int64_t>(m_startWorkspaceIndex);
       iws <= static_cast<int64_t>(m_stopWorkspaceIndex); ++iws) {
    PARALLEL_START_INTERUPT_REGION

    // get a copy of peak function and background function
    IPeakFunction_sptr peak_function =
        boost::dynamic_pointer_cast<IPeakFunction>(m_peakFunction->clone());
    IBackgroundFunction_sptr bkgd_function =
        boost::dynamic_pointer_cast<IBackgroundFunction>(
            m_bkgdFunction->clone());
    boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result_i =
        fit_results[iws - m_startWorkspaceIndex];
    // FIXME - This is a just a pure check
    if (!fit_result_i)
      throw std::runtime_error(
          "There is something wroing with PeakFitResult vector!");

    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      // get and set the peak function parameters
      const double chi2 = fit_result_i->getCost(ipeak);
      if (chi2 > 10.e10)
        continue;

      for (size_t iparam = 0; iparam < num_peakfunc_params; ++iparam)
        peak_function->setParameter(
            iparam, fit_result_i->getParameterValue(ipeak, iparam));
      for (size_t iparam = 0; iparam < num_bkgdfunc_params; ++iparam)
        bkgd_function->setParameter(iparam,
                                    fit_result_i->getParameterValue(
                                        ipeak, num_peakfunc_params + iparam));

      // use domain and function to calcualte
      // get the range of start and stop to construct a function domain
      const auto &vec_x = m_fittedPeakWS->x(static_cast<size_t>(iws));
      std::pair<double, double> peakwindow =
          getPeakFitWindow(static_cast<size_t>(iws), ipeak);
      std::vector<double>::const_iterator start_x_iter =
          std::lower_bound(vec_x.begin(), vec_x.end(), peakwindow.first);
      std::vector<double>::const_iterator stop_x_iter =
          std::lower_bound(vec_x.begin(), vec_x.end(), peakwindow.second);

      if (start_x_iter == stop_x_iter)
        throw std::runtime_error("Range size is zero in calculateFittedPeaks");

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
        m_fittedPeakWS->dataY(static_cast<size_t>(iws))[yindex] =
            values.getCalculated(yindex - istart);
    } // END-FOR (ipeak)
    PARALLEL_END_INTERUPT_REGION
  } // END-FOR (iws)
  PARALLEL_CHECK_INTERUPT_REGION

  return;
}

namespace {
// calculate the moments about the mean
vector<double> calculateMomentsAboutMean(const Histogram &histogram,
                                         const double mean,
                                         FunctionValues &bkgd_values,
                                         size_t start_index,
                                         size_t stop_index) {
  // Calculate second moment about the mean should be the variance. Assume that
  // the peak center is correct
  const auto &x_vec = histogram.points();
  const auto &y_vec = histogram.y();

  const size_t numPoints =
      min(start_index - stop_index, bkgd_values.size()) - 1;

  double zeroth = 0.; // total counts
  double first = 0.;  // peak center
  double second = 0.; // variance = sigma * sigma

  // zeroth and first don't require the mean
  for (size_t i = 0; i < numPoints; ++i) {
    // integrate using Newton's method
    const double y_adj =
        .5 * (y_vec[start_index + i] + y_vec[start_index + i + 1]) -
        .5 * (bkgd_values.getCalculated(i) + bkgd_values.getCalculated(i + 1));
    if (y_adj <= 0.)
      continue; // just skip to the next
    const double dx = x_vec[start_index + i + 1] - x_vec[start_index + i];
    const double x_adj = x_vec[start_index + i] - mean;

    zeroth += y_adj * dx;
    first += x_adj * y_adj * dx;
    second += x_adj * x_adj * y_adj * dx;
  }
  // need to normalize by the total to get the right position because
  // method of moments assumes a distribution normalized to one
  first = first / zeroth;
  second = second / zeroth;

  return vector<double>{zeroth, first, second};
}
} // namespace

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
/** Estimate peak profile's parameters values via observation
 * including
 * (1) peak center (2) peak intensity  (3) peak width depending on peak type
 * In order to make the estimation better, a pre-defined background function is
 * used to remove
 * the background from the obervation data
 * @param histogram :: Histogram instance containing experimental data
 * @param peak_window :: pair of X-value to specify the peak range
 * @param peakfunction :: (in/out) peak function to set observed value to
 * @param bkgdfunction :: background function previously defined
 * @param observe_peak_width :: flag to estimate peak width from input data
 * @return :: obervation success or not
 */
int FitPeaks::estimatePeakParameters(
    const Histogram &histogram, const std::pair<double, double> &peak_window,
    API::IPeakFunction_sptr peakfunction,
    API::IBackgroundFunction_sptr bkgdfunction, bool observe_peak_width) {

  // get the range of start and stop to construct a function domain
  const auto &vector_x = histogram.points();
  std::vector<double>::const_iterator start_iter =
      std::lower_bound(vector_x.begin(), vector_x.end(), peak_window.first);
  std::vector<double>::const_iterator stop_iter =
      std::lower_bound(vector_x.begin(), vector_x.end(), peak_window.second);
  size_t start_index = static_cast<size_t>(start_iter - vector_x.begin());
  size_t stop_index = static_cast<size_t>(stop_iter - vector_x.begin());

  // calculate background
  if (start_index == stop_index)
    throw std::runtime_error("Range size is zero in estimatePeakParameters");
  FunctionDomain1DVector domain(start_iter, stop_iter);
  FunctionValues bkgd_values(domain);
  bkgdfunction->function(domain, bkgd_values);

  // Estimate peak center
  double peak_height;
  double peak_center = peakfunction->centre();
  size_t peak_center_index;

  int result =
      observePeakCenter(histogram, bkgd_values, start_index, stop_index,
                        peak_center, peak_center_index, peak_height);

  // return if failing to 'observe' peak center
  if (result != GOOD)
    return result;

  if (peak_height < m_minPeakHeight || std::isnan(peak_height))
    return LOWPEAK;

  // use values from background to locate FWHM
  peakfunction->setHeight(peak_height);
  peakfunction->setCentre(peak_center);

  // Estimate FHWM (peak width)
  if (observe_peak_width &&
      m_peakWidthEstimateApproach != EstimatePeakWidth::NoEstimation) {
    double peak_width = observePeakWidth(
        histogram, bkgd_values, peak_center_index, start_index, stop_index);

    // proper factor for gaussian
    const double CONVERSION = 1.; // 2. * std::sqrt(2.);
    if (peak_width > 0.) {        // TODO promote to property?
      peakfunction->setFwhm(CONVERSION * peak_width);
    }
  }

  return GOOD;
}

//----------------------------------------------------------------------------------------------
/** check whether a peak profile is allowed to observe peak width and set width
 * @brief isObservablePeakProfile
 * @param peakprofile : name of peak profile to check against
 * @return :: flag whether the specified peak profile observable
 */
bool FitPeaks::isObservablePeakProfile(const std::string &peakprofile) {
  return (std::find(supported_peak_profiles.begin(),
                    supported_peak_profiles.end(),
                    peakprofile) != supported_peak_profiles.end());
}

//----------------------------------------------------------------------------------------------
/** Guess/estimate peak center and thus height by observation
 * @param histogram :: Histogram instance
 * @param bkgd_values :: calculated background value to removed from observed
 * data
 * @param start_index :: X index of the left boundary of observation widow
 * @param stop_index :: X index of the right boundary of the observation window
 * @param peak_center :: estiamted peak center (output)
 * @param peak_center_index :: estimated peak center's index in histogram
 * (output)
 * @param peak_height :: estimated peak height (output)
 * @return :: state whether peak center can be found by obervation
 */
int FitPeaks::observePeakCenter(const Histogram &histogram,
                                FunctionValues &bkgd_values, size_t start_index,
                                size_t stop_index, double &peak_center,
                                size_t &peak_center_index,
                                double &peak_height) {
  const auto &vector_x = histogram.points();

  // find the original starting point
  auto peak_center_iter =
      std::lower_bound(vector_x.begin() + start_index,
                       vector_x.begin() + stop_index, peak_center);
  if (peak_center_iter == vector_x.begin() + stop_index)
    return OUTOFBOUND; // suggested center is not in the window
  peak_center_index = static_cast<size_t>(peak_center_iter - vector_x.begin());

  // initialize the search to that in case something goes wrong below
  const auto &vector_y = histogram.y();
  peak_center =
      *peak_center_iter; // set the value in case something goes wrong below
  peak_height = vector_y[peak_center_index] -
                bkgd_values.getCalculated(peak_center_index - start_index);

  // assume that the actual peak is within 40% (in index number)
  // of the window size of the suggested peak. This assumes that
  // the minimum search size is 5 bins (arbitrary).
  const size_t windowSize = stop_index - start_index;
  const size_t searchBox =
      max(static_cast<size_t>(.3 * static_cast<double>(windowSize)),
          static_cast<size_t>(5));
  const size_t left = max(peak_center_index - searchBox, start_index);
  const size_t rght = min(peak_center_index + searchBox, stop_index);

  for (size_t i = left; i < rght; ++i) {
    const double y = vector_y[i] - bkgd_values.getCalculated(i - start_index);
    if (y > peak_height) {
      peak_height = y;
      peak_center = vector_x[i];
      peak_center_index = i;
    }
  }

  return GOOD;
}

//----------------------------------------------------------------------------------------------
/** estimate peak width from 'observation'
 * @param histogram :: Histogram instance
 * @param bkgd_values :: (output) background values calculated from X in given
 * histogram
 * @param ipeak :: array index for the peak center in histogram
 * @param istart :: array index for the left boundary of the peak
 * @param istop :: array index for the right boundary of the peak
 * @return peak width as double
 */
double FitPeaks::observePeakWidth(const Histogram &histogram,
                                  FunctionValues &bkgd_values, size_t ipeak,
                                  size_t istart, size_t istop) {
  double peak_width(-0.);

  if (m_peakWidthEstimateApproach == EstimatePeakWidth::InstrumentResolution) {
    // width from guessing from delta(D)/D
    const double peak_center = histogram.points()[ipeak];
    peak_width = peak_center * m_peakWidthPercentage;
  } else if (m_peakWidthEstimateApproach == EstimatePeakWidth::Observation) {
    const auto moments = calculateMomentsAboutMean(
        histogram, histogram.points()[ipeak], bkgd_values, istart, istop);

    // not enough total weight in range
    if (moments[0] < m_minPeakHeight)
      peak_width = -1.; // set to bad value
    else
      peak_width = std::sqrt(moments[2]);

  } else {
    // get from last peak or from input!
    throw std::runtime_error(
        "This case for observing peak width is not supported.");
  }

  return peak_width;
}

//----------------------------------------------------------------------------------------------
/** Fit background function
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
    for (size_t n = 0; n < bkgd_func->nParams(); ++n)
      bkgd_func->setParameter(n, 0);

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
 */
double FitPeaks::fitIndividualPeak(size_t wi, API::IAlgorithm_sptr fitter,
                                   const double expected_peak_center,
                                   const std::pair<double, double> &fitwindow,
                                   const bool observe_peak_width,
                                   API::IPeakFunction_sptr peakfunction,
                                   API::IBackgroundFunction_sptr bkgdfunc) {
  double cost(DBL_MAX);

  // confirm that there is something to fit
  if (numberCounts(m_inputMatrixWS->histogram(wi), fitwindow.first,
                   fitwindow.second) <= m_minPeakHeight)
    return cost;

  if (m_highBackground) {
    // fit peak with high background!
    cost =
        fitFunctionHighBackground(fitter, fitwindow, wi, expected_peak_center,
                                  observe_peak_width, peakfunction, bkgdfunc);
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
 */
double FitPeaks::fitFunctionSD(IAlgorithm_sptr fit,
                               API::IPeakFunction_sptr peak_function,
                               API::IBackgroundFunction_sptr bkgd_function,
                               API::MatrixWorkspace_sptr dataws, size_t wsindex,
                               double xmin, double xmax,
                               const double &expected_peak_center,
                               bool observe_peak_width,
                               bool estimate_background) {
  std::stringstream errorid;
  errorid << "(WorkspaceIndex=" << wsindex
          << " PeakCentre=" << expected_peak_center << ")";

  // generate peak window
  std::pair<double, double> peak_window = std::make_pair(xmin, xmax);

  const auto &histogram = dataws->histogram(wsindex);

  // Estimate background
  if (estimate_background) {
    estimateBackground(histogram, peak_window, bkgd_function);
  }

  // Estimate peak profile parameter
  peak_function->setCentre(expected_peak_center); // set expected position first
  int result = estimatePeakParameters(histogram, peak_window, peak_function,
                                      bkgd_function, observe_peak_width);
  if (result != GOOD) {
    peak_function->setCentre(expected_peak_center);
    if (result == NOSIGNAL || result == LOWPEAK) {
      return DBL_MAX; // exit early - don't fit
    }
  }

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
  fit->setProperty("MaxIterations", m_fitIterations); // magic number
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
  g_log.debug() << "[E1201] FitSingleDomain Before fitting, Fit function: "
                << fit->asString() << "\n";
  errorid << " starting function [" << comp_func->asString() << "]";
  try {
    fit->execute();
    g_log.debug() << "[E1202] FitSingleDomain After fitting, Fit function: "
                  << fit->asString() << "\n";

    if (!fit->isExecuted()) {
      g_log.warning() << "Fitting peak SD (single domain) failed to execute. " +
                             errorid.str();
      return DBL_MAX;
    }
  } catch (std::invalid_argument &e) {
    errorid << ": " << e.what();
    g_log.warning() << "While fitting " + errorid.str();
    return DBL_MAX; // probably the wrong thing to do
    throw std::runtime_error("While fitting " + errorid.str());
  }

  // Retrieve result
  std::string fitStatus = fit->getProperty("OutputStatus");
  double chi2{std::numeric_limits<double>::max()};
  if (fitStatus == "success") {
    chi2 = fit->getProperty("OutputChi2overDoF");
  }

  return chi2;
}

//----------------------------------------------------------------------------------------------
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
  fit->setProperty("MaxIterations", m_fitIterations);

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
    fit_function = fit->getProperty("Function");
  }

  return chi2;
}

//----------------------------------------------------------------------------------------------
/// Fit peak with high background
double FitPeaks::fitFunctionHighBackground(
    IAlgorithm_sptr fit, const std::pair<double, double> &fit_window,
    const size_t &ws_index, const double &expected_peak_center,
    bool observe_peak_width, API::IPeakFunction_sptr peakfunction,
    API::IBackgroundFunction_sptr bkgdfunc) {
  // high background to reduce
  API::IBackgroundFunction_sptr high_bkgd_function(nullptr);
  if (m_linearBackgroundFunction)
    high_bkgd_function = boost::dynamic_pointer_cast<API::IBackgroundFunction>(
        m_linearBackgroundFunction->clone());

  // Fit the background first if there is enough data points
  fitBackground(ws_index, fit_window, expected_peak_center, high_bkgd_function);

  // Get partial of the data
  std::vector<double> vec_x, vec_y, vec_e;
  getRangeData(ws_index, fit_window, vec_x, vec_y, vec_e);

  // Reduce the background
  reduceByBackground(high_bkgd_function, vec_x, vec_y);
  for (size_t n = 0; n < bkgdfunc->nParams(); ++n)
    bkgdfunc->setParameter(n, 0);

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

  cost = fitFunctionSD(fit, peakfunction, bkgdfunc, m_inputMatrixWS, ws_index,
                       vec_x.front(), vec_x.back(), expected_peak_center, false,
                       false);

  return cost;
}

//----------------------------------------------------------------------------------------------
/// Create a single spectrum workspace for fitting
API::MatrixWorkspace_sptr
FitPeaks::createMatrixWorkspace(const std::vector<double> &vec_x,
                                const std::vector<double> &vec_y,
                                const std::vector<double> &vec_e) {
  size_t size = vec_x.size();
  size_t ysize = vec_y.size();

  HistogramBuilder builder;
  builder.setX(std::move(size));
  builder.setY(std::move(ysize));
  MatrixWorkspace_sptr matrix_ws = create<Workspace2D>(1, builder.build());

  auto &dataX = matrix_ws->mutableX(0);
  auto &dataY = matrix_ws->mutableY(0);
  auto &dataE = matrix_ws->mutableE(0);

  dataX.assign(vec_x.cbegin(), vec_x.cend());
  dataY.assign(vec_y.cbegin(), vec_y.cend());
  dataE.assign(vec_e.cbegin(), vec_e.cend());

  return matrix_ws;
}

//----------------------------------------------------------------------------------------------
/** generate output workspace for peak positions
 */
void FitPeaks::generateOutputPeakPositionWS() {
  // create output workspace for peak positions: can be partial spectra to input
  // workspace
  size_t num_hist = m_stopWorkspaceIndex - m_startWorkspaceIndex + 1;
  m_outputPeakPositionWorkspace =
      create<Workspace2D>(num_hist, Points(m_numPeaksToFit));
  // set default
  for (size_t wi = 0; wi < num_hist; ++wi) {
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
/** Set up parameter table (parameter value or error)
 * @brief FitPeaks::generateParameterTable
 * @param table_ws:: an empty workspace
 * @param param_names
 * @param with_chi2:: flag to append chi^2 to the table
 */
void FitPeaks::setupParameterTableWorkspace(
    API::ITableWorkspace_sptr table_ws,
    const std::vector<std::string> &param_names, bool with_chi2) {

  // add columns
  table_ws->addColumn("int", "wsindex");
  table_ws->addColumn("int", "peakindex");
  for (const auto &param_name : param_names)
    table_ws->addColumn("double", param_name);
  if (with_chi2)
    table_ws->addColumn("double", "chi2");

  // add rows
  const size_t numParam = m_fittedParamTable->columnCount() - 3;
  for (size_t iws = m_startWorkspaceIndex; iws <= m_stopWorkspaceIndex; ++iws) {
    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      API::TableRow newRow = table_ws->appendRow();
      newRow << static_cast<int>(iws);   // workspace index
      newRow << static_cast<int>(ipeak); // peak number
      for (size_t iparam = 0; iparam < numParam; ++iparam)
        newRow << 0.; // parameters for each peak
      if (with_chi2)
        newRow << DBL_MAX; // chisq
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate table workspace for fitted parameters' value
 * and optionally the table workspace for those parameters' fitting error
 * @brief FitPeaks::generateFittedParametersValueWorkspace
 */
void FitPeaks::generateFittedParametersValueWorkspaces() {
  // peak parameter workspace
  m_rawPeaksTable = getProperty("RawPeakParameters");

  // create parameters
  // peak
  std::vector<std::string> param_vec;
  if (m_rawPeaksTable) {
    std::vector<std::string> peak_params = m_peakFunction->getParameterNames();
    for (const auto &peak_param : peak_params)
      param_vec.push_back(peak_param);
  } else {
    param_vec.emplace_back("centre");
    param_vec.emplace_back("width");
    param_vec.emplace_back("height");
    param_vec.emplace_back("intensity");
  }
  // background
  for (size_t iparam = 0; iparam < m_bkgdFunction->nParams(); ++iparam)
    param_vec.emplace_back(m_bkgdFunction->parameterName(iparam));

  // parameter value table
  m_fittedParamTable = boost::make_shared<TableWorkspace>();
  setupParameterTableWorkspace(m_fittedParamTable, param_vec, true);

  // for error workspace
  std::string fiterror_table_name =
      getPropertyValue("OutputParameterFitErrorsWorkspace");
  // do nothing if user does not specifiy
  if (fiterror_table_name.empty()) {
    // not specified
    m_fitErrorTable = nullptr;
  } else {
    // create table and set up parameter table
    m_fitErrorTable = boost::make_shared<TableWorkspace>();
    setupParameterTableWorkspace(m_fitErrorTable, param_vec, false);
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate the output MatrixWorkspace for calculated peaks (as an option)
 * @brief FitPeaks::generateCalculatedPeaksWS
 */
void FitPeaks::generateCalculatedPeaksWS() {
  // matrix workspace contained calculated peaks from fitting
  std::string fit_ws_name = getPropertyValue("FittedPeaksWorkspace");
  if (fit_ws_name.size() == 0) {
    // skip if user does not specify
    m_fittedPeakWS = nullptr;
    return;
  }

  // create a wokspace with same size as in the input matrix workspace
  m_fittedPeakWS = create<Workspace2D>(*m_inputMatrixWS);
}

//----------------------------------------------------------------------------------------------
/// set up output workspaces
void FitPeaks::processOutputs(
    std::vector<boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult>>
        fit_result_vec) {
  setProperty("OutputWorkspace", m_outputPeakPositionWorkspace);
  setProperty("OutputPeakParametersWorkspace", m_fittedParamTable);

  if (m_fitErrorTable) {
    g_log.warning("Output error table workspace");
    setProperty("OutputParameterFitErrorsWorkspace", m_fitErrorTable);
  } else {
    g_log.warning("No error table output");
  }

  // optional
  if (m_fittedPeakWS && m_fittedParamTable) {
    g_log.debug("about to calcualte fitted peaks");
    calculateFittedPeaks(fit_result_vec);
    setProperty("FittedPeaksWorkspace", m_fittedPeakWS);
  }
}

//----------------------------------------------------------------------------------------------
/// Get the expected peak's position
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
    exp_centers = m_peakCenterWorkspace->x(peak_wi).rawData();
  }

  return exp_centers;
}

//----------------------------------------------------------------------------------------------
/// get the peak fit window
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
    double estimate_peak_width = peak_pos * m_peakWidthPercentage;
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

    left = m_peakWindowWorkspace->x(window_wi)[ipeak * 2];
    right = m_peakWindowWorkspace->x(window_wi)[ipeak * 2 + 1];
  } else {
    throw std::runtime_error("Unhandled case for get peak fit window!");
  }
  if (left >= right) {
    std::stringstream errss;
    errss << "Peak window is inappropriate for workspace index " << wi
          << " peak " << ipeak << ": " << left << " >= " << right;
    throw std::runtime_error(errss.str());
  }

  return std::make_pair(left, right);
}

//----------------------------------------------------------------------------------------------
/** get vector X, Y and E in a given range
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
/** Reduce Y value with given background function
 * @param bkgd_func :: bacground function pointer
 * @param vec_x :: vector of X valye
 * @param vec_y :: (input/output) vector Y to be reduced by background function
 */
void FitPeaks::reduceByBackground(API::IBackgroundFunction_sptr bkgd_func,
                                  const std::vector<double> &vec_x,
                                  std::vector<double> &vec_y) {
  // calculate the background
  FunctionDomain1DVector vectorx(vec_x.begin(), vec_x.end());
  FunctionValues vector_bkgd(vectorx);
  bkgd_func->function(vectorx, vector_bkgd);

  // subtract the background from the supplied data
  for (size_t i = 0; i < vec_y.size(); ++i) {
    (vec_y)[i] -= vector_bkgd[i];
    // it is better not to mess up with E here
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Estimate linear background from observation in a given fit window
 *  This assumes that there are significantly more background bins than peak
 *  bins
 * @param histogram :: Histogram instance
 * @param left_window_boundary :: x value as the left boundary of the fit window
 * @param right_window_boundary :: x value as the right boundary of the fit
 * window
 * @param bkgd_a0 :: constant factor (output)
 * @param bkgd_a1 :: linear factor (output)
 */
void FitPeaks::estimateLinearBackground(const Histogram &histogram,
                                        double left_window_boundary,
                                        double right_window_boundary,
                                        double &bkgd_a0, double &bkgd_a1) {
  bkgd_a0 = 0.;
  bkgd_a1 = 0.;

  const auto &vecX = histogram.points();
  const size_t istart = findXIndex(vecX.rawData(), left_window_boundary);
  const size_t istop = findXIndex(vecX.rawData(), right_window_boundary);

  // 10 is a magic number that worked in a variety of situations
  const size_t iback_start = istart + 10;
  const size_t iback_stop = istop - 10;

  // there aren't enough bins in the window to try to estimate so just leave the
  // estimate at zero
  if (iback_start < iback_stop) {
    double bg2, chisq;
    HistogramData::estimateBackground(1, histogram, istart, istop, iback_start,
                                      iback_stop, bkgd_a0, bkgd_a1, bg2, chisq);
  }
}

//----------------------------------------------------------------------------------------------
/** Write result of peak fit per spectrum to output analysis workspaces
 * including (1) output peak position workspace (2) parameter table workspace
 * and optionally (3) fitting error/uncertainty workspace
 * @brief FitPeaks::writeFitResult
 * @param wi
 * @param expected_positions :: vector for expected peak positions
 * @param fit_result :: PeakFitResult instance
 */
void FitPeaks::writeFitResult(
    size_t wi, const std::vector<double> &expected_positions,
    boost::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result) {
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
    double fitted_peak_pos = fit_result->getPeakPosition(ipeak);
    double peak_chi2 = fit_result->getCost(ipeak);

    m_outputPeakPositionWorkspace->mutableX(out_wi)[ipeak] = exp_peak_pos;
    m_outputPeakPositionWorkspace->mutableY(out_wi)[ipeak] = fitted_peak_pos;
    m_outputPeakPositionWorkspace->mutableE(out_wi)[ipeak] = peak_chi2;
  }

  // Output the peak parameters to the table workspace
  // check vector size

  // last column of the table is for chi2
  size_t chi2_index = m_fittedParamTable->columnCount() - 1;

  // check TableWorkspace and given FitResult
  if (m_rawPeaksTable) {
    // duplicate from FitPeakResult to table workspace
    // check again with the column size versus peak parameter values
    if (fit_result->getNumberParameters() !=
        m_fittedParamTable->columnCount() - 3) {
      g_log.error() << "Peak of type (" << m_peakFunction->name() << ") has "
                    << fit_result->getNumberParameters()
                    << " parameters.  Parameter table shall have 3 more "
                       "columns.  But not it has "
                    << m_fittedParamTable->columnCount() << " columns\n";
      throw std::runtime_error(
          "Peak parameter vector for one peak has different sizes to output "
          "table workspace");
    }
  } else {
    // effective peak profile parameters: need to re-construct the peak function
    if (4 + m_bkgdFunction->nParams() !=
        m_fittedParamTable->columnCount() - 3) {

      std::stringstream err_ss;
      err_ss << "Peak has 4 effective peak parameters and "
             << m_bkgdFunction->nParams() << " background parameters "
             << ". Parameter table shall have 3 more  columns.  But not it has "
             << m_fittedParamTable->columnCount() << " columns";
      throw std::runtime_error(err_ss.str());
    }
  }

  // go through each peak
  // get a copy of peak function and background function
  IPeakFunction_sptr peak_function =
      boost::dynamic_pointer_cast<IPeakFunction>(m_peakFunction->clone());
  size_t num_peakfunc_params = peak_function->nParams();
  size_t num_bkgd_params = m_bkgdFunction->nParams();

  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
    // get row number
    size_t row_index = out_wi * m_numPeaksToFit + ipeak;

    // treat as different cases for writing out raw or effective parametr
    if (m_rawPeaksTable) {
      // duplicate from FitPeakResult to table workspace
      for (size_t iparam = 0; iparam < num_peakfunc_params + num_bkgd_params;
           ++iparam) {
        size_t col_index = iparam + 2;
        // fitted parameter's value
        m_fittedParamTable->cell<double>(row_index, col_index) =
            fit_result->getParameterValue(ipeak, iparam);
        // fitted parameter's fitting error
        if (m_fitErrorTable) {
          m_fitErrorTable->cell<double>(row_index, col_index) =
              fit_result->getParameterError(ipeak, iparam);
        }

      } // end for (iparam)
    } else {
      // effective peak profile parameter
      // construct the peak function
      for (size_t iparam = 0; iparam < num_peakfunc_params; ++iparam)
        peak_function->setParameter(
            iparam, fit_result->getParameterValue(ipeak, iparam));

      // set the effective peak parameters
      m_fittedParamTable->cell<double>(row_index, 2) = peak_function->centre();
      m_fittedParamTable->cell<double>(row_index, 3) = peak_function->fwhm();
      m_fittedParamTable->cell<double>(row_index, 4) = peak_function->height();
      m_fittedParamTable->cell<double>(row_index, 5) =
          peak_function->intensity();

      // background
      for (size_t iparam = 0; iparam < num_bkgd_params; ++iparam)
        m_fittedParamTable->cell<double>(row_index, 6 + iparam) =
            fit_result->getParameterValue(ipeak, num_peakfunc_params + iparam);
    }

    // set chi2
    m_fittedParamTable->cell<double>(row_index, chi2_index) =
        fit_result->getCost(ipeak);
  }

  return;
}

//----------------------------------------------------------------------------------------------
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
