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

/** constructor
 * @brief FitPeaks::FitPeaks
 */
FitPeaks::FitPeaks() : m_minPeakMaxValue(20.) {}

void FitPeaks::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace for peak fitting.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "EventNumberWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Name of an optional workspace, whose each spectrum corresponds to each "
      "spectrum "
      "in input workspace. "
      "It has 1 value of each spectrum, standing for the number of events of "
      "the corresponding spectrum.");

  declareProperty("StartWorkspaceIndex", 0, "Starting workspace index for fit");
  declareProperty("StopWorkspaceIndex", 0,
                  "Last workspace index to fit (not included)");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("PeakParameterValues"),
      "List of (back-to-back exponential) peak parameters' value");

  declareProperty(Kernel::make_unique<ArrayProperty<double>>("PeakCenters"),
                  "List of peak centers to fit against.");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("FitWindowLeftBoundary"),
      "List of left boundaries of the peak fitting window corresponding to "
      "PeakCenters.");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("FitWindowRightBoundary"),
      "List of right boundaries of the peak fitting window corresponding to "
      "PeakCenters.");
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("PeakRanges"),
                  "List of double for each peak's range.");

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

  return;
}

void FitPeaks::exec() {
  processInputs();

  generateOutputWorkspaces();

  fitPeaks();

  setOutputProperties();
}

void FitPeaks::processInputs() {
  m_inputWS = getProperty("InputWorkspace");
  std::string event_ws_name = getPropertyValue("EventNumberWorkspace");
  if (event_ws_name.size() > 0)
    m_eventNumberWS = getProperty("EventNumberWorkspace");
  else
    m_eventNumberWS = 0;

  int start_wi = getProperty("StartWorkspaceIndex");
  int stop_wi = getProperty("StopWorkspaceIndex");
  m_startWorkspaceIndex = static_cast<size_t>(start_wi);
  m_stopWorkspaceIndex = static_cast<size_t>(stop_wi);
  if (m_stopWorkspaceIndex == 0)
    m_stopWorkspaceIndex = m_inputWS->getNumberHistograms();

  // Set up peak function
  m_peakFunction = boost::dynamic_pointer_cast<IPeakFunction>(
      API::FunctionFactory::Instance().createFunction(m_peakFuncType));
  m_peakParameterNames = m_peakFunction->getParameterNames();

  m_peakCenters = getProperty("PeakCenters");
  m_peakWindowLeft = getProperty("FitWindowLeftBoundary");
  m_peakWindowRight = getProperty("FitWindowRightBoundary");
  m_numPeaksToFit = m_peakCenters.size();

  m_initParamValues = getProperty("PeakParameterValues");

  std::vector<double> vecPeakRange = getProperty("PeakRanges");

  // set up more
  if (m_peakWindowLeft.size() != m_peakWindowRight.size())
    throw std::runtime_error("xx");
  for (size_t i = 0; i < m_peakWindowLeft.size(); ++i) {
    std::vector<double> window_i;
    window_i.push_back(m_peakWindowLeft[i]);
    window_i.push_back(m_peakWindowRight[i]);
    m_peakWindows.push_back(window_i);
  }

  if (m_numPeaksToFit != vecPeakRange.size())
    throw std::runtime_error("xxaere");

  for (size_t i = 0; i < m_numPeaksToFit; ++i) {
    std::vector<double> range_i;
    range_i.push_back(m_peakCenters[i] - vecPeakRange[i]);
    range_i.push_back(m_peakCenters[i] + vecPeakRange[i]);
    m_peakRangeVec.push_back(range_i);
  }

  return;
}

