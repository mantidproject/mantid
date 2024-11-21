// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FitPeaks.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FrameworkManager.h"
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
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidHistogramData/EstimatePolynomial.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/WarningSuppressions.h"

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/trim.hpp"
#include <limits>
#include <utility>

using namespace Mantid;
using namespace Algorithms::PeakParameterHelper;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::HistogramData::Histogram;
using namespace std;

namespace Mantid::Algorithms {

namespace {
namespace PropertyNames {
const std::string INPUT_WKSP("InputWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string START_WKSP_INDEX("StartWorkspaceIndex");
const std::string STOP_WKSP_INDEX("StopWorkspaceIndex");
const std::string PEAK_CENTERS("PeakCenters");
const std::string PEAK_CENTERS_WKSP("PeakCentersWorkspace");
const std::string PEAK_FUNC("PeakFunction");
const std::string BACK_FUNC("BackgroundType");
const std::string FIT_WINDOW_LIST("FitWindowBoundaryList");
const std::string FIT_WINDOW_WKSP("FitPeakWindowWorkspace");
const std::string PEAK_WIDTH_PERCENT("PeakWidthPercent");
const std::string PEAK_PARAM_NAMES("PeakParameterNames");
const std::string PEAK_PARAM_VALUES("PeakParameterValues");
const std::string PEAK_PARAM_TABLE("PeakParameterValueTable");
const std::string FIT_FROM_RIGHT("FitFromRight");
const std::string MINIMIZER("Minimizer");
const std::string COST_FUNC("CostFunction");
const std::string MAX_FIT_ITER("MaxFitIterations");
const std::string BACKGROUND_Z_SCORE("FindBackgroundSigma");
const std::string HIGH_BACKGROUND("HighBackground");
const std::string POSITION_TOL("PositionTolerance");
const std::string PEAK_MIN_HEIGHT("MinimumPeakHeight");
const std::string CONSTRAIN_PEAK_POS("ConstrainPeakPositions");
const std::string OUTPUT_WKSP_MODEL("FittedPeaksWorkspace");
const std::string OUTPUT_WKSP_PARAMS("OutputPeakParametersWorkspace");
const std::string OUTPUT_WKSP_PARAM_ERRS("OutputParameterFitErrorsWorkspace");
const std::string RAW_PARAMS("RawPeakParameters");
const std::string PEAK_MIN_SIGNAL_TO_NOISE_RATIO("MinimumSignalToNoiseRatio");
const std::string PEAK_MIN_TOTAL_COUNT("MinimumPeakTotalCount");
const std::string PEAK_MIN_SIGNAL_TO_SIGMA_RATIO("MinimumSignalToSigmaRatio");
} // namespace PropertyNames
} // namespace

namespace FitPeaksAlgorithm {

//----------------------------------------------------------------------------------------------
/// Holds all of the fitting information for a single spectrum
PeakFitResult::PeakFitResult(size_t num_peaks, size_t num_params) : m_function_parameters_number(num_params) {
  // check input
  if (num_peaks == 0 || num_params == 0)
    throw std::runtime_error("No peak or no parameter error.");

  //
  m_fitted_peak_positions.resize(num_peaks, std::numeric_limits<double>::quiet_NaN());
  m_costs.resize(num_peaks, DBL_MAX);
  m_function_parameters_vector.resize(num_peaks);
  m_function_errors_vector.resize(num_peaks);
  for (size_t ipeak = 0; ipeak < num_peaks; ++ipeak) {
    m_function_parameters_vector[ipeak].resize(num_params, std::numeric_limits<double>::quiet_NaN());
    m_function_errors_vector[ipeak].resize(num_params, std::numeric_limits<double>::quiet_NaN());
  }

  return;
}

//----------------------------------------------------------------------------------------------
size_t PeakFitResult::getNumberParameters() const { return m_function_parameters_number; }

size_t PeakFitResult::getNumberPeaks() const { return m_function_parameters_vector.size(); }

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
double PeakFitResult::getPeakPosition(size_t ipeak) const { return m_fitted_peak_positions[ipeak]; }

//----------------------------------------------------------------------------------------------
double PeakFitResult::getCost(size_t ipeak) const { return m_costs[ipeak]; }

//----------------------------------------------------------------------------------------------
/// set the peak fitting record/parameter for one peak
void PeakFitResult::setRecord(size_t ipeak, const double cost, const double peak_position,
                              const FitFunction &fit_functions) {
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
    m_function_parameters_vector[ipeak][ipar] = fit_functions.peakfunction->getParameter(ipar);
    m_function_errors_vector[ipeak][ipar] = fit_functions.peakfunction->getError(ipar);
  }
  for (size_t ipar = 0; ipar < fit_functions.bkgdfunction->nParams(); ++ipar) {
    // background function
    m_function_parameters_vector[ipeak][ipar + peak_num_params] = fit_functions.bkgdfunction->getParameter(ipar);
    m_function_errors_vector[ipeak][ipar + peak_num_params] = fit_functions.bkgdfunction->getError(ipar);
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
    m_function_errors_vector[ipeak][ipar] = std::numeric_limits<double>::quiet_NaN();
  }
}

PeakFitPreCheckResult &PeakFitPreCheckResult::operator+=(const PeakFitPreCheckResult &another) {
  m_submitted_spectrum_peaks += another.m_submitted_spectrum_peaks;
  m_submitted_individual_peaks += another.m_submitted_individual_peaks;
  m_low_count_spectrum += another.m_low_count_spectrum;
  m_out_of_range += another.m_out_of_range;
  m_low_count_individual += another.m_low_count_individual;
  m_not_enough_datapoints += another.m_not_enough_datapoints;
  m_low_snr += another.m_low_snr;

  return *this;
}

void PeakFitPreCheckResult::setNumberOfSubmittedSpectrumPeaks(const size_t n) { m_submitted_spectrum_peaks = n; }

void PeakFitPreCheckResult::setNumberOfSubmittedIndividualPeaks(const size_t n) { m_submitted_individual_peaks = n; }

void PeakFitPreCheckResult::setNumberOfSpectrumPeaksWithLowCount(const size_t n) { m_low_count_spectrum = n; }

void PeakFitPreCheckResult::setNumberOfOutOfRangePeaks(const size_t n) { m_out_of_range = n; }

void PeakFitPreCheckResult::setNumberOfIndividualPeaksWithLowCount(const size_t n) { m_low_count_individual = n; }

void PeakFitPreCheckResult::setNumberOfPeaksWithNotEnoughDataPoints(const size_t n) { m_not_enough_datapoints = n; }

void PeakFitPreCheckResult::setNumberOfPeaksWithLowSignalToNoise(const size_t n) { m_low_snr = n; }

bool PeakFitPreCheckResult::isIndividualPeakRejected() const {
  // the method should be used on an individual peak, not on a spectrum
  assert(m_submitted_spectrum_peaks == 0);
  assert(m_submitted_individual_peaks == 1);

  // if a peak is rejected, it is rejected based on the very first check it fails
  size_t individual_rejection_count = m_low_count_individual + m_not_enough_datapoints + m_low_snr;
  assert(individual_rejection_count <= 1);

  return individual_rejection_count == 1;
}

std::string PeakFitPreCheckResult::getReport() const {
  assert(m_submitted_individual_peaks <= m_submitted_spectrum_peaks);

  // if no peaks were rejected by the pre-check, keep quiet
  if (m_low_count_spectrum + m_out_of_range + m_low_count_individual + m_not_enough_datapoints + m_low_snr == 0)
    return "";

  std::ostringstream os;
  os << "Total number of peaks pre-checked before fitting: " << m_submitted_spectrum_peaks << "\n";
  if (m_low_count_spectrum > 0)
    os << m_low_count_spectrum << " peak(s) rejected: low signal count (whole spectrum).\n";
  if (m_out_of_range > 0)
    os << m_out_of_range << " peak(s) rejected: out of range.\n";
  if (m_not_enough_datapoints > 0)
    os << m_not_enough_datapoints << " peak(s) rejected: not enough X(Y) datapoints.\n";
  if (m_low_count_individual > 0)
    os << m_low_count_individual << " peak(s) rejected: low signal count (individual peak).\n";
  if (m_low_snr > 0)
    os << m_low_snr << " peak(s) rejected: low signal-to-noise ratio.\n";

  return os.str();
}
} // namespace FitPeaksAlgorithm

//----------------------------------------------------------------------------------------------
FitPeaks::FitPeaks()
    : m_fitPeaksFromRight(true), m_fitIterations(50), m_numPeaksToFit(0), m_minPeakHeight(0.),
      m_minSignalToNoiseRatio(0.), m_minPeakTotalCount(0.), m_peakPosTolCase234(false) {}

//----------------------------------------------------------------------------------------------
/** initialize the properties
 */
void FitPeaks::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::INPUT_WKSP, "", Direction::Input),
                  "Name of the input workspace for peak fitting.");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
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
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(PropertyNames::START_WKSP_INDEX, 0, mustBePositive, "Starting workspace index for fit");
  declareProperty(
      PropertyNames::STOP_WKSP_INDEX, EMPTY_INT(),
      "Last workspace index for fit is the smaller of this value and the workspace index of last spectrum.");
  // properties about peak positions to fit
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::PEAK_CENTERS),
                  "List of peak centers to use as initial guess for fit.");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::PEAK_CENTERS_WKSP, "", Direction::Input,
                                                           PropertyMode::Optional),
      "MatrixWorkspace containing referent peak centers for each spectrum, defined at the same workspace indices.");

  const std::string peakcentergrp("Peak Positions");
  setPropertyGroup(PropertyNames::PEAK_CENTERS, peakcentergrp);
  setPropertyGroup(PropertyNames::PEAK_CENTERS_WKSP, peakcentergrp);

  // properties about peak profile
  const std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<API::IPeakFunction>();
  declareProperty(PropertyNames::PEAK_FUNC, "Gaussian", std::make_shared<StringListValidator>(peakNames),
                  "Use of a BackToBackExponential profile is only reccomended if the "
                  "coeficients to calculate A and B are defined in the instrument "
                  "Parameters.xml file.");
  const vector<string> bkgdtypes{"Flat", "Linear", "Quadratic"};
  declareProperty(PropertyNames::BACK_FUNC, "Linear", std::make_shared<StringListValidator>(bkgdtypes),
                  "Type of Background.");

  const std::string funcgroup("Function Types");
  setPropertyGroup(PropertyNames::PEAK_FUNC, funcgroup);
  setPropertyGroup(PropertyNames::BACK_FUNC, funcgroup);

  // properties about peak range including fitting window and peak width
  // (percentage)
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::FIT_WINDOW_LIST),
                  "List of boundaries of the peak fitting window corresponding to "
                  "PeakCenters.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::FIT_WINDOW_WKSP, "",
                                                                       Direction::Input, PropertyMode::Optional),
                  "MatrixWorkspace containing peak windows for each peak center in each spectrum, defined at the same "
                  "workspace indices.");

  auto min = std::make_shared<BoundedValidator<double>>();
  min->setLower(1e-3);
  // min->setUpper(1.); TODO make this a limit
  declareProperty(PropertyNames::PEAK_WIDTH_PERCENT, EMPTY_DBL(), min,
                  "The estimated peak width as a "
                  "percentage of the d-spacing "
                  "of the center of the peak. Value must be less than 1.");

  const std::string fitrangeegrp("Peak Range Setup");
  setPropertyGroup(PropertyNames::PEAK_WIDTH_PERCENT, fitrangeegrp);
  setPropertyGroup(PropertyNames::FIT_WINDOW_LIST, fitrangeegrp);
  setPropertyGroup(PropertyNames::FIT_WINDOW_WKSP, fitrangeegrp);

  // properties about peak parameters' names and value
  declareProperty(std::make_unique<ArrayProperty<std::string>>(PropertyNames::PEAK_PARAM_NAMES),
                  "List of peak parameters' names");
  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::PEAK_PARAM_VALUES),
                  "List of peak parameters' value");
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>(PropertyNames::PEAK_PARAM_TABLE, "",
                                                                      Direction::Input, PropertyMode::Optional),
                  "Name of the an optional workspace, whose each column "
                  "corresponds to given peak parameter names, "
                  "and each row corresponds to a subset of spectra.");

  const std::string startvaluegrp("Starting Parameters Setup");
  setPropertyGroup(PropertyNames::PEAK_PARAM_NAMES, startvaluegrp);
  setPropertyGroup(PropertyNames::PEAK_PARAM_VALUES, startvaluegrp);
  setPropertyGroup(PropertyNames::PEAK_PARAM_TABLE, startvaluegrp);

  // optimization setup
  declareProperty(PropertyNames::FIT_FROM_RIGHT, true,
                  "Flag for the order to fit peaks.  If true, peaks are fitted "
                  "from rightmost;"
                  "Otherwise peaks are fitted from leftmost.");

  const std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();
  declareProperty(PropertyNames::MINIMIZER, "Levenberg-Marquardt",
                  Kernel::IValidator_sptr(new Kernel::StartsWithValidator(minimizerOptions)),
                  "Minimizer to use for fitting.");

  const std::array<string, 2> costFuncOptions = {{"Least squares", "Rwp"}};
  declareProperty(PropertyNames::COST_FUNC, "Least squares",
                  Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(costFuncOptions)), "Cost functions");

  auto min_max_iter = std::make_shared<BoundedValidator<int>>();
  min_max_iter->setLower(49);
  declareProperty(PropertyNames::MAX_FIT_ITER, 50, min_max_iter, "Maximum number of function fitting iterations.");

  const std::string optimizergrp("Optimization Setup");
  setPropertyGroup(PropertyNames::MINIMIZER, optimizergrp);
  setPropertyGroup(PropertyNames::COST_FUNC, optimizergrp);

  // other helping information
  std::ostringstream os;
  os << "Deprecated property. Use " << PropertyNames::PEAK_MIN_SIGNAL_TO_NOISE_RATIO << " instead.";
  declareProperty(PropertyNames::BACKGROUND_Z_SCORE, EMPTY_DBL(), os.str());

  declareProperty(PropertyNames::HIGH_BACKGROUND, true,
                  "Flag whether the input data has high background compared to peak heights.");

  declareProperty(std::make_unique<ArrayProperty<double>>(PropertyNames::POSITION_TOL),
                  "List of tolerance on fitted peak positions against given peak positions."
                  "If there is only one value given, then ");

  declareProperty(PropertyNames::PEAK_MIN_HEIGHT, 0.,
                  "Used for validating peaks before and after fitting. If a peak's observed/estimated or "
                  "fitted height is under this value, the peak will be marked as error.");

  declareProperty(PropertyNames::CONSTRAIN_PEAK_POS, true,
                  "If true peak position will be constrained by estimated positions "
                  "(highest Y value position) and "
                  "the peak width either estimted by observation or calculate.");

  // additional output for reviewing
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP_MODEL, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Name of the output matrix workspace with fitted peak. "
                  "This output workspace has the same dimension as the input workspace."
                  "The Y values belonged to peaks to fit are replaced by fitted value. "
                  "Values of estimated background are used if peak fails to be fit.");

  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(PropertyNames::OUTPUT_WKSP_PARAMS, "",
                                                                            Direction::Output),
                  "Name of table workspace containing all fitted peak parameters.");

  // Optional output table workspace for each individual parameter's fitting
  // error
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(PropertyNames::OUTPUT_WKSP_PARAM_ERRS, "",
                                                                            Direction::Output, PropertyMode::Optional),
                  "Name of workspace containing all fitted peak parameters' fitting error."
                  "It must be used along with FittedPeaksWorkspace and RawPeakParameters "
                  "(True)");

  declareProperty(PropertyNames::RAW_PARAMS, true,
                  "false generates table with effective centre/width/height "
                  "parameters. true generates a table with peak function "
                  "parameters");

  declareProperty(
      PropertyNames::PEAK_MIN_SIGNAL_TO_NOISE_RATIO, 0.,
      "Used for validating peaks before fitting. If the signal-to-noise ratio is under this value, "
      "the peak will be marked as error. This does not apply to peaks for which the noise cannot be estimated.");

  declareProperty(PropertyNames::PEAK_MIN_TOTAL_COUNT, EMPTY_DBL(),
                  "Used for validating peaks before fitting. If the total peak window Y-value count "
                  "is under this value, the peak will be excluded from fitting and calibration.");

  declareProperty(PropertyNames::PEAK_MIN_SIGNAL_TO_SIGMA_RATIO, 0.,
                  "Used for validating peaks after fitting. If the signal-to-sigma ratio is under this value, "
                  "the peak will be excluded from fitting and calibration.");

  const std::string addoutgrp("Analysis");
  setPropertyGroup(PropertyNames::OUTPUT_WKSP_PARAMS, addoutgrp);
  setPropertyGroup(PropertyNames::OUTPUT_WKSP_MODEL, addoutgrp);
  setPropertyGroup(PropertyNames::OUTPUT_WKSP_PARAM_ERRS, addoutgrp);
  setPropertyGroup(PropertyNames::RAW_PARAMS, addoutgrp);
}

