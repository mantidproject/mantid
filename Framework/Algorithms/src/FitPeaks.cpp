//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FitPeaks.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/FindPeakBackground.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
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

const size_t X0 = 3;
const size_t HEIGHT = 0;

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

//----------------------------------------------------------------------------------------------
/** constructor
 * @brief FitPeaks::FitPeaks
 */
FitPeaks::FitPeaks() : m_minPeakMaxValue(20.) {}

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
                  "Last workspace index to fit (not included)");

  // properties about peak positions to fit
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("PeakCenters"),
                  "List of peak centers to fit against.");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "PeakCentersWorkspace", "", Direction::Input, PropertyMode::Optional),
      "MatrixWorkspace containing peak centers");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("PositionTolerance"),
      "List of tolerance on fitted peak positions against given peak positions."
      "If there is only one value given, then ");

  std::string peakcentergrp("Peak Positions");
  setPropertyGroup("PeakCenters", peakcentergrp);
  setPropertyGroup("PeakCentersWorkspace", peakcentergrp);

  // properties about peak profile
  std::vector<std::string> peakNames =
      FunctionFactory::Instance().getFunctionNames<API::IPeakFunction>();
  declareProperty("PeakFunction", "Gaussian",
                  boost::make_shared<StringListValidator>(peakNames));
  vector<string> bkgdtypes{"Flat", "Linear"};
  declareProperty("BackgroundType", "Linear",
                  boost::make_shared<StringListValidator>(bkgdtypes),
                  "Type of Background.");

  std::string funcgroup("Function Types");
  setPropertyGroup("PeakFunction", funcgroup);
  setPropertyGroup("BackgroundType", funcgroup);

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

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("FitWindowBoundaryList"),
      "List of left boundaries of the peak fitting window corresponding to "
      "PeakCenters.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "FitPeakWindowWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "MatrixWorkspace for of peak windows");

  std::string startvaluegrp("Fitting Setup");
  setPropertyGroup("PeakParameterNames", startvaluegrp);
  setPropertyGroup("PeakParameterValues", startvaluegrp);
  setPropertyGroup("PeakParameterValueTable", startvaluegrp);
  setPropertyGroup("FitWindowBoundaryList", startvaluegrp);
  setPropertyGroup("FitPeakWindowWorkspace", startvaluegrp);
  setPropertyGroup("", startvaluegrp);

  // optimization setup
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

  std::string helpgrp("Additional Information");

  setPropertyGroup("EventNumberWorkspace", helpgrp);

  //  declareProperty(Kernel::make_unique<ArrayProperty<double>>("PeakRanges"),
  //                  "List of double for each peak's range.");

  // additional output for reviewing
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
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
  processInputs();

  generateOutputWorkspaces();

  fitPeaks();

  setOutputProperties();
}

//----------------------------------------------------------------------------------------------
/** process inputs
 * @brief FitPeaks::processInputs
 */
void FitPeaks::processInputs() {
  // input workspaces
  m_inputWS = getProperty("InputWorkspace");
  std::string event_ws_name = getPropertyValue("EventNumberWorkspace");
  if (event_ws_name.size() > 0)
    m_eventNumberWS = getProperty("EventNumberWorkspace");
  else
    m_eventNumberWS = 0;

  // fit range
  int start_wi = getProperty("StartWorkspaceIndex");
  int stop_wi = getProperty("StopWorkspaceIndex");
  m_startWorkspaceIndex = static_cast<size_t>(start_wi);
  m_stopWorkspaceIndex = static_cast<size_t>(stop_wi);
  if (m_stopWorkspaceIndex == 0)
    m_stopWorkspaceIndex = m_inputWS->getNumberHistograms();

  m_highBackground = getProperty("HighBackground");

  // Set up peak and background functions
  processInputFunctions();

  // Peak centers and tolerance
  processInputPeakCenters();

  // process peak fitting range
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
  m_bkgdFunction = boost::dynamic_pointer_cast<IBackgroundFunction>(
      API::FunctionFactory::Instance().createFunction(bkgdfunctiontype));

  // input peak parameters
  std::string partablename = getPropertyValue("PeakParameterValueTable");
  m_peakParamNames = getProperty("PeakParameterNames");
  if (partablename.size() == 0 && m_peakParamNames.size() > 0)
  {
    // use uniform starting value of peak parameters
    m_uniformProfileStartingValue = true;
    m_initParamValues = getProperty("PeakParameterValues");
    // convert the parameter name in string to parameter name in integer index
    convert_parameter_name_to_index();
  }
  else if (partablename.size() > 0 && m_peakParamNames.size() == 0)
  {
    // use non-uniform starting value of peak parameters
    m_uniformProfileStartingValue = false;
    m_profileStartingValueTable = getProperty("partablename");
  }

}

//----------------------------------------------------------------------------------------------
/** process and check for inputs about peak fitting range (i.e., window)
 * @brief FitPeaks::processInputFitRanges
 */