void FitPeaks::fitPeaks() {

  // cppcheck-suppress syntaxError
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (size_t wi = m_startWorkspaceIndex; wi < m_stopWorkspaceIndex; ++wi) {

      PARALLEL_START_INTERUPT_REGION

      std::vector<double> peak_positions;
      std::vector<std::vector<double>> peak_parameters;
      std::vector<std::vector<double>> fitted_peaks;
      std::vector<std::vector<double>> fitted_peaks_windows;
      std::vector<double> peak_chi2_vec;

      fitSpectraPeaks(wi, peak_positions, peak_parameters, peak_chi2_vec,
                      fitted_peaks, fitted_peaks_windows);

      PARALLEL_CRITICAL(FindPeaks_WriteOutput) {
        //        g_log.notice() << "[DB....Outer Most] " <<
        //        peak_positions.size() << ", "
        //                       << peak_parameters.size() << ", " <<
        //                       fitted_peaks.size()
        //                       << "\n"
        //                       << "Size of peak parameters vector = " <<
        //                       peak_parameters.size()
        //                       << "\n";

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
            //            g_log.notice() << "Data Y size = " <<
            //            m_peakParamsWS->getNumberHistograms()
            //                           << "; Working on spectrum " <<
            //                           spec_index + ipar << " with size "
            //                           << m_peakParamsWS->histogram(spec_index
            //                           + ipar).y().size()
            //                           << " and set value to index " << xindex
            //                           << "\n"
            //                           << "Peak parameters size = " <<
            //                           peak_parameters[ipeak].size();
            if (peak_parameters[ipeak].size() < 5) {
              std::stringstream errss;
              errss << "wsindex: " << wi << "  Data Y size = "
                    << m_peakParamsWS->getNumberHistograms()
                    << "; Working on spectrum " << spec_index + ipar
                    << " with size "
                    << m_peakParamsWS->histogram(spec_index + ipar).y().size()
                    << " and set value to index " << xindex << "\n"
                    << "Peak parameters size = "
                    << peak_parameters[ipeak].size();
              throw std::runtime_error(errss.str());
            }

            m_peakParamsWS->dataY(spec_index + ipar)[xindex] =
                peak_parameters[ipeak][ipar];
          }

          // about the peak: if fitting is bad, then the fitted peak window is
          // empty
          if (fitted_peaks_windows[ipeak].size() == 2) {
            auto vec_x = m_fittedPeakWS->histogram(wi).x();
            double window_left = fitted_peaks_windows[ipeak][0];
            double window_right = fitted_peaks_windows[ipeak][1];
            size_t window_left_index = findXIndex(vec_x, window_left);
            size_t window_right_index = findXIndex(vec_x, window_right);
            //            g_log.notice() << "Set Y value from index " <<
            //            window_left_index
            //                           << " to " << window_right_index <<
            //                           "\n";
            for (size_t ix = window_left_index; ix < window_right_index; ++ix)
              m_fittedPeakWS->dataY(wi)[ix] =
                  fitted_peaks[ipeak][ix - window_left_index];
          }
        }
      }

      PARALLEL_END_INTERUPT_REGION
    }

    PARALLEL_CHECK_INTERUPT_REGION
}

/**
 * @brief FitPeaks::fitSpectraPeaks
 * @param wi
 * @param peak_pos
 * @param peak_params
 * @param fitted_functions
 * @param fitted_peak_windows
 */
void FitPeaks::fitSpectraPeaks(
    size_t wi, std::vector<double> &peak_pos,
    std::vector<std::vector<double>> &peak_params,
    std::vector<double> &peak_chi2_vec,
    std::vector<std::vector<double>> &fitted_functions,
    std::vector<std::vector<double>> &fitted_peak_windows) {

  // init outputs
  peak_pos.resize(m_numPeaksToFit);
  peak_chi2_vec.resize(m_numPeaksToFit);
  fitted_peak_windows.clear();

  std::vector<double> lastPeakParameters = m_initParamValues;

  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak) {
    // definition
    double bkgd_a, bkgd_b;
    bool peak_i_no_fit(false);

    // estimate peak
    estimateLinearBackground(wi, m_peakWindowLeft[ipeak],
                             m_peakWindowRight[ipeak], bkgd_a, bkgd_b);
    std::vector<double> bkgd_params;
    bkgd_params.push_back(bkgd_b);
    bkgd_params.push_back(bkgd_a);
    double max_value, peak_center;
    double real_max =
        findMaxValue(wi, m_peakWindowLeft[ipeak], m_peakWindowRight[ipeak],
                     bkgd_a, bkgd_b, peak_center, max_value);

    if (m_eventNumberWS && m_eventNumberWS->readX(wi)[0] < 1.0) {
      // no events
      peak_pos[ipeak] = -1;
      peak_i_no_fit = true;
    } else if (real_max < 1.0) {
      // none-event, but no signal within region
      peak_pos[ipeak] = -1;
      peak_i_no_fit = true;
    } else if (max_value < m_minPeakMaxValue) {
      peak_i_no_fit = true;
      peak_pos[ipeak] = -2;
    } else {
      lastPeakParameters[X0] = peak_center;
      lastPeakParameters[HEIGHT] = max_value;
    }

    //    g_log.warning() << "[DB-TRACE] ipeak_faile = " << ipeak_fail << ", "
    //                    << "real max = " << real_max << "\n";

    // call Fit to fit peak and background
    std::vector<double> fitted_params_values(7, 0.0);
    std::vector<double> fitted_params_errors(7, 0.0);
    std::vector<double> fitted_x_window;
    std::vector<double> fitted_y_vector;

    if (!peak_i_no_fit) {
      double chi2 = fitSinglePeak(wi, ipeak, lastPeakParameters, bkgd_params,
                                  m_peakWindows[ipeak], m_peakRangeVec[ipeak],
                                  fitted_params_values, fitted_params_errors,
                                  fitted_x_window, fitted_y_vector);
      if (chi2 >= 0) {
        double peak_pos_i = fitted_params_values[X0];
        double TOLERANCE = 0.01;
        peak_chi2_vec[ipeak] = chi2;
        if (fabs(peak_pos_i - m_peakCenters[ipeak]) < TOLERANCE) {
          peak_pos[ipeak] = peak_pos_i;
        } else
        // fitted peak position is too off
        {
          peak_pos[ipeak] = -4;
          g_log.warning() << "wsindex " << wi << " Fitted peak center "
                          << peak_pos_i
                          << " is far off with theoretical center "
                          << m_peakCenters[ipeak] << "\n";
        }
      } else {
        // failed to execute FitPeak
        peak_pos[ipeak] = -3;
      }

      //    double window_left = fitted_x_window[0];
      //    double window_right = fitted_x_window[1];
      //    auto vec_x = m_inputWS->histogram(wi).x();
      //    size_t window_left_index = findXIndex(vec_x, window_left);
      //     size_t window_right_index = findXIndex(vec_x, window_right);
      //      g_log.notice() << "ws-index " << wi << " window " << window_left
      //      << ", "
      //                     << window_right << " (" << window_left_index
      //                     << ", " << window_right_index << ")\n";
      //      g_log.notice() << "output fitted data: " <<
      //      m_fittedPeakWS->getNumberHistograms() << "\n";
      //      g_log.notice() << "output data range: " <<
      //      m_fittedPeakWS->histogram(wi).x()[window_left_index]
      //                        << " to " <<
      //                        m_fittedPeakWS->histogram(wi).x()[window_right_index]
      //                        << "\n";
    } // END OF FITTING PEAK

    // set up the output value for fitted result
    if (peak_pos[ipeak] > 0) {
      // a valid fitting
      // peak parameters that are fitted
      peak_params.push_back(fitted_params_values);
      // peak values from fitted model
      fitted_peak_windows.push_back(fitted_x_window);
      fitted_functions.push_back(fitted_y_vector);
    } else {
      // no fit or bad fit
      // push back whatever all-zero
      peak_params.push_back(fitted_params_values);
      // non-data
      std::vector<double> empty(0);
      fitted_peak_windows.push_back(empty);
      fitted_functions.push_back(empty);
    }
  } // END-FOR

  return;
}