//----------------------------------------------------------------------------------------------
/** Validate inputs
 */
std::map<std::string, std::string> FitPeaks::validateInputs() {
  map<std::string, std::string> issues;

  // check that min/max spectra indices make sense - only matters if both are specified
  if (!(isDefault(PropertyNames::START_WKSP_INDEX) && isDefault(PropertyNames::STOP_WKSP_INDEX))) {
    const int startIndex = getProperty(PropertyNames::START_WKSP_INDEX);
    const int stopIndex = getProperty(PropertyNames::STOP_WKSP_INDEX);
    if (startIndex > stopIndex) {
      const std::string msg =
          PropertyNames::START_WKSP_INDEX + " must be less than or equal to " + PropertyNames::STOP_WKSP_INDEX;
      issues[PropertyNames::START_WKSP_INDEX] = msg;
      issues[PropertyNames::STOP_WKSP_INDEX] = msg;
    }
  }

  // check that the peak parameters are in parallel properties
  bool haveCommonPeakParameters(false);
  std::vector<string> suppliedParameterNames = getProperty(PropertyNames::PEAK_PARAM_NAMES);
  std::vector<double> peakParamValues = getProperty(PropertyNames::PEAK_PARAM_VALUES);
  if ((!suppliedParameterNames.empty()) || (!peakParamValues.empty())) {
    haveCommonPeakParameters = true;
    if (suppliedParameterNames.size() != peakParamValues.size()) {
      issues[PropertyNames::PEAK_PARAM_NAMES] = "must have same number of values as PeakParameterValues";
      issues[PropertyNames::PEAK_PARAM_VALUES] = "must have same number of values as PeakParameterNames";
    }
  }

  // get the information out of the table
  std::string partablename = getPropertyValue(PropertyNames::PEAK_PARAM_TABLE);
  if (!partablename.empty()) {
    if (haveCommonPeakParameters) {
      const std::string msg = "Parameter value table and initial parameter "
                              "name/value vectors cannot be given "
                              "simultanenously.";
      issues[PropertyNames::PEAK_PARAM_TABLE] = msg;
      issues[PropertyNames::PEAK_PARAM_NAMES] = msg;
      issues[PropertyNames::PEAK_PARAM_VALUES] = msg;
    } else {
      m_profileStartingValueTable = getProperty(PropertyNames::PEAK_PARAM_TABLE);
      suppliedParameterNames = m_profileStartingValueTable->getColumnNames();
    }
  }

  // check that the suggested peak parameter names exist in the peak function
  if (!suppliedParameterNames.empty()) {
    std::string peakfunctiontype = getPropertyValue(PropertyNames::PEAK_FUNC);
    m_peakFunction =
        std::dynamic_pointer_cast<IPeakFunction>(API::FunctionFactory::Instance().createFunction(peakfunctiontype));

    // put the names in a vector
    std::vector<string> functionParameterNames;
    for (size_t i = 0; i < m_peakFunction->nParams(); ++i)
      functionParameterNames.emplace_back(m_peakFunction->parameterName(i));
    // check that the supplied names are in the function
    // it is acceptable to be missing parameters
    const bool failed = std::any_of(suppliedParameterNames.cbegin(), suppliedParameterNames.cend(),
                                    [&functionParameterNames](const auto &parName) {
                                      return std::find(functionParameterNames.begin(), functionParameterNames.end(),
                                                       parName) == functionParameterNames.end();
                                    });
    if (failed) {
      std::string msg = "Specified invalid parameter for peak function";
      if (haveCommonPeakParameters)
        issues[PropertyNames::PEAK_PARAM_NAMES] = msg;
      else
        issues[PropertyNames::PEAK_PARAM_TABLE] = msg;
    }
  }

  // check inputs for uncertainty (fitting error)
  const std::string error_table_name = getPropertyValue(PropertyNames::OUTPUT_WKSP_PARAM_ERRS);
  if (!error_table_name.empty()) {
    const bool use_raw_params = getProperty(PropertyNames::RAW_PARAMS);
    if (!use_raw_params) {
      issues[PropertyNames::OUTPUT_WKSP_PARAM_ERRS] = "Cannot be used with " + PropertyNames::RAW_PARAMS + "=False";
      issues[PropertyNames::RAW_PARAMS] =
          "Cannot be False with " + PropertyNames::OUTPUT_WKSP_PARAM_ERRS + " specified";
    }
  }

  return issues;
}

//----------------------------------------------------------------------------------------------
void FitPeaks::exec() {
  // process inputs
  processInputs();

  // create output workspace: fitted peak positions
  generateOutputPeakPositionWS();

  // create output workspace: fitted peaks' parameters values
  generateFittedParametersValueWorkspaces();

  // create output workspace: calculated from fitted peak and background
  generateCalculatedPeaksWS();

  // fit peaks
  auto fit_results = fitPeaks();

  // set the output workspaces to properites
  processOutputs(fit_results);
}