void FitPeaks::processInputFitRanges() {

  // get peak fit window
  std::vector<double> peakwindow = getProperty("FitWindowBoundary");
  std::string peakwindowname = getPropertyValue("FitPeakWindowWorkspace");

  if (peakwindow.size() > 0 && peakwindowname.size() == 0) {
    // use vector for peak windows
    m_uniformPeakPositions = true;
    // check peak positions
    if (!m_uniformPeakPositions)
      throw std::invalid_argument(
          "Uniform peak range/window requires uniform peak positions.");

    // check size
    if (peakwindow.size() != m_numPeaksToFit * 2)
      throw std::invalid_argument(
          "Peak window vector must be twice as large as number of peaks.");
    // check range
    m_peakWindowVector.resize(m_numPeaksToFit);
    for (size_t i = 0; i < m_numPeaksToFit; ++i) {
      std::vector<double> peakranges(2);
      peakranges[0] = peakwindow[i * 2];
      peakranges[1] = peakwindow[i * 2 + 1];
      if (!(peakranges[0] < m_peakCenters[i]) ||
          !(m_peakCenters[i] < peakranges[1])) {
        std::stringstream errss;
        errss << "Peak " << i
              << ": use specifies an invalid range and peak center against "
              << peakranges[0] << " < " << m_peakCenters[i] << peakranges[1];
        throw std::invalid_argument(errss.str());
      }
    }
  } else if (peakwindow.size() == 0 && peakwindowname.size() > 0) {
    // use matrix workspace for non-uniform peak windows
    m_peakWindowWorkspace = getProperty("FitPeakWindowWorkspace");
    m_uniformPeakWindows = false;

    // check size
    if (m_peakWindowWorkspace->getNumberHistograms() ==
        m_inputWS->getNumberHistograms())
      m_partialWindowSpectra = false;
    else if (m_peakWindowWorkspace->getNumberHistograms() ==
             (m_stopWorkspaceIndex - m_startWorkspaceIndex))
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
      if (m_peakWindowWorkspace->y(wi).size() != m_numPeaksToFit * 2)
      {
        std::stringstream errss;
        errss << "Peak window workspace index " << wi << " has incompatible number of fit windows (x2) " << m_peakWindowWorkspace->y(wi).size()
              << "with the number of peaks " << m_numPeaksToFit << " to fit.";
        throw std::invalid_argument(errss.str());
      }

      // check window range against peak center
      size_t window_index = window_index_start + wi;
      size_t center_index = window_index - center_index_start;

      for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak)
      {
        double left_w_bound = m_peakWindowWorkspace->y(wi)[ipeak*2];
        double right_w_bound = m_peakWindowWorkspace->y(wi)[ipeak*2+1];
        double center = m_peakCenterWorkspace->x(center_index)[ipeak];
        if (!(left_w_bound < center && center < right_w_bound))
        {
          std::stringstream errss;
          errss << "Workspace index " << wi << " has incompatible peak window (" << left_w_bound << ", " << right_w_bound
                << ") with " << ipeak << "-th expected peak's center " << center;
          throw std::runtime_error(errss.str());
        }

      }
    }
  } else {
    // non-supported situation
    throw std::invalid_argument("One and only one of peak window array and peak window workspace can be specified.");
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** processing peaks centers information from input.  the parameters that are
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
  if (m_peakCenters.size() > 0 && peakpswsname.size() == 0) {
    m_uniformPeakPositions = true;
    m_numPeaksToFit = m_peakCenters.size();
  } else if (m_peakCenters.size() == 0 && peakpswsname.size() > 0) {
    m_uniformPeakPositions = false;
    m_peakCenterWorkspace = getProperty("PeakCentersWorkspace");
    m_numPeaksToFit = m_peakCenterWorkspace->x(0).size();
  } else {
    std::stringstream errss;
    errss << "One and only one in 'PeakCenters' (vector) and "
             "'PeakCentersWorkspace' shall be given. "
          << "'PeakCenters' has size " << m_peakCenters.size()
          << ", and name of peak center workspace "
          << "is " << peakpswsname;
    throw std::invalid_argument(errss.str());
  }

  // check matrix worksapce for peak positions
  if (!m_uniformPeakPositions) {
    size_t numhist = m_peakCenterWorkspace->getNumberHistograms();
    if (numhist == m_inputWS->size())
      m_partialSpectra = false;
    else if (numhist == m_stopWorkspaceIndex - m_startWorkspaceIndex)
      m_partialSpectra = true;
    else
      throw std::invalid_argument(
          "Input peak center workspace has wrong number of spectra.");
  }

  // peak tolerance
  m_peakPosTolerances = getProperty("PositionTolerance");
  if (m_peakPosTolerances.size() == 0)
    throw std::invalid_argument("Peak positions' tolerances must be given!");
  else if (m_peakPosTolerances.size() == 1) {
    // single tolerance, expand to all peaks
    double peaktol = m_peakPosTolerances[0];
    m_peakPosTolerances.resize(m_numPeaksToFit, peaktol);
  } else if (m_peakPosTolerances.size() != m_numPeaksToFit)
    throw std::invalid_argument(
        "Number of input peak tolerance is different from input peaks to fit.");

  return;
}

//----------------------------------------------------------------------------------------------
/** main method to fit peaks among all
 * @brief FitPeaks::fitPeaks
 */
void FitPeaks::fitPeaks() {

  // cppcheck-suppress syntaxError
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (size_t wi = m_startWorkspaceIndex; wi < m_stopWorkspaceIndex; ++wi) {

      PARALLEL_START_INTERUPT_REGION

      // initialize outputs
      std::vector<double> peak_positions(m_numPeaksToFit, -1);
      std::vector<std::vector<double>> peak_parameters(m_numPeaksToFit);
      std::vector<std::vector<double>> fitted_peaks;
      std::vector<std::vector<double>> fitted_peaks_windows;
      std::vector<double> peak_chi2_vec(m_numPeaksToFit, DBL_MAX);

      if (m_eventNumberWS && m_eventNumberWS->readX(wi)[0] < 1.0) {
        // no events in this
        fitted_peaks.clear();
      } else if (m_eventWS && m_eventWS->getNumberEvents() < 100) {
        // too few events in this
        fitted_peaks.clear();
      } else {
        // fit
        fitSpectrumPeaks(wi, peak_positions, peak_parameters, peak_chi2_vec,
                         fitted_peaks, fitted_peaks_windows);
      }

      PARALLEL_CRITICAL(FindPeaks_WriteOutput) {
        writeFitResult(wi, peak_positions, peak_parameters, fitted_peaks,
                       fitted_peaks_windows);
      }

      PARALLEL_END_INTERUPT_REGION
    }

    PARALLEL_CHECK_INTERUPT_REGION
}

//----------------------------------------------------------------------------------------------
/** Fit peaks across one single spectrum
 * @brief FitPeaks::fitSpectrumPeaks
 * @param wi
 * @param peak_pos : fitted peak positions
 * @param peak_params : fitted peak parameters
 * @param peak_chi2_vec : fitted chi squiares
 * @param fitted_functions : ???
 * @param fitted_peak_windows : ???
 */