std::vector<size_t> FitPeaks::getRange(size_t wi,
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
double FitPeaks::fitSinglePeak(size_t wsindex, size_t peakindex,
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

  //  g_log.notice() << "[DB] wsindex " << wsindex << ", peak index " <<
  //  peakindex
  //                 << "\n";
  //  g_log.notice() << "[DB] Init peak parameters values: ";
  //  for (auto iter = init_peak_values.begin(); iter != init_peak_values.end();
  //       ++iter)
  //    g_log.notice() << *iter << ", ";
  //  g_log.notice() << "\n[DB] Init background values: ";
  //  for (auto iter = init_bkgd_values.begin(); iter != init_bkgd_values.end();
  //       ++iter)
  //    g_log.notice() << *iter << ", ";
  //  g_log.notice() << "\n[DB] Fit window: " << fit_window[0] << ", "
  //                 << fit_window[1] << "\n";
  //  g_log.notice() << "[DB] Peak range: " << peak_range[0] << ", "
  //                 << peak_range[1];
  //  g_log.notice() << "... ... ||\n";

  fit_peak->setPropertyValue("InputWorkspace", m_inputWS->getName());
  fit_peak->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit_peak->setPropertyValue("OutputWorkspace", outwsname);
  fit_peak->setPropertyValue("ParameterTableWorkspace", paramwsname);
  fit_peak->setProperty("PeakFunctionType", "BackToBackExponential");
  fit_peak->setProperty("PeakParameterNames", "I,A,B,X0,S");
  fit_peak->setProperty("PeakParameterValues", init_peak_values);
  fit_peak->setProperty("BackgroundParameterNames", "A0, A1");
  fit_peak->setProperty("BackgroundParameterValues", init_bkgd_values);
  fit_peak->setProperty("FitWindow", fit_window);
  fit_peak->setProperty("PeakRange", peak_range);
  fit_peak->setProperty("MinGuessedPeakWidth", 10);
  fit_peak->setProperty("MaxGuessedPeakWidth", 20);
  fit_peak->setProperty("GuessedPeakWidthStep", 1);
  fit_peak->setProperty("PeakPositionTolerance", 0.02);

  //  // Execute fit and get result of fitting background
  //  // m_sstream << "FitSingleDomain: " << fit->asString() << ".\n";

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

double FitPeaks::findMaxValue(size_t wi, double left_window_boundary,
                              double right_window_boundary, double b1,
                              double b0, double &peak_center,
                              double &max_value) {

  auto vecX = m_inputWS->x(wi);
  size_t istart = findXIndex(vecX, left_window_boundary);
  size_t istop = findXIndex(vecX, right_window_boundary);
  auto vecY = m_inputWS->y(wi);

  double abs_max = 0;

  max_value = 0;
  for (size_t i = istart; i < istop; ++i) {
    double x = vecX[i];
    double y = vecY[i] - (b1 * x + b0);
    if (y > max_value) {
      max_value = y;
      peak_center = x;
    }
    if (vecY[i] > abs_max)
      abs_max = y;
  }

  //  g_log.notice() << "[DB] wsindex " << wi << " between " <<
  //  left_window_boundary
  //                 << " and " << right_window_boundary << ": max Y " <<
  //                 max_value
  //                 << " at x = " << peak_center << "\n";

  return abs_max;
}

//----------------------------------------------------------------------------------------------
double FitPeaks::FitIndividualPeak() {
  // FitSinglePeak version 2.0

  // Estimate peak background

  // Fit peak (core)
  double cost =
      call_fit_peak(m_inputWS, ws_index, peak_function, m_backgroundFunction,
                    fit_window, peak_range, init_peak_parameters);

  // check chi^2 and height
  bool good_fit = false;
  if (0 < cost < DBL_MAX && peak_function->height() > m_minHeight)
    good_fit = true;

  // check with peak position tolerane
  if (good_fit && m_applyPeakPositionTolerance &&
      (peak_function->centre() - peak_position) < m_peakPositionTolerance())
    good_fit = true;
}

//----------------------------------------------------------------------------------------------
double FitPeaks::call_fit_peak() {

  std::stringstream dbss;
  dbss << "[Call FitPeak] Fit 1 peak at X = " << peakfunction->centre()
       << " of spectrum " << wsindex;
  g_log.information(dbss.str());

  double userFWHM = m_peakFunction->fwhm();
  bool fitwithsteppedfwhm = (guessedFWHMStep > 0);

  FitOneSinglePeak fitpeak;
  fitpeak.setChild(true);
  fitpeak.setWorskpace(dataws, wsindex);
  fitpeak.setFitWindow(vec_fitwindow[0], vec_fitwindow[1]);
  fitpeak.setFittingMethod(m_minimizer, m_costFunction);
  fitpeak.setFunctions(peakfunction, backgroundfunction);
  fitpeak.setupGuessedFWHM(userFWHM, minGuessFWHM, maxGuessFWHM,
                           guessedFWHMStep, fitwithsteppedfwhm);
  fitpeak.setPeakRange(vec_peakrange[0], vec_peakrange[1]);

  if (estBackResult == 1) {
    g_log.information("simpleFit");
    fitpeak.simpleFit();
  } else if (m_highBackground) {
    g_log.information("highBkgdFit");
    fitpeak.highBkgdFit();
  } else {
    g_log.information("simpleFit");
    fitpeak.simpleFit();
  }

  double costfuncvalue = fitpeak.getFitCostFunctionValue();
  std::string dbinfo = fitpeak.getDebugMessage();
  g_log.information(dbinfo);

  return costfuncvalue;
}

//----------------------------------------------------------------------------------------------
/** Fit function in single domain
  * @exception :: (1) Fit cannot be called. (2) Fit.isExecuted is false (cannot
 * be executed)
  * @return :: chi^2 or Rwp depending on input.  If fit is not SUCCESSFUL,
 * return DBL_MAX
  */
double FitPeaks::fitFunctionSD(IFunction_sptr fitfunc,
                               MatrixWorkspace_sptr dataws, size_t wsindex,
                               double xmin, double xmax) {
  // Set up sub algorithm fit
  IAlgorithm_sptr fit;
  try {
    fit = createChildAlgorithm("Fit", -1, -1, false);
  } catch (Exception::NotFoundError &) {
    std::stringstream errss;
    errss << "The FitPeak algorithm requires the CurveFitting library";
    g_log.error(errss.str());
    throw std::runtime_error(errss.str());
  }

  // Set the properties
  fit->setProperty("Function", fitfunc);
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("MaxIterations", 50); // magic number
  fit->setProperty("StartX", xmin);
  fit->setProperty("EndX", xmax);
  fit->setProperty("Minimizer", m_minimizer);
  fit->setProperty("CostFunction", m_costFunction);
  fit->setProperty("CalcErrors", true);

  // Execute fit and get result of fitting background
  m_sstream << "FitSingleDomain: " << fit->asString() << ".\n";

  fit->executeAsChildAlg();
  if (!fit->isExecuted()) {
    g_log.error("Fit for background is not executed. ");
    throw std::runtime_error("Fit for background is not executed. ");
  }
  ++m_numFitCalls;

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

DECLARE_ALGORITHM(FitPeaks)

} // namespace Algorithms
} // namespace Mantid