//----------------------------------------------------------------------------------------------
void FitPeaks::processInputs() {
  // input workspaces
  m_inputMatrixWS = getProperty(PropertyNames::INPUT_WKSP);

  if (m_inputMatrixWS->getAxis(0)->unit()->unitID() == "dSpacing")
    m_inputIsDSpace = true;
  else
    m_inputIsDSpace = false;

  // spectra to fit
  int start_wi = getProperty(PropertyNames::START_WKSP_INDEX);
  m_startWorkspaceIndex = static_cast<size_t>(start_wi);

  // last spectrum's workspace index, which is included
  int stop_wi = getProperty(PropertyNames::STOP_WKSP_INDEX);
  if (isEmpty(stop_wi))
    m_stopWorkspaceIndex = m_inputMatrixWS->getNumberHistograms() - 1;
  else {
    m_stopWorkspaceIndex = static_cast<size_t>(stop_wi);
    if (m_stopWorkspaceIndex > m_inputMatrixWS->getNumberHistograms() - 1)
      m_stopWorkspaceIndex = m_inputMatrixWS->getNumberHistograms() - 1;
  }

  // total number of spectra to be fit
  m_numSpectraToFit = m_stopWorkspaceIndex - m_startWorkspaceIndex + 1;

  // optimizer, cost function and fitting scheme
  m_minimizer = getPropertyValue(PropertyNames::MINIMIZER);
  m_costFunction = getPropertyValue(PropertyNames::COST_FUNC);
  m_fitPeaksFromRight = getProperty(PropertyNames::FIT_FROM_RIGHT);
  m_constrainPeaksPosition = getProperty(PropertyNames::CONSTRAIN_PEAK_POS);
  m_fitIterations = getProperty(PropertyNames::MAX_FIT_ITER);

  // Peak centers, tolerance and fitting range
  processInputPeakCenters();
  // check
  if (m_numPeaksToFit == 0)
    throw std::runtime_error("number of peaks to fit is zero.");
  // about how to estimate the peak width
  m_peakWidthPercentage = getProperty(PropertyNames::PEAK_WIDTH_PERCENT);
  if (isEmpty(m_peakWidthPercentage))
    m_peakWidthPercentage = -1;
  if (m_peakWidthPercentage >= 1.) // TODO
    throw std::runtime_error("PeakWidthPercent must be less than 1");
  g_log.debug() << "peak width/value = " << m_peakWidthPercentage << "\n";

  // set up background
  m_highBackground = getProperty(PropertyNames::HIGH_BACKGROUND);
  double temp = getProperty(PropertyNames::BACKGROUND_Z_SCORE);
  if (!isEmpty(temp)) {
    std::ostringstream os;
    os << "FitPeaks property \"" << PropertyNames::BACKGROUND_Z_SCORE << "\" is deprecated and will be ignored."
       << "\n";
    logNoOffset(4 /*warning*/, os.str());
  }

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
  std::string peakfunctiontype = getPropertyValue(PropertyNames::PEAK_FUNC);
  m_peakFunction =
      std::dynamic_pointer_cast<IPeakFunction>(API::FunctionFactory::Instance().createFunction(peakfunctiontype));

  // background functions
  std::string bkgdfunctiontype = getPropertyValue(PropertyNames::BACK_FUNC);
  std::string bkgdname;
  if (bkgdfunctiontype == "Linear")
    bkgdname = "LinearBackground";
  else if (bkgdfunctiontype == "Flat") {
    g_log.warning("There may be problems with Flat background");
    bkgdname = "FlatBackground";
  } else
    bkgdname = bkgdfunctiontype;
  m_bkgdFunction =
      std::dynamic_pointer_cast<IBackgroundFunction>(API::FunctionFactory::Instance().createFunction(bkgdname));
  if (m_highBackground)
    m_linearBackgroundFunction = std::dynamic_pointer_cast<IBackgroundFunction>(
        API::FunctionFactory::Instance().createFunction("LinearBackground"));
  else
    m_linearBackgroundFunction = nullptr;

  // TODO check that both parameter names and values exist
  // input peak parameters
  std::string partablename = getPropertyValue(PropertyNames::PEAK_PARAM_TABLE);
  m_peakParamNames = getProperty(PropertyNames::PEAK_PARAM_NAMES);

  m_uniformProfileStartingValue = false;
  if (partablename.empty() && (!m_peakParamNames.empty())) {
    // use uniform starting value of peak parameters
    m_initParamValues = getProperty(PropertyNames::PEAK_PARAM_VALUES);
    // convert the parameter name in string to parameter name in integer index
    convertParametersNameToIndex();
    // m_uniformProfileStartingValue = true;
  } else if ((!partablename.empty()) && m_peakParamNames.empty()) {
    // use non-uniform starting value of peak parameters
    m_profileStartingValueTable = getProperty(partablename);
  } else if (peakfunctiontype != "Gaussian") {
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
  std::vector<double> peakwindow = getProperty(PropertyNames::FIT_WINDOW_LIST);
  std::string peakwindowname = getPropertyValue(PropertyNames::FIT_WINDOW_WKSP);
  API::MatrixWorkspace_const_sptr peakwindowws = getProperty(PropertyNames::FIT_WINDOW_WKSP);

  // in most case, calculate window by instrument resolution is False

  if ((!peakwindow.empty()) && peakwindowname.empty()) {
    // Peak windows are uniform among spectra: use vector for peak windows

    // check peak positions
    if (!m_uniformPeakPositions)
      throw std::invalid_argument(
          "Specifying peak windows with a list requires also specifying peak positions with a list.");
    // check size
    if (peakwindow.size() != m_numPeaksToFit * 2)
      throw std::invalid_argument("Peak window vector must be twice as large as number of peaks.");

    // set up window to m_peakWindowVector
    m_peakWindowVector.resize(m_numPeaksToFit);
    for (size_t i = 0; i < m_numPeaksToFit; ++i) {
      std::vector<double> peakranges(2);
      peakranges[0] = peakwindow[i * 2];
      peakranges[1] = peakwindow[i * 2 + 1];
      // check peak window (range) against peak centers
      if ((peakranges[0] < m_peakCenters[i]) && (m_peakCenters[i] < peakranges[1])) {
        // pass check: set
        m_peakWindowVector[i] = peakranges;
      } else {
        // failed
        std::stringstream errss;
        errss << "Peak " << i << ": user specifies an invalid range and peak center against " << peakranges[0] << " < "
              << m_peakCenters[i] << " < " << peakranges[1];
        throw std::invalid_argument(errss.str());
      }
    } // END-FOR
    m_getPeakFitWindow = [this](std::size_t wi, std::size_t ipeak) -> std::pair<double, double> {
      this->checkWorkspaceIndices(wi);
      this->checkPeakIndices(wi, ipeak);
      double left = this->m_peakWindowVector[ipeak][0];
      double right = this->m_peakWindowVector[ipeak][1];
      this->checkPeakWindowEdgeOrder(left, right);
      return std::make_pair(left, right);
    };
    // END if list peak windows
  } else if (peakwindow.empty() && peakwindowws != nullptr) {
    // use matrix workspace for non-uniform peak windows
    m_peakWindowWorkspace = getProperty(PropertyNames::FIT_WINDOW_WKSP);

    // check each spectrum whether the window is defined with the correct size
    for (std::size_t wi = m_startWorkspaceIndex; wi <= m_stopWorkspaceIndex; wi++) {
      const auto &peakWindowX = m_peakWindowWorkspace->x(wi);
      const auto &peakCenterX = m_peakCenterWorkspace->x(wi);
      if (peakWindowX.empty()) {
        std::stringstream errss;
        errss << "Peak window required at workspace index " << wi << " "
              << "which is undefined in the peak window workspace.  "
              << "Ensure workspace indices correspond in peak window workspace and input workspace "
              << "when using start and stop indices.";
        throw std::invalid_argument(errss.str());
      }
      // check size
      if (peakWindowX.size() % 2 != 0) {
        throw std::invalid_argument("The peak window vector must be even, with two edges for each peak center.");
      }
      if (peakWindowX.size() != peakCenterX.size() * 2) {
        std::stringstream errss;
        errss << "Peak window workspace index " << wi << " has incompatible number of fit windows "
              << peakWindowX.size() / 2 << " with the number of peaks " << peakCenterX.size() << " to fit.";
        throw std::invalid_argument(errss.str());
      }

      for (size_t ipeak = 0; ipeak < peakCenterX.size(); ++ipeak) {
        double left_w_bound = peakWindowX[ipeak * 2];
        double right_w_bound = peakWindowX[ipeak * 2 + 1];
        double center = peakCenterX[ipeak];

        if (!(left_w_bound < center && center < right_w_bound)) {
          std::stringstream errss;
          errss << "Workspace index " << wi << " has incompatible peak window "
                << "(" << left_w_bound << ", " << right_w_bound << ") "
                << "with " << ipeak << "-th expected peak's center " << center;
          throw std::runtime_error(errss.str());
        }
      }
    }
    m_getPeakFitWindow = [this](std::size_t wi, std::size_t ipeak) -> std::pair<double, double> {
      this->checkWorkspaceIndices(wi);
      this->checkPeakIndices(wi, ipeak);
      double left = m_peakWindowWorkspace->x(wi)[ipeak * 2];
      double right = m_peakWindowWorkspace->x(wi)[ipeak * 2 + 1];
      this->checkPeakWindowEdgeOrder(left, right);
      return std::make_pair(left, right);
    };
    // END if workspace peak windows
  } else if (peakwindow.empty()) {
    // no peak window is defined, then the peak window will be estimated by
    // delta(D)/D
    if (m_inputIsDSpace && m_peakWidthPercentage > 0) {
      // m_peakWindowMethod = PeakWindowMethod::TOLERANCE;
      // m_calculateWindowInstrument = true;
      m_getPeakFitWindow = [this](std::size_t wi, std::size_t ipeak) -> std::pair<double, double> {
        this->checkWorkspaceIndices(wi);
        this->checkPeakIndices(wi, ipeak);
        // calcualte peak window by delta(d)/d
        double peak_pos = this->m_getExpectedPeakPositions(wi)[ipeak];
        // calcalate expected peak width
        double estimate_peak_width = peak_pos * m_peakWidthPercentage;
        // using the NUMBER THREE to estimate the peak window
        double THREE = 3.0;
        double left = peak_pos - estimate_peak_width * THREE;
        double right = peak_pos + estimate_peak_width * THREE;
        this->checkPeakWindowEdgeOrder(left, right);
        return std::make_pair(left, right);
      };
    } else {
      throw std::invalid_argument("Without definition of peak window, the "
                                  "input workspace must be in unit of dSpacing "
                                  "and Delta(D)/D must be given!");
    }
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
  m_peakCenters = getProperty(PropertyNames::PEAK_CENTERS);
  API::MatrixWorkspace_const_sptr peakcenterws = getProperty(PropertyNames::PEAK_CENTERS_WKSP);
  if (!peakcenterws)
    g_log.notice("Peak centers are not specified by peak center workspace");

  std::string peakpswsname = getPropertyValue(PropertyNames::PEAK_CENTERS_WKSP);
  if ((!m_peakCenters.empty()) && peakcenterws == nullptr) {
    // peak positions are uniform among all spectra
    m_uniformPeakPositions = true;
    // number of peaks to fit!
    m_numPeaksToFit = m_peakCenters.size();
    m_getExpectedPeakPositions = [this](std::size_t wi) -> std::vector<double> {
      this->checkWorkspaceIndices(wi);
      return this->m_peakCenters;
    };
  } else if (m_peakCenters.empty() && peakcenterws != nullptr) {
    // peak positions can be different among spectra
    m_uniformPeakPositions = false;
    m_peakCenterWorkspace = getProperty(PropertyNames::PEAK_CENTERS_WKSP);
    // number of peaks to fit must correspond to largest number of reference peaks
    m_numPeaksToFit = 0;
    g_log.debug() << "Input peak center workspace: " << m_peakCenterWorkspace->x(0).size() << ", "
                  << m_peakCenterWorkspace->y(0).size() << "\n";
    for (std::size_t wi = m_startWorkspaceIndex; wi <= m_stopWorkspaceIndex; wi++) {
      if (m_peakCenterWorkspace->x(wi).empty()) {
        std::stringstream errss;
        errss << "Fit peaks was asked to fit from workspace index " << m_startWorkspaceIndex << " "
              << "until workspace index " << m_stopWorkspaceIndex << ".  "
              << "However, the peak center workspace does not have values defined "
              << "at workspace index " << wi << ".  "
              << "Make sure the workspace indices between input and peak center workspaces correspond.";
        g_log.error() << errss.str();
        throw std::invalid_argument(errss.str());
      }
      // the number of peaks to try to fit should be the max number of peaks across spectra
      m_numPeaksToFit = std::max(m_numPeaksToFit, m_peakCenterWorkspace->x(wi).size());
    }
    m_getExpectedPeakPositions = [this](std::size_t wi) -> std::vector<double> {
      this->checkWorkspaceIndices(wi);
      return this->m_peakCenterWorkspace->x(wi).rawData();
    };
  } else {
    std::stringstream errss;
    errss << "One and only one in 'PeakCenters' (vector) and "
             "'PeakCentersWorkspace' shall be given. "
          << "'PeakCenters' has size " << m_peakCenters.size() << ", and name of peak center workspace "
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
  m_peakPosTolerances = getProperty(PropertyNames::POSITION_TOL);

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
    g_log.error() << "number of peak position tolerance " << m_peakPosTolerances.size()
                  << " is not same as number of peaks " << m_numPeaksToFit << "\n";
    throw std::runtime_error("Number of peak position tolerances and number of "
                             "peaks to fit are inconsistent.");
  }

  // set the minimum peak height to 0 (default value) if not specified or invalid
  m_minPeakHeight = getProperty(PropertyNames::PEAK_MIN_HEIGHT);
  if (isEmpty(m_minPeakHeight) || m_minPeakHeight < 0.)
    m_minPeakHeight = 0.;

  // PEAK_MIN_HEIGHT used to function as both "peak height" and "total count" checker.
  // Now the "total count" is checked by PEAK_MIN_TOTAL_COUNT, so set it accordingly.
  m_minPeakTotalCount = getProperty(PropertyNames::PEAK_MIN_TOTAL_COUNT);
  if (m_minPeakHeight > 0 && isEmpty(m_minPeakTotalCount))
    m_minPeakTotalCount = m_minPeakHeight;
  else {
    // set the minimum peak total count to 0 if not specified or invalid
    if (isEmpty(m_minPeakTotalCount) || m_minPeakTotalCount < 0.)
      m_minPeakTotalCount = 0.;
  }

  // set the signal-to-noise threshold to zero (default value) if not specified or invalid
  m_minSignalToNoiseRatio = getProperty(PropertyNames::PEAK_MIN_SIGNAL_TO_NOISE_RATIO);
  if (isEmpty(m_minSignalToNoiseRatio) || m_minSignalToNoiseRatio < 0.)
    m_minSignalToNoiseRatio = 0.;

  // set the signal-to-sigma threshold to zero (default value) if not specified or invalid
  m_minSignalToSigmaRatio = getProperty(PropertyNames::PEAK_MIN_SIGNAL_TO_SIGMA_RATIO);
  if (isEmpty(m_minSignalToSigmaRatio) || m_minSignalToSigmaRatio < 0.)
    m_minSignalToSigmaRatio = 0.;
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
    parname_index_map.insert(std::make_pair(m_peakFunction->parameterName(iparam), iparam));

  // define peak parameter names (class variable) if using table
  if (m_profileStartingValueTable)
    m_peakParamNames = m_profileStartingValueTable->getColumnNames();

  // map the input parameter names to parameter indexes
  for (const auto &paramName : m_peakParamNames) {
    auto locator = parname_index_map.find(paramName);
    if (locator != parname_index_map.end()) {
      m_initParamIndexes.emplace_back(locator->second);
    } else {
      // a parameter name that is not defined in the peak profile function.  An
      // out-of-range index is thus set to this
      g_log.warning() << "Given peak parameter " << paramName
                      << " is not an allowed parameter of peak "
                         "function "
                      << m_peakFunction->name() << "\n";
      m_initParamIndexes.emplace_back(m_peakFunction->nParams() * 10);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** main method to fit peaks among all
 */
std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> FitPeaks::fitPeaks() {
  API::Progress prog(this, 0., 1., m_numPeaksToFit - 1);

  /// Vector to record all the FitResult (only containing specified number of
  /// spectra. shift is expected)
  std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> fit_result_vector(m_numSpectraToFit);

  const int nThreads = FrameworkManager::Instance().getNumOMPThreads();
  size_t chunkSize = m_numSpectraToFit / nThreads;

  std::shared_ptr<FitPeaksAlgorithm::PeakFitPreCheckResult> pre_check_result =
      std::make_shared<FitPeaksAlgorithm::PeakFitPreCheckResult>();

  PRAGMA_OMP(parallel for schedule(dynamic, 1) )
  for (int ithread = 0; ithread < nThreads; ithread++) {
    PARALLEL_START_INTERRUPT_REGION
    auto iws_begin = m_startWorkspaceIndex + chunkSize * static_cast<size_t>(ithread);
    auto iws_end = (ithread == nThreads - 1) ? m_stopWorkspaceIndex + 1 : iws_begin + chunkSize;

    // vector to store fit params for last good fit to each peak
    std::vector<std::vector<double>> lastGoodPeakParameters(m_numPeaksToFit,
                                                            std::vector<double>(m_peakFunction->nParams(), 0.0));

    for (auto wi = iws_begin; wi < iws_end; ++wi) {
      // peaks to fit
      std::vector<double> expected_peak_centers = m_getExpectedPeakPositions(static_cast<size_t>(wi));

      // initialize output for this
      size_t numfuncparams = m_peakFunction->nParams() + m_bkgdFunction->nParams();
      std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result =
          std::make_shared<FitPeaksAlgorithm::PeakFitResult>(m_numPeaksToFit, numfuncparams);

      std::shared_ptr<FitPeaksAlgorithm::PeakFitPreCheckResult> spectrum_pre_check_result =
          std::make_shared<FitPeaksAlgorithm::PeakFitPreCheckResult>();

      fitSpectrumPeaks(static_cast<size_t>(wi), expected_peak_centers, fit_result, lastGoodPeakParameters,
                       spectrum_pre_check_result);

      PARALLEL_CRITICAL(FindPeaks_WriteOutput) {
        writeFitResult(static_cast<size_t>(wi), expected_peak_centers, fit_result);
        fit_result_vector[wi - m_startWorkspaceIndex] = fit_result;
        *pre_check_result += *spectrum_pre_check_result;
      }
      prog.report();
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  logNoOffset(5 /*notice*/, pre_check_result->getReport());
  return fit_result_vector;
}

namespace {
// Forward declarations
bool estimateBackgroundParameters(const Histogram &histogram, const std::pair<size_t, size_t> &peak_window,
                                  const API::IBackgroundFunction_sptr &bkgd_function);
void reduceByBackground(const API::IBackgroundFunction_sptr &bkgd_func, const std::vector<double> &vec_x,
                        std::vector<double> &vec_y);
template <typename vector_like>
void rangeToIndexBounds(const vector_like &vecx, const double range_left, const double range_right, size_t &left_index,
                        size_t &right_index);

/// Supported peak profiles for observation
std::vector<std::string> supported_peak_profiles{"Gaussian", "Lorentzian", "PseudoVoigt", "Voigt",
                                                 "BackToBackExponential"};

//----------------------------------------------------------------------------------------------
/** Estimate background noise from peak-window Y-values.
 * @param vec_y :: vector of peak-window Y-values
 * @return :: noise estimate
 */
double estimateBackgroundNoise(const std::vector<double> &vec_y) {
  // peak window must have a certain minimum number of data points necessary to do the statistics
  size_t half_number_of_bkg_datapoints{5};
  if (vec_y.size() < 2 * half_number_of_bkg_datapoints + 3 /*a magic number*/)
    return DBL_MIN; // can't estimate the noise

  // the specified number of left-most and right-most data points in the peak window are assumed to represent
  // background. Combine these data points into a single vector
  std::vector<double> vec_bkg;
  vec_bkg.resize(2 * half_number_of_bkg_datapoints);
  std::copy(vec_y.begin(), vec_y.begin() + half_number_of_bkg_datapoints, vec_bkg.begin());
  std::copy(vec_y.end() - half_number_of_bkg_datapoints, vec_y.end(), vec_bkg.begin() + half_number_of_bkg_datapoints);

  // estimate the noise as the standard deviation of the combined background vector, but without outliers
  std::vector<double> zscore_vec = Kernel::getZscore(vec_bkg);
  std::vector<double> vec_bkg_no_outliers;
  vec_bkg_no_outliers.resize(vec_bkg.size());
  double zscore_crit = 3.; // using three-sigma rule
  for (size_t ii = 0; ii < vec_bkg.size(); ii++) {
    if (zscore_vec[ii] <= zscore_crit)
      vec_bkg_no_outliers.push_back(vec_bkg[ii]);
  }

  if (vec_bkg_no_outliers.size() < half_number_of_bkg_datapoints)
    return DBL_MIN; // can't estimate the noise

  auto intensityStatistics = Kernel::getStatistics(vec_bkg_no_outliers, StatOptions::CorrectedStdDev);
  return intensityStatistics.standard_deviation;
}

//----------------------------------------------------------------------------------------------
/** Convert vector range boundaries to index boundaries
 * @param elems :: vector-like container
 * @param range_left :: left range boundary
 * @param range_right :: right range boundary
 * @param left_index :: (output) left index boundary
 * @param right_index :: (output) right index boundary
 */
template <typename vector_like>
void rangeToIndexBounds(const vector_like &elems, const double range_left, const double range_right, size_t &left_index,
                        size_t &right_index) {
  const auto left_iter = std::lower_bound(elems.cbegin(), elems.cend(), range_left);
  const auto right_iter = std::upper_bound(elems.cbegin(), elems.cend(), range_right);

  left_index = std::distance(elems.cbegin(), left_iter);
  right_index = std::distance(elems.cbegin(), right_iter);
  right_index = std::min(right_index, elems.size() - 1);
}

//----------------------------------------------------------------------------------------------
/** Subtract background from Y-values with given background function
 * @param bkgd_func :: background function pointer
 * @param vec_x :: vector of X-values
 * @param vec_y :: (input/output) vector of Y-values to be reduced by background function
 */
void reduceByBackground(const API::IBackgroundFunction_sptr &bkgd_func, const std::vector<double> &vec_x,
                        std::vector<double> &vec_y) {
  // calculate the background
  FunctionDomain1DVector vectorx(vec_x.begin(), vec_x.end());
  FunctionValues vector_bkgd(vectorx);
  bkgd_func->function(vectorx, vector_bkgd);

  // subtract the background from the supplied data
  for (size_t i = 0; i < vec_y.size(); ++i) {
    (vec_y)[i] -= vector_bkgd[i];
    // Note, E is not changed here
  }
}

//----------------------------------------------------------------------------------------------
/** Temporarily suspend the algorithm logging offset within the scope of a method where this sentry
 * is instantiated.
 */
class LoggingOffsetSentry {
public:
  LoggingOffsetSentry(Algorithm *const alg) : m_alg(alg) {
    m_loggingOffset = m_alg->getLoggingOffset();
    m_alg->setLoggingOffset(0);
  }
  ~LoggingOffsetSentry() { m_alg->setLoggingOffset(m_loggingOffset); }

private:
  Algorithm *const m_alg;
  int m_loggingOffset;
};
} // namespace

//----------------------------------------------------------------------------------------------
/** Fit peaks across one single spectrum
 */
void FitPeaks::fitSpectrumPeaks(size_t wi, const std::vector<double> &expected_peak_centers,
                                const std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> &fit_result,
                                std::vector<std::vector<double>> &lastGoodPeakParameters,
                                const std::shared_ptr<FitPeaksAlgorithm::PeakFitPreCheckResult> &pre_check_result) {
  assert(fit_result->getNumberPeaks() == m_numPeaksToFit);
  pre_check_result->setNumberOfSubmittedSpectrumPeaks(m_numPeaksToFit);
  // if the whole spectrum has low count, do not fit any peaks for that spectrum
  if (m_minPeakTotalCount >= 0. && numberCounts(wi) <= m_minPeakTotalCount) {
    for (size_t i = 0; i < m_numPeaksToFit; ++i)
      fit_result->setBadRecord(i, -1.);
    pre_check_result->setNumberOfSpectrumPeaksWithLowCount(m_numPeaksToFit);
    return;
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

  // Clone background function
  IBackgroundFunction_sptr bkgdfunction = std::dynamic_pointer_cast<API::IBackgroundFunction>(m_bkgdFunction->clone());

  // set up properties of algorithm (reference) 'Fit'
  peak_fitter->setProperty("Minimizer", m_minimizer);
  peak_fitter->setProperty("CostFunction", m_costFunction);
  peak_fitter->setProperty("CalcErrors", true);

  const double x0 = m_inputMatrixWS->histogram(wi).x().front();
  const double xf = m_inputMatrixWS->histogram(wi).x().back();

  // index of previous peak in same spectrum (initially invalid)
  size_t prev_peak_index = m_numPeaksToFit;
  bool neighborPeakSameSpectrum = false;
  size_t number_of_out_of_range_peaks{0};
  for (size_t fit_index = 0; fit_index < m_numPeaksToFit; ++fit_index) {
    // convert fit index to peak index (in ascending order)
    size_t peak_index(fit_index);
    if (m_fitPeaksFromRight)
      peak_index = m_numPeaksToFit - fit_index - 1;

    // reset the background function
    for (size_t i = 0; i < bkgdfunction->nParams(); ++i)
      bkgdfunction->setParameter(i, 0.);

    double expected_peak_pos = expected_peak_centers[peak_index];

    // clone peak function for each peak (need to do this so can
    // set center and calc any parameters from xml)
    auto peakfunction = std::dynamic_pointer_cast<API::IPeakFunction>(m_peakFunction->clone());
    peakfunction->setCentre(expected_peak_pos);
    peakfunction->setMatrixWorkspace(m_inputMatrixWS, wi, 0.0, 0.0);

    std::map<size_t, double> keep_values;
    for (size_t ipar = 0; ipar < peakfunction->nParams(); ++ipar) {
      if (peakfunction->isFixed(ipar)) {
        // save value of these parameters which have just been calculated
        // if they were set to be fixed (e.g. for the B2Bexp this would
        // typically be A and B but not Sigma)
        keep_values[ipar] = peakfunction->getParameter(ipar);
        // let them be free to fit as these are typically refined from a
        // focussed bank
        peakfunction->unfix(ipar);
      }
    }

    // Determine whether to set starting parameter from fitted value
    // of same peak but different spectrum
    bool samePeakCrossSpectrum = (lastGoodPeakParameters[peak_index].size() >
                                  static_cast<size_t>(std::count_if(lastGoodPeakParameters[peak_index].begin(),
                                                                    lastGoodPeakParameters[peak_index].end(),
                                                                    [&](auto const &val) { return val <= 1e-10; })));

    // Check whether current spectrum's pixel (detector ID) is close to its
    // previous spectrum's pixel (detector ID).
    try {
      if (wi > 0 && samePeakCrossSpectrum) {
        // First spectrum or discontinuous detector ID: do not start from same
        // peak of last spectrum
        std::shared_ptr<const Geometry::Detector> pdetector =
            std::dynamic_pointer_cast<const Geometry::Detector>(m_inputMatrixWS->getDetector(wi - 1));
        std::shared_ptr<const Geometry::Detector> cdetector =
            std::dynamic_pointer_cast<const Geometry::Detector>(m_inputMatrixWS->getDetector(wi));

        // If they do have detector ID
        if (pdetector && cdetector) {
          auto prev_id = pdetector->getID();
          auto curr_id = cdetector->getID();
          if (prev_id + 1 != curr_id)
            samePeakCrossSpectrum = false;
        } else {
          samePeakCrossSpectrum = false;
        }

      } else {
        // first spectrum in the workspace: no peak's fitting result to copy
        // from
        samePeakCrossSpectrum = false;
      }
    } catch (const std::runtime_error &) {
      // workspace does not have detector ID set: there is no guarantee that the
      // adjacent spectra can have similar peak profiles
      samePeakCrossSpectrum = false;
    }

    // Set starting values of the peak function
    if (samePeakCrossSpectrum) { // somePeakFit
      // Get from local best result
      for (size_t i = 0; i < peakfunction->nParams(); ++i) {
        peakfunction->setParameter(i, lastGoodPeakParameters[peak_index][i]);
      }
    } else if (neighborPeakSameSpectrum) {
      // set the peak parameters from last good fit to that peak
      for (size_t i = 0; i < peakfunction->nParams(); ++i) {
        peakfunction->setParameter(i, lastGoodPeakParameters[prev_peak_index][i]);
      }
    }

    // reset center though - don't know before hand which element this is
    peakfunction->setCentre(expected_peak_pos);
    // reset value of parameters that were fixed (but are now free to vary)
    for (const auto &[ipar, value] : keep_values) {
      peakfunction->setParameter(ipar, value);
    }

    double cost(DBL_MAX);
    if (expected_peak_pos <= x0 || expected_peak_pos >= xf) {
      // out of range and there won't be any fit
      peakfunction->setIntensity(0);
      number_of_out_of_range_peaks++;
    } else {
      // find out the peak position to fit
      std::pair<double, double> peak_window_i = m_getPeakFitWindow(wi, peak_index);

      // Decide whether to estimate peak width by observation
      // If no peaks fitted in the same or cross spectrum then the user supplied
      // parameters will be used if present and the width will not be estimated
      // (note this will overwrite parameter values caluclated from
      // Parameters.xml)
      auto useUserSpecifedIfGiven = !(samePeakCrossSpectrum || neighborPeakSameSpectrum);
      bool observe_peak_width = decideToEstimatePeakParams(useUserSpecifedIfGiven, peakfunction);

      if (observe_peak_width && m_peakWidthEstimateApproach == EstimatePeakWidth::NoEstimation) {
        g_log.warning("Peak width can be estimated as ZERO.  The result can be wrong");
      }

      // do fitting with peak and background function (no analysis at this point)
      std::shared_ptr<FitPeaksAlgorithm::PeakFitPreCheckResult> peak_pre_check_result =
          std::make_shared<FitPeaksAlgorithm::PeakFitPreCheckResult>();
      cost = fitIndividualPeak(wi, peak_fitter, expected_peak_pos, peak_window_i, observe_peak_width, peakfunction,
                               bkgdfunction, peak_pre_check_result);
      if (peak_pre_check_result->isIndividualPeakRejected())
        fit_result->setBadRecord(peak_index, -1.);

      if (m_minSignalToSigmaRatio > 0) {
        if (calculateSignalToSigmaRatio(wi, peak_window_i, peakfunction) < m_minSignalToSigmaRatio) {
          fit_result->setBadRecord(peak_index, -1.);
          cost = DBL_MAX;
        }
      }

      *pre_check_result += *peak_pre_check_result; // keep track of the rejection count within the spectrum
    }
    pre_check_result->setNumberOfOutOfRangePeaks(number_of_out_of_range_peaks);

    // process fitting result
    FitPeaksAlgorithm::FitFunction fit_function;
    fit_function.peakfunction = peakfunction;
    fit_function.bkgdfunction = bkgdfunction;

    auto good_fit = processSinglePeakFitResult(wi, peak_index, cost, expected_peak_centers, fit_function,
                                               fit_result); // sets the record

    if (good_fit) {
      // reset the flag such that there is at a peak fit in this spectrum
      neighborPeakSameSpectrum = true;
      prev_peak_index = peak_index;
      // copy values
      for (size_t i = 0; i < lastGoodPeakParameters[peak_index].size(); ++i) {
        lastGoodPeakParameters[peak_index][i] = peakfunction->getParameter(i);
      }
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Decide whether to estimate peak parameters. If not, then set the peak
 * parameters from
 * user specified starting value
 * @param firstPeakInSpectrum :: flag whether the given peak is the first peak
 * in the spectrum
 * @param peak_function :: peak function to set parameter values to
 * @return :: flag whether the peak width shall be observed
 */
bool FitPeaks::decideToEstimatePeakParams(const bool firstPeakInSpectrum,
                                          const API::IPeakFunction_sptr &peak_function) {
  // should observe the peak width if the user didn't supply all of the peak
  // function parameters
  bool observe_peak_shape(m_initParamIndexes.size() != peak_function->nParams());

  if (!m_initParamIndexes.empty()) {
    // user specifies starting value of peak parameters
    if (firstPeakInSpectrum) {
      // set the parameter values in a vector and loop over it
      // first peak.  using the user-specified value
      for (size_t i = 0; i < m_initParamIndexes.size(); ++i) {
        const size_t param_index = m_initParamIndexes[i];
        const double param_value = m_initParamValues[i];
        peak_function->setParameter(param_index, param_value);
      }
    } else {
      // using the fitted paramters from the previous fitting result
      // do noting
    }
  } else {
    // no previously defined peak parameters: observation is thus required
    observe_peak_shape = true;
  }

  return observe_peak_shape;
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
 * @return :: whether the peak fiting is good or not
 */
bool FitPeaks::processSinglePeakFitResult(size_t wsindex, size_t peakindex, const double cost,
                                          const std::vector<double> &expected_peak_positions,
                                          const FitPeaksAlgorithm::FitFunction &fitfunction,
                                          const std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> &fit_result) {
  // determine peak position tolerance
  double postol(DBL_MAX);
  bool case23(false);
  if (m_peakPosTolCase234) {
    // peak tolerance is not defined
    if (m_numPeaksToFit == 1) {
      // case (d) one peak only
      postol = m_inputMatrixWS->histogram(wsindex).x().back() - m_inputMatrixWS->histogram(wsindex).x().front();
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
  auto peak_pos = fitfunction.peakfunction->centre();
  auto peak_fwhm = fitfunction.peakfunction->fwhm();
  bool good_fit(false);
  if ((cost < 0) || (cost >= DBL_MAX - 1.) || std::isnan(cost)) {
    // unphysical cost function value
    peak_pos = -4;
  } else if (fitfunction.peakfunction->height() < m_minPeakHeight) {
    // peak height is under minimum request
    peak_pos = -3;
  } else if (case23) {
    // case b and c to check peak position without defined peak tolerance
    std::pair<double, double> fitwindow = m_getPeakFitWindow(wsindex, peakindex);
    if (fitwindow.first < fitwindow.second) {
      // peak fit window is specified or calculated: use peak window as position
      // tolerance
      if (peak_pos < fitwindow.first || peak_pos > fitwindow.second) {
        // peak is out of fit window
        peak_pos = -2;
        g_log.debug() << "Peak position " << peak_pos << " is out of fit "
                      << "window boundary " << fitwindow.first << ", " << fitwindow.second << "\n";
      } else if (peak_fwhm > (fitwindow.second - fitwindow.first)) {
        // peak is too wide or window is too small
        peak_pos = -2.25;
        g_log.debug() << "Peak position " << peak_pos << " has fwhm "
                      << "wider than the fit window " << fitwindow.second - fitwindow.first << "\n";
      } else {
        good_fit = true;
      }
    } else {
      // use the 1/2 distance to neiboring peak without defined peak window
      double left_bound(-1);
      if (peakindex > 0)
        left_bound = 0.5 * (expected_peak_positions[peakindex] - expected_peak_positions[peakindex - 1]);
      double right_bound(-1);
      if (peakindex < m_numPeaksToFit - 1)
        right_bound = 0.5 * (expected_peak_positions[peakindex + 1] - expected_peak_positions[peakindex]);
      if (left_bound < 0)
        left_bound = right_bound;
      if (right_bound < left_bound)
        right_bound = left_bound;
      if (left_bound < 0 || right_bound < 0)
        throw std::runtime_error("Code logic error such that left or right "
                                 "boundary of peak position is negative.");
      if (peak_pos < left_bound || peak_pos > right_bound) {
        peak_pos = -2.5;
      } else if (peak_fwhm > (right_bound - left_bound)) {
        // peak is too wide or window is too small
        peak_pos = -2.75;
        g_log.debug() << "Peak position " << peak_pos << " has fwhm "
                      << "wider than the fit window " << right_bound - left_bound << "\n";
      } else {
        good_fit = true;
      }
    }
  } else if (fabs(fitfunction.peakfunction->centre() - expected_peak_positions[peakindex]) > postol) {
    // peak center is not within tolerance
    peak_pos = -5;
    g_log.debug() << "Peak position difference "
                  << fabs(fitfunction.peakfunction->centre() - expected_peak_positions[peakindex])
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

  return good_fit;
}

//----------------------------------------------------------------------------------------------
/** calculate fitted peaks with background in the output workspace
 * The current version gets the peak parameters and background parameters from
 * fitted parameter
 * table
 */
void FitPeaks::calculateFittedPeaks(const std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> &fit_results) {
  // check
  if (!m_fittedParamTable)
    throw std::runtime_error("No parameters");

  const size_t num_peakfunc_params = m_peakFunction->nParams();
  const size_t num_bkgdfunc_params = m_bkgdFunction->nParams();

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_fittedPeakWS))
  for (int64_t iiws = m_startWorkspaceIndex; iiws <= static_cast<int64_t>(m_stopWorkspaceIndex); ++iiws) {
    PARALLEL_START_INTERRUPT_REGION
    std::size_t iws = static_cast<std::size_t>(iiws);
    // get a copy of peak function and background function
    IPeakFunction_sptr peak_function = std::dynamic_pointer_cast<IPeakFunction>(m_peakFunction->clone());
    IBackgroundFunction_sptr bkgd_function = std::dynamic_pointer_cast<IBackgroundFunction>(m_bkgdFunction->clone());
    std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> fit_result_i = fit_results[iws - m_startWorkspaceIndex];
    // FIXME - This is a just a pure check
    if (!fit_result_i)
      throw std::runtime_error("There is something wroing with PeakFitResult vector!");

    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      // get and set the peak function parameters
      const double chi2 = fit_result_i->getCost(ipeak);
      if (chi2 > 10.e10)
        continue;

      for (size_t iparam = 0; iparam < num_peakfunc_params; ++iparam)
        peak_function->setParameter(iparam, fit_result_i->getParameterValue(ipeak, iparam));
      for (size_t iparam = 0; iparam < num_bkgdfunc_params; ++iparam)
        bkgd_function->setParameter(iparam, fit_result_i->getParameterValue(ipeak, num_peakfunc_params + iparam));
      // use domain and function to calcualte
      // get the range of start and stop to construct a function domain
      const auto &vec_x = m_fittedPeakWS->points(iws);
      std::pair<double, double> peakwindow = m_getPeakFitWindow(iws, ipeak);
      auto start_x_iter = std::lower_bound(vec_x.begin(), vec_x.end(), peakwindow.first);
      auto stop_x_iter = std::lower_bound(vec_x.begin(), vec_x.end(), peakwindow.second);

      if (start_x_iter == stop_x_iter)
        throw std::runtime_error("Range size is zero in calculateFittedPeaks");

      FunctionDomain1DVector domain(start_x_iter, stop_x_iter);
      FunctionValues values(domain);
      CompositeFunction_sptr comp_func = std::make_shared<API::CompositeFunction>();
      comp_func->addFunction(peak_function);
      comp_func->addFunction(bkgd_function);
      comp_func->function(domain, values);

      // copy over the values
      std::size_t istart = static_cast<size_t>(start_x_iter - vec_x.begin());
      std::size_t istop = static_cast<size_t>(stop_x_iter - vec_x.begin());
      for (std::size_t yindex = istart; yindex < istop; ++yindex) {
        m_fittedPeakWS->dataY(iws)[yindex] = values.getCalculated(yindex - istart);
      }
    } // END-FOR (ipeak)
    PARALLEL_END_INTERRUPT_REGION
  } // END-FOR (iws)
  PARALLEL_CHECK_INTERRUPT_REGION

  return;
}

double FitPeaks::calculateSignalToSigmaRatio(const size_t &iws, const std::pair<double, double> &peakWindow,
                                             const API::IPeakFunction_sptr &peakFunction) {
  const auto &vecX = m_inputMatrixWS->points(iws);
  auto startX = std::lower_bound(vecX.begin(), vecX.end(), peakWindow.first);
  auto stopX = std::lower_bound(vecX.begin(), vecX.end(), peakWindow.second);

  FunctionDomain1DVector domain(startX, stopX);
  FunctionValues values(domain);

  peakFunction->function(domain, values);
  auto peakValues = values.toVector();

  const auto &errors = m_inputMatrixWS->readE(iws);
  auto startE = errors.begin() + (startX - vecX.begin());
  auto stopE = errors.begin() + (stopX - vecX.begin());
  std::vector<double> peakErrors(startE, stopE);

  double peakSum = std::accumulate(peakValues.cbegin(), peakValues.cend(), 0.0);
  double sigma = sqrt(std::accumulate(peakErrors.cbegin(), peakErrors.cend(), 0.0, VectorHelper::SumSquares<double>()));

  return peakSum / ((sigma == 0) ? 1 : sigma);
}

namespace {
bool estimateBackgroundParameters(const Histogram &histogram, const std::pair<size_t, size_t> &peak_window,
                                  const API::IBackgroundFunction_sptr &bkgd_function) {
  // for estimating background parameters
  // 0 = constant, 1 = linear
  const auto POLYNOMIAL_ORDER = std::min<size_t>(1, bkgd_function->nParams());

  if (peak_window.first >= peak_window.second)
    throw std::runtime_error("Invalid peak window");

  // reset the background function
  const auto nParams = bkgd_function->nParams();
  for (size_t i = 0; i < nParams; ++i)
    bkgd_function->setParameter(i, 0.);

  // 10 is a magic number that worked in a variety of situations
  const size_t iback_start = peak_window.first + 10;
  const size_t iback_stop = peak_window.second - 10;

  // use the simple way to find linear background
  // there aren't enough bins in the window to try to estimate so just leave the
  // estimate at zero
  if (iback_start < iback_stop) {
    double bkgd_a0{0.};    // will be fit
    double bkgd_a1{0.};    // may be fit
    double bkgd_a2{0.};    // will be ignored
    double chisq{DBL_MAX}; // how well the fit worked
    HistogramData::estimateBackground(POLYNOMIAL_ORDER, histogram, peak_window.first, peak_window.second, iback_start,
                                      iback_stop, bkgd_a0, bkgd_a1, bkgd_a2, chisq);
    // update the background function with the result
    bkgd_function->setParameter(0, bkgd_a0);
    if (nParams > 1)
      bkgd_function->setParameter(1, bkgd_a1);
    // quadratic term is always estimated to be zero

    // TODO: return false if chisq is too large
    return true;
  }

  return false; // too few data points for the fit
}
} // anonymous namespace

//----------------------------------------------------------------------------------------------
/** check whether a peak profile is allowed to observe peak width and set width
 * @brief isObservablePeakProfile
 * @param peakprofile : name of peak profile to check against
 * @return :: flag whether the specified peak profile observable
 */
bool FitPeaks::isObservablePeakProfile(const std::string &peakprofile) {
  return (std::find(supported_peak_profiles.begin(), supported_peak_profiles.end(), peakprofile) !=
          supported_peak_profiles.end());
}

//----------------------------------------------------------------------------------------------
/** Fit background function
 */
bool FitPeaks::fitBackground(const size_t &ws_index, const std::pair<double, double> &fit_window,
                             const double &expected_peak_pos, const API::IBackgroundFunction_sptr &bkgd_func) {
  constexpr size_t MIN_POINTS{10}; // TODO explain why 10

  // find out how to fit background
  const auto &points = m_inputMatrixWS->histogram(ws_index).points();
  size_t start_index = findXIndex(points.rawData(), fit_window.first);
  size_t expected_peak_index = findXIndex(points.rawData(), expected_peak_pos, start_index);
  size_t stop_index = findXIndex(points.rawData(), fit_window.second, expected_peak_index);

  // treat 5 as a magic number - TODO explain why
  bool good_fit(false);
  if (expected_peak_index - start_index > MIN_POINTS && stop_index - expected_peak_index > MIN_POINTS) {
    // enough data points left for multi-domain fitting
    // set a smaller fit window
    const std::pair<double, double> vec_min{fit_window.first, points[expected_peak_index + 5]};
    const std::pair<double, double> vec_max{points[expected_peak_index - 5], fit_window.second};

    // reset background function value
    for (size_t n = 0; n < bkgd_func->nParams(); ++n)
      bkgd_func->setParameter(n, 0);

    double chi2 = fitFunctionMD(bkgd_func, m_inputMatrixWS, ws_index, vec_min, vec_max);

    // process
    if (chi2 < DBL_MAX - 1) {
      good_fit = true;
    }

  } else {
    // fit as a single domain function.  check whether the result is good or bad

    // TODO FROM HERE!
    g_log.debug() << "Don't know what to do with background fitting with single "
                  << "domain function! " << (expected_peak_index - start_index) << " points to the left "
                  << (stop_index - expected_peak_index) << " points to the right\n";
  }

  return good_fit;
}

//----------------------------------------------------------------------------------------------
/** Fit an individual peak
 */
double FitPeaks::fitIndividualPeak(size_t wi, const API::IAlgorithm_sptr &fitter, const double expected_peak_center,
                                   const std::pair<double, double> &fitwindow, const bool estimate_peak_width,
                                   const API::IPeakFunction_sptr &peakfunction,
                                   const API::IBackgroundFunction_sptr &bkgdfunc,
                                   const std::shared_ptr<FitPeaksAlgorithm::PeakFitPreCheckResult> &pre_check_result) {
  pre_check_result->setNumberOfSubmittedIndividualPeaks(1);
  double cost(DBL_MAX);

  // make sure the number of data points satisfies the number of fitting parameters plus a magic cushion of 2.
  size_t min_required_datapoints{peakfunction->nParams() + bkgdfunc->nParams() + 2};
  size_t number_of_datapoints = histRangeToDataPointCount(wi, fitwindow);
  if (number_of_datapoints < min_required_datapoints) {
    pre_check_result->setNumberOfPeaksWithNotEnoughDataPoints(1);
    return cost;
  }

  // check the number of counts in the peak window
  if (m_minPeakTotalCount >= 0.0 && numberCounts(wi, fitwindow) <= m_minPeakTotalCount) {
    pre_check_result->setNumberOfIndividualPeaksWithLowCount(1);
    return cost;
  }

  // exclude a peak with a low signal-to-noise ratio
  if (m_minSignalToNoiseRatio > 0.0 && calculateSignalToNoiseRatio(wi, fitwindow, bkgdfunc) < m_minSignalToNoiseRatio) {
    pre_check_result->setNumberOfPeaksWithLowSignalToNoise(1);
    return cost;
  }

  if (m_highBackground) {
    // fit peak with high background!
    cost = fitFunctionHighBackground(fitter, fitwindow, wi, expected_peak_center, estimate_peak_width, peakfunction,
                                     bkgdfunc);
  } else {
    // fit peak and background
    cost = fitFunctionSD(fitter, peakfunction, bkgdfunc, m_inputMatrixWS, wi, fitwindow, expected_peak_center,
                         estimate_peak_width, true);
  }

  return cost;
}

//----------------------------------------------------------------------------------------------
/** Fit function in single domain (mostly applied for fitting peak + background)
 * with estimating peak parameters
 * This is the core fitting algorithm to deal with the simplest situation
 * @exception :: Fit.isExecuted is false (cannot be executed)
 */
double FitPeaks::fitFunctionSD(const IAlgorithm_sptr &fit, const API::IPeakFunction_sptr &peak_function,
                               const API::IBackgroundFunction_sptr &bkgd_function,
                               const API::MatrixWorkspace_sptr &dataws, size_t wsindex,
                               const std::pair<double, double> &peak_range, const double &expected_peak_center,
                               bool estimate_peak_width, bool estimate_background) {
  std::stringstream errorid;
  errorid << "(WorkspaceIndex=" << wsindex << " PeakCentre=" << expected_peak_center << ")";

  // validate peak window
  if (peak_range.first >= peak_range.second) {
    std::stringstream msg;
    msg << "Invalid peak window: xmin>xmax (" << peak_range.first << ", " << peak_range.second << ")" << errorid.str();
    throw std::runtime_error(msg.str());
  }

  // determine the peak window in terms of vector indexes
  const auto &histogram = dataws->histogram(wsindex);
  const auto &vector_x = histogram.points();
  const auto start_index = findXIndex(vector_x, peak_range.first);
  const auto stop_index = findXIndex(vector_x, peak_range.second, start_index);
  if (start_index == stop_index)
    throw std::runtime_error("Range size is zero in fitFunctionSD");
  std::pair<size_t, size_t> peak_index_window = std::make_pair(start_index, stop_index);

  // Estimate background
  if (estimate_background) {
    if (!estimateBackgroundParameters(histogram, peak_index_window, bkgd_function)) {
      return DBL_MAX;
    }
  }

  // Estimate peak profile parameter
  peak_function->setCentre(expected_peak_center); // set expected position first
  int result = estimatePeakParameters(histogram, peak_index_window, peak_function, bkgd_function, estimate_peak_width,
                                      m_peakWidthEstimateApproach, m_peakWidthPercentage, m_minPeakHeight);

  if (result != GOOD) {
    peak_function->setCentre(expected_peak_center);
    if (result == NOSIGNAL || result == LOWPEAK) {
      return DBL_MAX; // exit early - don't fit
    }
  }

  // Create the composition function
  CompositeFunction_sptr comp_func = std::make_shared<API::CompositeFunction>();
  comp_func->addFunction(peak_function);
  comp_func->addFunction(bkgd_function);
  IFunction_sptr fitfunc = std::dynamic_pointer_cast<IFunction>(comp_func);

  // Set the properties
  fit->setProperty("Function", fitfunc);
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("MaxIterations", m_fitIterations); // magic number
  fit->setProperty("StartX", peak_range.first);
  fit->setProperty("EndX", peak_range.second);
  fit->setProperty("IgnoreInvalidData", true);

  if (m_constrainPeaksPosition) {
    // set up a constraint on peak position
    double peak_center = peak_function->centre();
    double peak_width = peak_function->fwhm();
    std::stringstream peak_center_constraint;
    peak_center_constraint << (peak_center - 0.5 * peak_width) << " < f0." << peak_function->getCentreParameterName()
                           << " < " << (peak_center + 0.5 * peak_width);
    fit->setProperty("Constraints", peak_center_constraint.str());
  }

  // Execute fit and get result of fitting background
  g_log.debug() << "[E1201] FitSingleDomain Before fitting, Fit function: " << fit->asString() << "\n";
  errorid << " starting function [" << comp_func->asString() << "]";
  try {
    fit->execute();
    g_log.debug() << "[E1202] FitSingleDomain After fitting, Fit function: " << fit->asString() << "\n";

    if (!fit->isExecuted()) {
      g_log.warning() << "Fitting peak SD (single domain) failed to execute. " + errorid.str();
      return DBL_MAX;
    }
  } catch (std::invalid_argument &e) {
    errorid << ": " << e.what();
    g_log.warning() << "\nWhile fitting " + errorid.str();
    return DBL_MAX; // probably the wrong thing to do
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
double FitPeaks::fitFunctionMD(API::IFunction_sptr fit_function, const API::MatrixWorkspace_sptr &dataws,
                               const size_t wsindex, const std::pair<double, double> &vec_xmin,
                               const std::pair<double, double> &vec_xmax) {
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
  std::shared_ptr<MultiDomainFunction> md_function = std::make_shared<MultiDomainFunction>();

  // Set function first
  md_function->addFunction(std::move(fit_function));

  //  set domain for function with index 0 covering both sides
  md_function->clearDomainIndices();
  md_function->setDomainIndices(0, {0, 1});

  // Set the properties
  fit->setProperty("Function", std::dynamic_pointer_cast<IFunction>(md_function));
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("StartX", vec_xmin.first);
  fit->setProperty("EndX", vec_xmax.first);
  fit->setProperty("InputWorkspace_1", dataws);
  fit->setProperty("WorkspaceIndex_1", static_cast<int>(wsindex));
  fit->setProperty("StartX_1", vec_xmin.second);
  fit->setProperty("EndX_1", vec_xmax.second);
  fit->setProperty("MaxIterations", m_fitIterations);
  fit->setProperty("IgnoreInvalidData", true);

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
/// Fit peak with high background
double FitPeaks::fitFunctionHighBackground(const IAlgorithm_sptr &fit, const std::pair<double, double> &fit_window,
                                           const size_t &ws_index, const double &expected_peak_center,
                                           bool observe_peak_shape, const API::IPeakFunction_sptr &peakfunction,
                                           const API::IBackgroundFunction_sptr &bkgdfunc) {
  assert(m_linearBackgroundFunction);

  // high background to reduce
  API::IBackgroundFunction_sptr high_bkgd_function =
      std::dynamic_pointer_cast<API::IBackgroundFunction>(m_linearBackgroundFunction->clone());

  // Fit the background first if there is enough data points
  fitBackground(ws_index, fit_window, expected_peak_center, high_bkgd_function);

  // Get partial of the data
  std::vector<double> vec_x, vec_y, vec_e;
  getRangeData(ws_index, fit_window, vec_x, vec_y, vec_e);

  // Reduce the background
  reduceByBackground(high_bkgd_function, vec_x, vec_y);
  for (std::size_t n = 0; n < bkgdfunc->nParams(); ++n)
    bkgdfunc->setParameter(n, 0);

  // Create a new workspace
  API::MatrixWorkspace_sptr reduced_bkgd_ws = createMatrixWorkspace(vec_x, vec_y, vec_e);

  // Fit peak with background
  fitFunctionSD(fit, peakfunction, bkgdfunc, reduced_bkgd_ws, 0, {vec_x.front(), vec_x.back()}, expected_peak_center,
                observe_peak_shape, false);

  // add the reduced background back
  bkgdfunc->setParameter(0, bkgdfunc->getParameter(0) + high_bkgd_function->getParameter(0));
  bkgdfunc->setParameter(1, bkgdfunc->getParameter(1) + // TODO doesn't work for flat background
                                high_bkgd_function->getParameter(1));

  double cost = fitFunctionSD(fit, peakfunction, bkgdfunc, m_inputMatrixWS, ws_index, {vec_x.front(), vec_x.back()},
                              expected_peak_center, false, false);

  return cost;
}

//----------------------------------------------------------------------------------------------
/// Create a single spectrum workspace for fitting
API::MatrixWorkspace_sptr FitPeaks::createMatrixWorkspace(const std::vector<double> &vec_x,
                                                          const std::vector<double> &vec_y,
                                                          const std::vector<double> &vec_e) {
  std::size_t size = vec_x.size();
  std::size_t ysize = vec_y.size();

  HistogramBuilder builder;
  builder.setX(size);
  builder.setY(ysize);
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
  m_outputPeakPositionWorkspace = create<Workspace2D>(m_numSpectraToFit, Points(m_numPeaksToFit));
  // set default
  for (std::size_t wi = 0; wi < m_numSpectraToFit; ++wi) {
    // convert to workspace index of input data workspace
    std::size_t inp_wi = wi + m_startWorkspaceIndex;
    std::vector<double> expected_position = m_getExpectedPeakPositions(inp_wi);
    for (std::size_t ipeak = 0; ipeak < expected_position.size(); ++ipeak) {
      m_outputPeakPositionWorkspace->dataX(wi)[ipeak] = expected_position[ipeak];
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
void FitPeaks::setupParameterTableWorkspace(const API::ITableWorkspace_sptr &table_ws,
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
  m_rawPeaksTable = getProperty(PropertyNames::RAW_PARAMS);

  // create parameters
  // peak
  std::vector<std::string> param_vec;
  if (m_rawPeaksTable) {
    param_vec = m_peakFunction->getParameterNames();
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
  m_fittedParamTable = std::make_shared<TableWorkspace>();
  setupParameterTableWorkspace(m_fittedParamTable, param_vec, true);

  // for error workspace
  std::string fiterror_table_name = getPropertyValue(PropertyNames::OUTPUT_WKSP_PARAM_ERRS);
  // do nothing if user does not specifiy
  if (fiterror_table_name.empty()) {
    // not specified
    m_fitErrorTable = nullptr;
  } else {
    // create table and set up parameter table
    m_fitErrorTable = std::make_shared<TableWorkspace>();
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
  std::string fit_ws_name = getPropertyValue(PropertyNames::OUTPUT_WKSP_MODEL);
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
void FitPeaks::processOutputs(std::vector<std::shared_ptr<FitPeaksAlgorithm::PeakFitResult>> fit_result_vec) {
  setProperty(PropertyNames::OUTPUT_WKSP, m_outputPeakPositionWorkspace);
  setProperty(PropertyNames::OUTPUT_WKSP_PARAMS, m_fittedParamTable);

  if (m_fitErrorTable) {
    g_log.warning("Output error table workspace");
    setProperty(PropertyNames::OUTPUT_WKSP_PARAM_ERRS, m_fitErrorTable);
  } else {
    g_log.warning("No error table output");
  }

  // optional
  if (m_fittedPeakWS && m_fittedParamTable) {
    g_log.debug("about to calcualte fitted peaks");
    calculateFittedPeaks(std::move(fit_result_vec));
    setProperty(PropertyNames::OUTPUT_WKSP_MODEL, m_fittedPeakWS);
  }
}

//----------------------------------------------------------------------------------------------
/** Sum up all counts in a histogram
 * @param iws ::  histogram index in workspace
 * @return :: total number of counts in the histogram
 */
double FitPeaks::numberCounts(size_t iws) {
  const std::vector<double> &vec_y = m_inputMatrixWS->histogram(iws).y().rawData();
  double total = std::accumulate(vec_y.begin(), vec_y.end(), 0.);
  return total;
}

//----------------------------------------------------------------------------------------------
/** Sum up all counts in a histogram range
 * @param iws :: histogram index in workspace
 * @param range :: histogram range
 * @return :: total number of counts in the range
 */
double FitPeaks::numberCounts(size_t iws, const std::pair<double, double> &range) {
  // get data range
  std::vector<double> vec_x, vec_y, vec_e;
  getRangeData(iws, range, vec_x, vec_y, vec_e);
  // sum up all counts
  double total = std::accumulate(vec_y.begin(), vec_y.end(), 0.);
  return total;
}

//----------------------------------------------------------------------------------------------
/** Calculate number of data points in a histogram range
 * @param iws :: histogram index in workspace
 * @param range :: histogram range
 * @return :: number of data points
 */
size_t FitPeaks::histRangeToDataPointCount(size_t iws, const std::pair<double, double> &range) {
  size_t left_index, right_index;
  histRangeToIndexBounds(iws, range, left_index, right_index);
  size_t number_dp = right_index - left_index + 1;
  if (m_inputMatrixWS->isHistogramData())
    number_dp -= 1;
  assert(number_dp > 0);
  return number_dp;
}

GNU_DIAG_OFF("dangling-reference")

//----------------------------------------------------------------------------------------------
/** Convert a histogram range to vector index boundaries
 * @param iws :: histogram index in workspace
 * @param range :: histogram range
 * @param left_index :: (output) left index boundary
 * @param right_index :: (output) right index boundary
 */
void FitPeaks::histRangeToIndexBounds(size_t iws, const std::pair<double, double> &range, size_t &left_index,
                                      size_t &right_index) {
  const auto &orig_x = m_inputMatrixWS->histogram(iws).x();
  rangeToIndexBounds(orig_x, range.first, range.second, left_index, right_index);

  // handle an invalid range case. For the histogram point data, make sure the number of data points is non-zero as
  // well.
  if (left_index >= right_index || (m_inputMatrixWS->isHistogramData() && left_index == right_index - 1)) {
    std::stringstream err_ss;
    err_ss << "Unable to get a valid subset of histogram from given fit window. "
           << "Histogram X: " << orig_x.front() << "," << orig_x.back() << "; Range: " << range.first << ","
           << range.second;
    throw std::runtime_error(err_ss.str());
  }
}

//----------------------------------------------------------------------------------------------
/** get vector X, Y and E in a given range
 * @param iws :: histogram index in workspace
 * @param range :: histogram range
 * @param vec_x :: (output) vector of X-values
 * @param vec_y :: (output) vector of Y-values
 * @param vec_e :: (output) vector of E-values
 */
void FitPeaks::getRangeData(size_t iws, const std::pair<double, double> &range, std::vector<double> &vec_x,
                            std::vector<double> &vec_y, std::vector<double> &vec_e) {
  // convert range to index boundaries
  size_t left_index, right_index;
  histRangeToIndexBounds(iws, range, left_index, right_index);

  // copy X, Y and E
  size_t num_elements_x = right_index - left_index;

  vec_x.resize(num_elements_x);
  const auto &orig_x = m_inputMatrixWS->histogram(iws).x();
  std::copy(orig_x.begin() + left_index, orig_x.begin() + right_index, vec_x.begin());

  size_t num_datapoints = m_inputMatrixWS->isHistogramData() ? num_elements_x - 1 : num_elements_x;

  const std::vector<double> orig_y = m_inputMatrixWS->histogram(iws).y().rawData();
  const std::vector<double> orig_e = m_inputMatrixWS->histogram(iws).e().rawData();
  vec_y.resize(num_datapoints);
  vec_e.resize(num_datapoints);
  std::copy(orig_y.begin() + left_index, orig_y.begin() + left_index + num_datapoints, vec_y.begin());
  std::copy(orig_e.begin() + left_index, orig_e.begin() + left_index + num_datapoints, vec_e.begin());
}

GNU_DIAG_ON("dangling-reference")

//----------------------------------------------------------------------------------------------
/** Calculate signal-to-noise ratio in a histogram range
 * @param iws :: histogram index in workspace
 * @param range :: histogram range
 * @param bkgd_function :: background function pointer
 * @return :: signal-to-noise ratio
 */
double FitPeaks::calculateSignalToNoiseRatio(size_t iws, const std::pair<double, double> &range,
                                             const API::IBackgroundFunction_sptr &bkgd_function) {
  // convert range to index boundaries
  size_t left_index, right_index;
  histRangeToIndexBounds(iws, range, left_index, right_index);

  // estimate background level by Y(X) fitting
  if (!estimateBackgroundParameters(m_inputMatrixWS->histogram(iws), std::pair<size_t, size_t>(left_index, right_index),
                                    bkgd_function))
    return 0.0; // failed to estimate background parameters

  // get X,Y,and E for the data range
  std::vector<double> vec_x, vec_y, vec_e;
  getRangeData(iws, range, vec_x, vec_y, vec_e);
  if (vec_x.empty())
    return 0.0;

  // subtract background from Y-values
  reduceByBackground(bkgd_function, vec_x, vec_y);

  // estimate the signal as the highest Y-value in the data range
  auto it_max = std::max_element(vec_y.begin(), vec_y.end());
  double signal = vec_y[it_max - vec_y.begin()];
  if (signal <= DBL_MIN)
    return 0.0;

  // estimate noise from background. If noise is zero, or impossible to estimate, return DBL_MAX so that the peak
  // won't be rejected.
  double noise = estimateBackgroundNoise(vec_y);
  if (noise <= DBL_MIN)
    return DBL_MAX;

  // finally, calculate the signal-to-noise ratio
  return signal / noise;
}

//----------------------------------------------------------------------------------------------
/// Get the expected peak's position

void FitPeaks::checkWorkspaceIndices(std::size_t const &wi) {
  if (wi < m_startWorkspaceIndex || wi > m_stopWorkspaceIndex) {
    std::stringstream errss;
    errss << "Workspace index " << wi << " is out of range "
          << "[" << m_startWorkspaceIndex << ", " << m_stopWorkspaceIndex << "]";
    throw std::runtime_error(errss.str());
  }
}

void FitPeaks::checkPeakIndices(std::size_t const &wi, std::size_t const &ipeak) {
  // check peak index
  if (ipeak >= m_getExpectedPeakPositions(wi).size()) {
    std::stringstream errss;
    errss << "Peak index " << ipeak << " is out of range (" << m_numPeaksToFit << ")";
    throw std::runtime_error(errss.str());
  }
}

void FitPeaks::checkPeakWindowEdgeOrder(double const &left, double const &right) {
  if (left >= right) {
    std::stringstream errss;
    errss << "Peak window is inappropriate for workspace index: " << left << " >= " << right;
    throw std::runtime_error(errss.str());
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
void FitPeaks::writeFitResult(size_t wi, const std::vector<double> &expected_positions,
                              const std::shared_ptr<FitPeaksAlgorithm::PeakFitResult> &fit_result) {
  // convert to
  size_t out_wi = wi - m_startWorkspaceIndex;
  if (out_wi >= m_outputPeakPositionWorkspace->getNumberHistograms()) {
    g_log.error() << "workspace index " << wi << " is out of output peak position workspace "
                  << "range of spectra, which contains " << m_outputPeakPositionWorkspace->getNumberHistograms()
                  << " spectra"
                  << "\n";
    throw std::runtime_error("Out of boundary to set output peak position workspace");
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
    if (fit_result->getNumberParameters() != m_fittedParamTable->columnCount() - 3) {
      g_log.error() << "Peak of type (" << m_peakFunction->name() << ") has " << fit_result->getNumberParameters()
                    << " parameters.  Parameter table shall have 3 more "
                       "columns.  But not it has "
                    << m_fittedParamTable->columnCount() << " columns\n";
      throw std::runtime_error("Peak parameter vector for one peak has different sizes to output "
                               "table workspace");
    }
  } else {
    // effective peak profile parameters: need to re-construct the peak function
    if (4 + m_bkgdFunction->nParams() != m_fittedParamTable->columnCount() - 3) {

      std::stringstream err_ss;
      err_ss << "Peak has 4 effective peak parameters and " << m_bkgdFunction->nParams() << " background parameters "
             << ". Parameter table shall have 3 more  columns.  But not it has " << m_fittedParamTable->columnCount()
             << " columns";
      throw std::runtime_error(err_ss.str());
    }
  }

  // go through each peak
  // get a copy of peak function and background function
  IPeakFunction_sptr peak_function = std::dynamic_pointer_cast<IPeakFunction>(m_peakFunction->clone());
  size_t num_peakfunc_params = peak_function->nParams();
  size_t num_bkgd_params = m_bkgdFunction->nParams();

  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
    // get row number
    size_t row_index = out_wi * m_numPeaksToFit + ipeak;

    // treat as different cases for writing out raw or effective parametr
    if (m_rawPeaksTable) {
      // duplicate from FitPeakResult to table workspace
      for (size_t iparam = 0; iparam < num_peakfunc_params + num_bkgd_params; ++iparam) {
        size_t col_index = iparam + 2;
        // fitted parameter's value
        m_fittedParamTable->cell<double>(row_index, col_index) = fit_result->getParameterValue(ipeak, iparam);
        // fitted parameter's fitting error
        if (m_fitErrorTable) {
          m_fitErrorTable->cell<double>(row_index, col_index) = fit_result->getParameterError(ipeak, iparam);
        }

      } // end for (iparam)
    } else {
      // effective peak profile parameter
      // construct the peak function
      for (size_t iparam = 0; iparam < num_peakfunc_params; ++iparam)
        peak_function->setParameter(iparam, fit_result->getParameterValue(ipeak, iparam));

      // set the effective peak parameters
      m_fittedParamTable->cell<double>(row_index, 2) = peak_function->centre();
      m_fittedParamTable->cell<double>(row_index, 3) = peak_function->fwhm();
      m_fittedParamTable->cell<double>(row_index, 4) = peak_function->height();
      m_fittedParamTable->cell<double>(row_index, 5) = peak_function->intensity();

      // background
      for (size_t iparam = 0; iparam < num_bkgd_params; ++iparam)
        m_fittedParamTable->cell<double>(row_index, 6 + iparam) =
            fit_result->getParameterValue(ipeak, num_peakfunc_params + iparam);
    }

    // set chi2
    m_fittedParamTable->cell<double>(row_index, chi2_index) = fit_result->getCost(ipeak);
  }

  return;
}

//----------------------------------------------------------------------------------------------
std::string FitPeaks::getPeakHeightParameterName(const API::IPeakFunction_const_sptr &peak_function) {
  std::string height_name("");

  std::vector<std::string> peak_parameters = peak_function->getParameterNames();
  for (const auto &parName : peak_parameters) {
    if (parName == "Height") {
      height_name = "Height";
      break;
    } else if (parName == "I") {
      height_name = "I";
      break;
    } else if (parName == "Intensity") {
      height_name = "Intensity";
      break;
    }
  }

  if (height_name.empty())
    throw std::runtime_error("Peak height parameter name cannot be found.");

  return height_name;
}

// A client, like PDCalibration, may set a logging offset to make FitPeaks less "chatty".
// This method temporarily removes the logging offset and logs the message at its priority level.
void FitPeaks::logNoOffset(const size_t &priority, const std::string &msg) {
  LoggingOffsetSentry sentry(this);

  switch (priority) {
  case 4: // warning
    g_log.warning() << msg;
    break;
  case 5: // notice
    g_log.notice() << msg;
    break;
  default:
    assert(false); // not implemented yet
  }
}

DECLARE_ALGORITHM(FitPeaks)

} // namespace Mantid::Algorithms