void FitPeaks::fitSpectrumPeaks(
    size_t wi, std::vector<double> &peak_pos,
    std::vector<std::vector<double>> &peak_params,
    std::vector<double> &peak_chi2_vec,
    std::vector<std::vector<double>> &fitted_functions,
    std::vector<std::vector<double>> &fitted_peak_windows) {

  // Set up sub algorithm Fit for peak and background
  IAlgorithm_sptr peak_fitter; // both peak and background (combo)
  IAlgorithm_sptr bkgd_fitter;
  try {
    peak_fitter = createChildAlgorithm("Fit", -1, -1, false);
    bkgd_fitter = createChildAlgorithm("Fit", -1, -1, false);
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

  bkgd_fitter->setProperty("Minimizer", m_minimizer);
  bkgd_fitter->setProperty("CostFunction", "Least squares");

  std::vector<double> peak_centers = getExpectedPeakPositions(wi);
  // std::vector<std::pair<double, double>> peak_windows = getPeakWidnows(wi);

  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
    // find out the peak position to fit
    // center
    double center_i = peak_centers[ipeak];
    // get xmin and xmax from ..
    std::pair<double, double> peak_window_i = getPeakFitWindow(wi, ipeak);
    // Estimate background
    estimateBackground(wi, peak_window_i, bkgdfunction, bkgd_fitter);

    // Estimate peak profile parameter
    estimatePeakParameters(wi, peak_window_i, peakfunction, bkgdfunction);

    // Fit peak
    double tol = 0.001;
    double cost =
        fitIndividualPeak(wi, peak_fitter, compfunc, peakfunction, bkgdfunction,
                          peak_window_i, center_i, tol, m_highBackground);

    // process fitting result
    processSinglePeakFitResult(wi, ipeak, peakfunction, bkgdfunction, cost,
                               peak_pos, peak_params, peak_chi2_vec,
                               fitted_peaks);
  }

  return;
}

void FitPeaks::processSinglePeakFitResult(
    size_t wi, size_t peakindex, API::IFunction_sptr peakbkgdfunction,
    API::IPeakFunction_sptr peakfunction,
    API::IBackgroundFunction_sptr bkgdfunction, double chi2,
    std::vector<double> &fitted_peak_positions,
    std::vector<std::vector<double>> &peak_params_vector,
    std::vector<double> &peak_chi2_vec, bool calculated_fitted,
    HistogramData::Histogram &fitted_data,
    const std::pair<double, double> &peakwindow) {
  // check input
  if (peakindex >= fitted_peak_positions.size() ||
      peakindex >= peak_params_vector.size() ||
      peakindex >= peak_chi2_vec.size()) {
    throw std::runtime_error("peak index size is out of boundary for fitted "
                             "peaks positions, peak parameters or chi2s");
  }

  // peak position
  fitted_peak_positions[peakindex] = peakfunction->centre();

  // new method
  // TODO/NOW/How to get the peak parameters out?
  std::vector<double> peak_params;
  for (size_t iparam = 0; iparam < m_numberPeakParams; ++iparam)
    peak_params.push_back(peakfunction->getParameter(iparam));
  peak_params_vector[ipeak] = peak_params;

  // chi2
  peak_chi2_vec[ipeak] = chi2;

  // return if it is not required to calculated fitted peaks
  if (!calculated_fitted)
    return;

  // calculate fitted peak with background
  size_t start_x_index = getXIndex(wi, peakwindow.first);
  size_t end_x_index = getXIndex(wi, peakwindow.second);
  for (size_t i = start_x_index; i < end_x_index; ++i) {
    peakbkgdfunction->function(fitted_data.mutableX(), fitted_data.mutableY());
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Estimate background
 * @brief estimateBackground
 * @param wi
 * @param peak_window
 * @param function
 * @param fitter
 */
void FitPeaks::estimateBackground(size_t wi,
                                  const std::pair<double, double> &peak_window,
                                  API::IBackgroundFunction_sptr function,
                                  API::IAlgorithm_sptr fitter) {
  // TODO/ISSUE/NOW - Implement!.. Need to document the algorithm to estimate
  // call algorithm FindPeakBackground
  std::vector<size_t> peak_min_max_indexes;
  std::vector<double> &vector_bkgd(3);

  Mantid::Algorithms::FindPeakBackground bkgd_finder;
  bkgd_finder.setBackgroundOrder(2);
  bkgd_finder.setSigma(sigma);
  bkgd_finder.setFitWindow(peak_window);
  bkgd_finder.findBackground(m_inputWS->histogram(wi), l0, n,
                             peak_min_max_indexes, bkgd_vectors);

  bool result = bkgd_finder.getResults();

  // use the simple way to find linear background
  if (!result) {
    this->estimateLinearBackground()
  }
  /*
    const HistogramData::Histogram &histogram, const size_t &l0,
    const size_t &n, std::vector<size_t> &peak_min_max_indexes,
    std::vector<double> &bkgd3) {
        */

  // blabla()

  // step 2 ... (refer to documentation)

  // step 3 ... blabla

  return;
}

enum { NOSIGNAL, LOWPEAK, OUTOFBOUND, GOOD };

//----------------------------------------------------------------------------------------------
/** Estimate peak profile's parameters values via observation
 * including
 * (1) peak center (2) peak intensity  (3) peak width depending on peak type
 * @brief FitPeaks::esitmatePeakParameters
 * @param wi
 * @param peak_window
 * @param peakfunction
 * @param bgkdfunction
 */
int FitPeaks::estimatePeakParameters(
    size_t wi, const std::pair<double, double> &peak_window,
    API::IPeakFunction_sptr peakfunction,
    API::IBackgroundFunction_sptr bkgdfunction) {
  // TODO/ISSUE/NOW - In development!

  // find the maximum value with and without background
  //  double max_value(0);
  //  double peak_center(DBL_MAX);
  //  size_t peak_center_index(INT_MAX);
  //  double real_max =
  //      findMaxValue(wi, peak_window, bkgdfunction, peak_center_index,
  //      peak_center, max_value);

  // copied from FindMaxValue
  double left_window_boundary = peak_window.first;
  double right_window_boundary = peak_window.second;

  auto vecY = m_inputWS->y(wi);

  double real_y_max = 0;
  double max_value = 0;

  // get the range of start and stop to construct a function domain
  auto vec_x = m_inputWS->x(wi);
  std::vector<double>::const_iterator istart =
      std::lower_bound(vec_x.begin(), vec_x.end(), left_window_boundary);
  std::vector<double>::const_iterator istop =
      std::lower_bound(vec_x.begin(), vec_x.end(), right_window_boundary);

  // FunctionDomain1DVector domain(m_inputWS->x(wi).begin(),
  // m_inputWS->x(wi).end());
  FunctionDomain1DVector domain(istart, istop);
  FunctionValues values(domain);
  bkgdfunction->function(domain, values);

  size_t start_index = static_cast<size_t>(istart - vec_x.begin());
  for (size_t i = 0; i < values.size(); ++i) {
    double y = vecY[i + start_index] - values.getCalculated(i);
    if (y > max_value) {
      max_value = y;
      peak_center = vec_x[i + start_index];
      center_index = i + start_index;
    }
    if (vecY[i] > real_y_max)
      real_y_max = y;
  }

  // check peak position
  size_t ileft = getXIndex(wi, peak_window.first);
  size_t iright = getXIndex(wi, peak_window.second);

  // check peak height
  const size_t MAGIC3(3);

  int result(0);
  if (real_max < 1.0) {
    // none-event, but no signal within region
    result = NOSIGNAL;
  } else if (max_value < m_minPeakMaxValue) {
    // peak too low
    result = LOWPEAK;
  } else if ((peak_center_index - ileft) < MAGIC3 ||
             (iright - peak_center_index) < MAGIC3) {
    // peak not at center
    result = OUTOFBOUND;
  } else {
    result = GOOD;
    //    lastPeakParameters[X0] = peak_center;
    //    lastPeakParameters[HEIGHT] = max_value;
  }

  // estimate FWHM (left and right) by observation
  if (result == GOOD) {
    // TODO - Implement!
    // use values from background to locate FWHM
  }

  return result;
}

////----------------------------------------------------------------------------------------------
///** find the maximum value in a range
// * @brief FitPeaks::findMaxValue
// * @param wi
// * @param left_window_boundary
// * @param right_window_boundary
// * @param b1
// * @param b0
// * @param center_index
// * @param peak_center
// * @param max_value
// * @return
// */
// double FitPeaks::findMaxValue(size_t wi,
//                              const std::pair<double, double> &window,
//                              API::IBackgroundFunction_sptr bkgdfunction,
//                              size_t &center_index,
//                              double &peak_center, double &max_value) {

//  double left_window_boundary = window.first;
//  double right_window_boundary = window.second;

//  auto vecY = m_inputWS->y(wi);

//  double abs_max = 0;
//  max_value = 0;

//  // get the range of start and stop to construct a function domain
//  auto vec_x = m_inputWS->x(wi);
//  std::vector<double>::const_iterator istart =
//      std::lower_bound(vec_x.begin(), vec_x.end(), left_window_boundary);
//  std::vector<double>::const_iterator istop =
//      std::lower_bound(vec_x.begin(), vec_x.end(), right_window_boundary);

//  // FunctionDomain1DVector domain(m_inputWS->x(wi).begin(),
//  // m_inputWS->x(wi).end());
//  FunctionDomain1DVector domain(istart, istop);
//  FunctionValues values(domain);
//  bkgdfunction->function(domain, values);

//  size_t start_index = static_cast<size_t>(istart - vec_x.begin());
//  for (size_t i = 0; i < values.size(); ++i) {
//    double y = vecY[i + start_index] - values.getCalculated(i);
//    if (y > max_value) {
//      max_value = y;
//      peak_center = vec_x[i + start_index];
//      center_index = i + start_index;
//    }
//    if (vecY[i] > abs_max)
//      abs_max = y;
//  }

//  return abs_max;
//}

//----------------------------------------------------------------------------------------------
/** Fit a specific peak with estimated peak and background parameters
 * @brief FitPeaks::fitIndividualPeak
 * @param wi
 * @param fitter
 * @param peakbkgdfunc
 * @param peakfunction
 * @param bkgdfunc
 * @param fitwindow
 * @param exppeakcenter
 * @param postol
 * @param high : high background
 * @return cost of fitting peak
 */
double FitPeaks::fitIndividualPeak(size_t wi, API::IAlgorithm_sptr fitter,
                                   API::IFunction_sptr peakbkgdfunc,
                                   API::IPeakFunction_sptr peakfunction,
                                   API::IBackgroundFunction_sptr bkgdfunc,
                                   const std::pair<double, double> &fitwindow,
                                   const double &exppeakcenter,
                                   const double &postol, const bool high) {

  if (high) {
    // high background : create a new workspace with high background
  }

  //
  bool high(false);

  if (high) {

    // if it does not work, then fit!  refer to FitPeak()
    // fit to background
    fitFunctionMD(fitter, m_inputWS, wi, function);
  }

  // fit peak and background
  double cost = fitFunctionSD(fitter, peakbkgdfunc, m_inputWS, wi,
                              fitwindow.first, fitwindow.second);

  // check cost (chi2 or etc), peak center and peak height
  double peak_pos(0);
  bool good_fit = false;

  if ((cost < 0) || (cost > DBL_MAX - 1.))
    peak_pos = -4; // unphysical cost function value
  else if (peakfunction->height() < m_minHeight)
    peak_pos = -3; // peak height is under minimum request
  else if (fabs(peakfunction->centre() - exppeakcenter) > postol)
    peak_pos = -2; // peak center is not within tolerance
  else {
    // all criteria are passed
    good_fit = true;
  }

  // set cost function to DBL_MAX if fitting is bad
  if (good_fit) {
    // convert fitting result to analysis data structure
    std::cout << "pass";
  } else {
    // set the cost function value to DBL_MAX
    cost = DBL_MAX;
  }

  return cost;
}

//----------------------------------------------------------------------------------------------
/** Fit function in single domain (mostly applied for fitting peak + background)
 * @exception :: Fit.isExecuted is false (cannot be executed)
 * @brief FitPeaks::fitFunctionSD
 * @param fit
 * @param fitfunc
 * @param dataws
 * @param wsindex
 * @param xmin
 * @param xmax
 * @return chi^2 or Rwp depending on input.  If fit is not SUCCESSFUL,
 * return DBL_MAX
 */
double FitPeaks::fitFunctionSD(IAlgorithm_sptr fit, IFunction_sptr fitfunc,
                               MatrixWorkspace_sptr dataws, size_t wsindex,
                               double xmin, double xmax) {

  // Set the properties
  fit->setProperty("Function", fitfunc);
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("MaxIterations", 50); // magic number
  fit->setProperty("StartX", xmin);
  fit->setProperty("EndX", xmax);
  //  fit->setProperty("Minimizer", m_minimizer);
  //  fit->setProperty("CostFunction", m_costFunction);
  //  fit->setProperty("CalcErrors", true);

  // Execute fit and get result of fitting background
  // m_sstream << "FitSingleDomain: " << fit->asString() << ".\n";

  fit->executeAsChildAlg();
  if (!fit->isExecuted()) {
    g_log.error("Fit for background is not executed. ");
    throw std::runtime_error("Fit for background is not executed. ");
  }
  // ++m_numFitCalls;

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

  return chi2;
}

//----------------------------------------------------------------------------------------------
/** Fit function in multi-domain (mostly applied to fitting background without
 * peak)
  * @param mdfunction :: function to fit
  * @param dataws :: matrix workspace to fit with
  * @param wsindex :: workspace index of the spectrum in matrix workspace
  * @param vec_xmin :: minimin values of domains
  * @param vec_xmax :: maximim values of domains
  */
double
FitPeaks::fitFunctionMD(boost::shared_ptr<API::MultiDomainFunction> mdfunction,
                        API::MatrixWorkspace_sptr dataws, size_t wsindex,
                        std::vector<double> &vec_xmin,
                        std::vector<double> &vec_xmax) {
  // Validate
  if (vec_xmin.size() != vec_xmax.size())
    throw runtime_error("Sizes of xmin and xmax (vectors) are not equal. ");

  // Set up sub algorithm fit
  IAlgorithm_sptr fit;
  try {
    fit = createChildAlgorithm("Fit", -1, -1, true);
  } catch (Exception::NotFoundError &) {
    std::stringstream errss;
    errss << "The FitPeak algorithm requires the CurveFitting library";
    g_log.error(errss.str());
    throw std::runtime_error(errss.str());
  }

  // This use multi-domain; but does not know how to set up
  //   IFunction_sptr fitfunc,
  //  boost::shared_ptr<MultiDomainFunction> funcmd =
  //      boost::make_shared<MultiDomainFunction>();

  // Set function first
  funcmd->addFunction(fitfunc);

  // set domain for function with index 0 covering both sides
  funcmd->clearDomainIndices();
  std::vector<size_t> ii(2);
  ii[0] = 0;
  ii[1] = 1;
  funcmd->setDomainIndices(0, ii);

  // Set the properties
  fit->setProperty("Function",
                   boost::dynamic_pointer_cast<IFunction>(mdfunction));
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("StartX", vec_xmin[0]);
  fit->setProperty("EndX", vec_xmax[0]);
  fit->setProperty("InputWorkspace_1", dataws);
  fit->setProperty("WorkspaceIndex_1", static_cast<int>(wsindex));
  fit->setProperty("StartX_1", vec_xmin[1]);
  fit->setProperty("EndX_1", vec_xmax[1]);
  fit->setProperty("MaxIterations", 50);
  //  fit->setProperty("Minimizer", m_minimizer);
  //  fit->setProperty("CostFunction", "Least squares");

  m_sstream << "FitMultiDomain: Funcion " << funcmd->name() << ": "
            << "Range: (" << vec_xmin[0] << ", " << vec_xmax[0] << ") and ("
            << vec_xmin[1] << ", " << vec_xmax[1] << "); " << funcmd->asString()
            << "\n";

  // Execute
  fit->execute();
  if (!fit->isExecuted()) {
    throw runtime_error("Fit is not executed on multi-domain function/data. ");
  }
  ++m_numFitCalls;

  // Retrieve result
  std::string fitStatus = fit->getProperty("OutputStatus");
  m_sstream << "[DB] Multi-domain fit status: " << fitStatus << ".\n";

  double chi2 = EMPTY_DBL();
  if (fitStatus == "success") {
    chi2 = fit->getProperty("OutputChi2overDoF");
    m_sstream << "FitMultidomain: Successfully-Fitted Function "
              << fitfunc->asString() << ", Chi^2 = " << chi2 << "\n";
  }

  return chi2;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::generateOutputWorkspaces
 */
void FitPeaks::generateOutputWorkspaces() {
  // create output workspace for peak positions
  size_t num_hist = m_inputWS->getNumberHistograms();
  m_peakPosWS = WorkspaceFactory::Instance().create(
      "Workspace2D", num_hist, m_numPeaksToFit, m_numPeaksToFit);
  for (size_t wi = 0; wi < num_hist; ++wi)
    for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
      m_peakPosWS->dataX(wi)[m_numPeaksToFit - ipeak - 1] =
          m_peakCenters[ipeak];
    }

  // create output workspace of all fitted peak parameters
  // it has number of peaks * 6 spectra
  // following the order of input peak positions, they are
  // I, A, B, X0, S
  size_t num_spectra_to_fit = m_stopWorkspaceIndex - m_startWorkspaceIndex;
  m_peakParamsWS = WorkspaceFactory::Instance().create(
      "Workspace2D", m_numPeaksToFit * 5, num_spectra_to_fit,
      num_spectra_to_fit);
  for (size_t wi = 0; wi < m_peakParamsWS->getNumberHistograms(); ++wi) {
    for (size_t xi = 0; xi < num_spectra_to_fit; ++xi)
      m_peakParamsWS->dataY(wi)[xi] =
          static_cast<double>(xi + m_startWorkspaceIndex);
  }

  // std::string fit_ws_name = getPropertyValue("FittedPeaksWorkspace");
  m_fittedPeakWS = API::WorkspaceFactory::Instance().create(m_inputWS);
  for (size_t iws = 0; iws < m_fittedPeakWS->getNumberHistograms(); ++iws) {
    auto out_vecx = m_fittedPeakWS->histogram(iws).x();
    auto in_vecx = m_inputWS->histogram(iws).x();
    for (size_t j = 0; j < out_vecx.size(); ++j)
      m_fittedPeakWS->dataX(iws)[j] = in_vecx[j];
    // out_vecx[j] = in_vecx[j];
  }
}

//----------------------------------------------------------------------------------------------
/** Get the expected peak's position
 * @brief FitPeaks::getExpectedPeakPositions
 * @param wi
 * @param ipeak
 * @return
 */
std::vector<double> FitPeaks::getExpectedPeakPositions(size_t wi) {
  std::vector<double> exp_centers;

  if (m_uniformPeakPositions)
    exp_centers = m_peakCenters;
  else
  {
    // no uniform peak center.  locate peak in the workspace
    // check
    if (wi >= m_peakCenterWorkspace->getNumberHistograms()) {
      std::stringstream errss;
      errss << "Workspace index " << wi << " is out of range ("
            << m_peakCenterWorkspace->getNumberHistograms() << ")";
      throw std::runtime_error(errss.str());
    }

    exp_centers = m_peakCenterWorkspace->y(wi).rawData();
  }

  return exp_centers;
}

//----------------------------------------------------------------------------------------------
/** get the peak fit window
 * @brief FitPeaks::getPeakFitWindow
 * @param wi
 * @param ipeak
 * @return
 */
std::pair<double, double> FitPeaks::getPeakFitWindow(size_t wi, size_t ipeak)
{
  // check input
  if (ipeak >= m_numPeaksToFit)
  {
    std::stringstream errss;
    errss << "Peak index " << ipeak << " is out of range (" << m_numPeaksToFit << ")";
    throw std::runtime_error(errss.str());
  }

  double left(0), right(0);
  if (m_uniformPeakWindows)
  {
    left = m_peakWindowVector[ipeak][0];
    right = m_peakWindowVector[ipeak][1];
  }
  else
  {
    // no uniform peak fit window.  locate peak in the workspace
    // check
    if (wi >= m_peakWindowWorkspace->getNumberHistograms())
    {
      std::stringstream errss;
      errss << "Workspace index " << wi << " is out of range (" << m_peakWindowWorkspace->getNumberHistograms() << ")";
      throw std::runtime_error(errss.str());
    }

    left = m_peakWindowWorkspace->y(wi)[ipeak*2];
    right = m_peakWindowWorkspace->y(wi)[ipeak*2+1];
  }

  return std::make_pair(left, right);
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::processFitResult
 * @param param_table
 * @param param_values: sequence is I,A,B,X0,S,A0,A1
 * @param param_erros
 * @return
 */
double FitPeaks::processFitResult(DataObjects::TableWorkspace_sptr param_table,
                                  std::vector<double> &param_values,
                                  std::vector<double> &param_errors) {
  if (param_table->rowCount() != 10)
    throw std::runtime_error(
        "Expected 10 rows in the returned table workspace.");

  // clear
  param_values.clear();
  param_values.resize(7, 0.0);
  param_errors.clear();
  param_values.resize(7, 0.0);

  //    g_log.notice() << "Number of rows " << param_table->rowCount() << ",
  //    columns "
  //                   << param_table->columnCount() << "\n";

  // chi2
  double chi2 = param_table->cell<double>(0, 1);

  size_t iparam = 0;
  for (size_t irow = 2; irow < param_table->rowCount(); ++irow) {
    if (irow == 7)
      continue;

    // const std::string &parname = param_table->cell<std::string>(irow, 0);
    double param_value = param_table->cell<double>(irow, 1);
    double param_error = param_table->cell<double>(irow, 2);
    //    g_log.notice() << "Row " << irow << ": " << parname << " = " <<
    //    param_value
    //                   << " +/- " << param_error << "\n";

    param_values[iparam] = param_value;
    param_errors[iparam] = param_error;
    ++iparam;
  }

  //    const std::string &cell00 = param_table->cell<std::string>(0, 0);
  //    double chi2 = param_table->cell<double>(0, 1);
  //    g_log.notice() << "Row 0: " << cell00 << ": " << chi2 << "\n";
  //    const std::string &cell01 = param_table->cell<std::string>(1, 0);
  //    double param0value = param_table->cell<double>(1, 1);
  //    double param0error = param_table->cell<double>(1, 2);
  //    g_log.notice() << cell01 << ": " << param0value << " +/- " <<
  //    param0error << "\n";
  //    const std::string &cell02 = param_table->cell<std::string>(2, 0);
  //    double param1value = param_table->cell<double>(2, 1);
  //    double param1error = param_table->cell<double>(2, 2);
  //    g_log.notice() << cell02 << ": " << param1value << " +/- " <<
  //    param1error << "\n";

  return chi2;
}

void FitPeaks::setOutputProperties() {
  setProperty("OutputWorkspace", m_peakPosWS);
  setProperty("OutputPeakParametersWorkspace", m_peakParamsWS);
  setProperty("FittedPeaksWorkspace", m_fittedPeakWS);
}

//----------------------------------------------------------------------------------------------
/** Write result of peak fit per spectrum to output analysis workspaces
 * @brief FitPeaks::writeFitResult
 * @param wi
 * @param peak_positions
 * @param peak_parameters
 * @param fitted_peaks
 * @param fitted_peaks_windows
 */
void FitPeaks::writeFitResult(
    size_t wi, const std::vector<double> &peak_positions,
    std::vector<std::vector<double>> &peak_parameters,
    std::vector<std::vector<double>> &fitted_peaks,
    std::vector<std::vector<double>> &fitted_peaks_windows) {

  // TODO - Refine this part!

  // set the fitted peaks' value to output workspace
  for (size_t ipeak = 0; ipeak < fitted_peaks.size(); ++ipeak) {
    // set the peak positions
    if (peak_positions[ipeak] > 0) {
      m_peakPosWS->dataX(wi)[m_numPeaksToFit - ipeak - 1] =
          peak_positions[ipeak];
      m_peakPosWS->dataY(wi)[m_numPeaksToFit - ipeak - 1] =
          peak_parameters[ipeak][HEIGHT];
      m_peakPosWS->dataE(wi)[m_numPeaksToFit - ipeak - 1] =
          peak_chi2_vec[ipeak];
    } else {
      m_peakPosWS->dataY(wi)[m_numPeaksToFit - ipeak - 1] =
          peak_positions[ipeak];
    }
    // peak parameters
    size_t xindex = wi - m_startWorkspaceIndex;
    size_t spec_index = 5 * ipeak;
    for (size_t ipar = 0; ipar < 5; ++ipar) {
      if (peak_parameters[ipeak].size() < 5) {
        std::stringstream errss;
        errss << "wsindex: " << wi
              << "  Data Y size = " << m_peakParamsWS->getNumberHistograms()
              << "; Working on spectrum " << spec_index + ipar << " with size "
              << m_peakParamsWS->histogram(spec_index + ipar).y().size()
              << " and set value to index " << xindex << "\n"
              << "Peak parameters size = " << peak_parameters[ipeak].size();
        throw std::runtime_error(errss.str());
      }

      m_peakParamsWS->dataY(spec_index + ipar)[xindex] =
          peak_parameters[ipeak][ipar];
    }

    // about the peak: if fitting is bad, then the fitted peak window is
    // empty
    //    if (fitted_peaks_windows[ipeak].size() == 2) {
    //      auto vec_x = m_fittedPeakWS->histogram(wi).x();
    //      double window_left = fitted_peaks_windows[ipeak][0];
    //      double window_right = fitted_peaks_windows[ipeak][1];
    //      size_t window_left_index = findXIndex(vec_x, window_left);
    //      size_t window_right_index = findXIndex(vec_x, window_right);
    //      for (size_t ix = window_left_index; ix < window_right_index; ++ix)
    //        m_fittedPeakWS->dataY(wi)[ix] =
    //            fitted_peaks[ipeak][ix - window_left_index];
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
void FitPeaks::reduceBackground(const std::vector<double> &vec_x,
                                const std::vector<double> &vec_y,
                                double &bkgd_a, double &bkgd_b) {
  // calculate the area
  double area = 0;
  for (size_t i = 1; i < vec_y.size(); ++i) {
    double y_0 = vec_y[i - 1];
    double y_f = vec_y[i];
    double dx = vec_x[i] - vec_x[i - 1];
    area += 0.5 * (y_0 + y_f) * dx;
  }

  // find out the local minima
  std::vector<size_t> local_min_indices;
  if (vec_y[0] <= vec_y[1])
    local_min_indices.push_back(0);
  for (size_t i = 1; i < vec_y.size() - 1; ++i) {
    if (vec_y[i] <= vec_y[i - 1] && vec_y[i] <= vec_y[i + 1])
      local_min_indices.push_back(i);
  }
  size_t lastindex = vec_y.size() - 1;
  if (vec_y[lastindex] <= vec_y[lastindex - 1])
    local_min_indices.push_back(lastindex);

  if (local_min_indices.size() < 2)
    throw std::runtime_error(
        "It is not possible to have less than 2 local minima for a peak");

  // loop around to find the pair
  double min_area = DBL_MAX;
  double min_bkgd_a, min_bkgd_b;
  double x_0 = vec_x[0];
  double x_f = vec_x.back();
  double y_0 = vec_y.front();
  double y_f = vec_y.back();

  for (size_t i = 0; i < local_min_indices.size(); ++i) {
    size_t index_i = local_min_indices[i];
    double x_i = vec_x[index_i];
    double y_i = vec_y[index_i];
    for (size_t j = i + 1; j < local_min_indices.size(); ++j) {
      // get x and y
      size_t index_j = local_min_indices[j];
      double x_j = vec_x[index_j];
      double y_j = vec_y[index_j];

      // calculate a and b
      double a_ij = (y_i - y_j) / (x_i - x_j);
      double b_ij = (y_i * x_j - y_j * x_j) / (x_j - x_i);

      // verify no other local minimum being negative after background removed
      bool all_non_negative = true;
      for (size_t ilm = 0; ilm < local_min_indices.size(); ++ilm) {
        if (ilm == index_j || ilm == index_j)
          continue;

        double y_no_bkgd = vec_y[ilm] - (a_ij * vec_x[ilm] + b_ij);
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

  bkgd_a = min_bkgd_a;
  bkgd_b = min_bkgd_b;

  return;
}

size_t FitPeaks::getXIndex(size_t wi, double x) {
  // TODO - Implement NOW
  return 0;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FitPeaks::fitSinglePeak
 * @param fitfunc
 * @param dataws
 * @param wsindex
 * @param xmin
 * @param xmax
 * @return
 */
/*
 *FitPeak(InputWorkspace='diamond_high_res_d', OutputWorkspace='peak0_19999',
   * ParameterTableWorkspace='peak0_19999_Param', WorkspaceIndex=19999,
   * PeakFunctionType='BackToBackExponential', PeakParameterNames='I,A,B,X0,S',
   * PeakParameterValues='2.5e+06,5400,1700,1.07,0.000355',
   * FittedPeakParameterValues='129.407,-1.82258e+06,-230935,1.06065,-0.0154214',
   * BackgroundParameterNames='A0,A1', BackgroundParameterValues='0,0',
   * FittedBackgroundParameterValues='3694.92,-3237.13', FitWindow='1.05,1.14',
 *PeakRange='1.06,1.09',
   * MinGuessedPeakWidth=10, MaxGuessedPeakWidth=20, GuessedPeakWidthStep=1,
 *PeakPositionTolerance=0.02)
 *
 */
double FitPeaks::fitSinglePeakX(size_t wsindex, size_t peakindex,
                                const std::vector<double> &init_peak_values,
                                const std::vector<double> &init_bkgd_values,
                                const std::vector<double> &fit_window,
                                const std::vector<double> &peak_range,
                                std::vector<double> &fitted_params_values,
                                std::vector<double> &fitted_params_errors,
                                std::vector<double> &fitted_window,
                                std::vector<double> &fitted_data) {
  // Set up sub algorithm fit
  IAlgorithm_sptr fit_peak;
  try {
    fit_peak = createChildAlgorithm("FitPeak", -1, -1, false);
    fit_peak->initialize();
  } catch (Exception::NotFoundError &) {
    std::stringstream errss;
    errss << "The FitPeak algorithm requires the CurveFitting library";
    g_log.error(errss.str());
    throw std::runtime_error(errss.str());
  }

  std::stringstream namess;
  namess << m_inputWS->getName() << "_" << wsindex << "_" << peakindex;
  std::string outwsname = namess.str();
  namess << "_param";
  std::string paramwsname = namess.str();

  fit_peak->setPropertyValue("InputWorkspace", m_inputWS->getName());
  fit_peak->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit_peak->setPropertyValue("OutputWorkspace", outwsname);
  fit_peak->setPropertyValue("ParameterTableWorkspace", paramwsname);
  // fit_peak->setProperty("PeakFunctionType", "BackToBackExponential");
  fit_peak->setProperty("PeakFunctionType", mPeakProfile);
  // TODO/FIXME - from here! fit_peak->setProperty("PeakParameterNames", );
  // fit_peak->setProperty("PeakParameterNames", "I,A,B,X0,S");
  fit_peak->setProperty("PeakParameterValues", init_peak_values);
  fit_peak->setProperty("BackgroundParameterNames", "A0, A1");
  fit_peak->setProperty("BackgroundParameterValues", init_bkgd_values);
  fit_peak->setProperty("FitWindow", fit_window);
  fit_peak->setProperty("PeakRange", peak_range);
  fit_peak->setProperty("MinGuessedPeakWidth", 10);
  fit_peak->setProperty("MaxGuessedPeakWidth", 20);
  fit_peak->setProperty("GuessedPeakWidthStep", 1);
  fit_peak->setProperty("PeakPositionTolerance", 0.02);

  fit_peak->executeAsChildAlg();

  double chi2 = -1;
  if (!fit_peak->isExecuted()) {
    std::stringstream errss;
    errss << "Unable to fit peak of workspace index " << wsindex << "'s "
          << peakindex << "-th peak";
    g_log.error(errss.str());
    return chi2;
  }

  // get the information back
  fitted_params_values.resize(7, 0.);
  fitted_params_errors.resize(7, 0.);

  DataObjects::TableWorkspace_sptr param_table =
      fit_peak->getProperty("ParameterTableWorkspace");
  if (!param_table) {
    g_log.information() << "Unable to get fitted parameters\n";
    return chi2;
  } else {
    g_log.information() << "Good to have fitted data\n";

    chi2 = processFitResult(param_table, fitted_params_values,
                            fitted_params_errors);
    //    g_log.notice() << "Number of fitted parameters = " <<
    //    fitted_params_values.size() << "\n";
    //    for (size_t i = 0; i < fitted_params_values.size(); ++i)
    //        g_log.notice() << "Fitted parameter " << i << " = " <<
    //        fitted_params_values[i] << "\n";

    MatrixWorkspace_const_sptr out_ws_i =
        fit_peak->getProperty("OutputWorkspace");
    auto vecx = out_ws_i->histogram(1).x();
    //    g_log.notice() << "[DB] Output workspace from " << vecx.front() << ",
    //    "
    //                   << vecx.back() << ", number of points = " <<
    //                   vecx.size()
    //                   << "\n";

    fitted_window.resize(2);
    fitted_window[0] = vecx.front();
    fitted_window[1] = vecx.back();

    auto vecy = out_ws_i->histogram(1).y();
    fitted_data.resize(vecy.size());
    for (size_t i = 0; i < vecy.size(); ++i)
      fitted_data[i] = vecy[i];
  }

  return chi2;
}

void FitPeaks::estimateLinearBackground(size_t wi, double left_window_boundary,
                                        double right_window_boundary,
                                        double &bkgd_a1, double &bkgd_a0) {

  bkgd_a0 = 0.;
  bkgd_a1 = 0.;

  //  g_log.notice() << "[DB] Estimate background between " <<
  //  left_window_boundary
  //                 << " to " << right_window_boundary << "\n";

  auto &vecX = m_inputWS->x(wi);
  auto &vecY = m_inputWS->y(wi);
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

/// convert peak window from value to index
std::vector<size_t> getRange(size_t wi,
                             const std::vector<double> &peak_window) {
  if (peak_window.size() != 2)
    throw std::runtime_error("Invalid peak window size");

  auto vecX = m_inputWS->histogram(wi).x();
  size_t istart = findXIndex(vecX, peak_window[0]);
  size_t istop = findXIndex(vecX, peak_window[1]);

  std::vector<size_t> range_index_window(2);
  range_index_window[0] = istart;
  range_index_window[1] = istop;

  return range_index_window;
}

DECLARE_ALGORITHM(FitPeaks)

} // namespace Algorithms
} // namespace Mantid
