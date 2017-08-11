//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FitPeaks.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/StartsWithValidator.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

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

const double MAGICNUMBER = 2.0;
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
FitPeaks::FitPeaks()
{

}

void FitPeaks::init()
{
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace for peak fitting.");

  declareProperty("StartWorkspaceIndex", 0,
                  "Starting workspace index for fit");
  declareProperty("StopWorkspaceIndex", 0,
                  "Last workspace index to fit (not included)");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("PeakParameterValues"),
      "List of (back-to-back exponential) peak parameters' value");

  declareProperty(Kernel::make_unique<ArrayProperty<double>>("PeakCenters"),
                  "List of peak centers to fit against.");
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("FitWindowLeftBoundary"),
                  "List of left boundaries of the peak fitting window corresponding to PeakCenters.");
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("FitWindowRightBoundary"),
                  "List of right boundaries of the peak fitting window corresponding to PeakCenters.");
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("PeakRanges"),
                  "List of double for each peak's range.");

  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace containing peak centers for "
                  "fitting offset.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputPeakParametersWorkspace", "", Direction::Output),
                  "Name of workspace containing all fitted peak parameters.  "
                  "X-values are spectra/workspace index.");

  return;
}

void FitPeaks::exec()
{
  processInputs();

  generateOutputWorkspaces();

  fitPeaks();

  setOutputProperties();
}

void FitPeaks::processInputs()
{
    m_inputWS = getProperty("InputWorkspace");

    int start_wi = getProperty("StartWorkspaceIndex");
    int stop_wi = getProperty("StopWorkspaceIndex");
    m_startWorkspaceIndex = static_cast<size_t>(start_wi);
    m_stopWorkspaceIndex = static_cast<size_t>(stop_wi);
    if (m_stopWorkspaceIndex == 0)
        m_stopWorkspaceIndex = m_inputWS->getNumberHistograms();

    m_peakCenters = getProperty("PeakCenters");
    m_peakWindowLeft = getProperty("FitWindowLeftBoundary");
    m_peakWindowRight = getProperty("FitWindowRightBoundary");
    m_numPeaksToFit = m_peakCenters.size();

    m_initParamValues = getProperty("PeakParameterValues");

    std::vector<double> vecPeakRange = getProperty("PeakRanges");

    // set up more
    if (m_peakWindowLeft.size() != m_peakWindowRight.size())
      throw std::runtime_error("xx");
    for (size_t i = 0; i < m_peakWindowLeft.size(); ++i)
    {
      std::vector<double> window_i;
      window_i.push_back(m_peakWindowLeft[i]);
      window_i.push_back(m_peakWindowRight[i]);
      m_peakWindows.push_back(window_i);
    }

    if (m_numPeaksToFit != vecPeakRange.size())
      throw std::runtime_error("xxaere");

    for (size_t i = 0; i < m_numPeaksToFit; ++i)
    {
      std::vector<double> range_i;
      range_i.push_back(m_peakCenters[i] - vecPeakRange[i]);
      range_i.push_back(m_peakCenters[i] + vecPeakRange[i]);
      m_peakRangeVec.push_back(range_i);
    }


    return;
}

void FitPeaks::fitPeaks()
{
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )

    for (size_t wi = m_startWorkspaceIndex; wi < m_stopWorkspaceIndex; ++wi) {

      PARALLEL_START_INTERUPT_REGION

      fitSpectraPeaks(wi);

      PARALLEL_END_INTERUPT_REGION
  }

  PARALLEL_CHECK_INTERUPT_REGION
}

/**
 * @brief FitPeaks::fitSpectraPeaks
 * @param wi
 */
void FitPeaks::fitSpectraPeaks(size_t wi)
{

    g_log.notice() << "[DB] Fit peaks on workspace index :" << wi << "\n";

  std::vector<double> lastPeakParameters = m_initParamValues;

  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak)
  {
    // definition
    double bkgd_a, bkgd_b;
    bool ipeak_fail(false);

    // estimate peak
    estimateLinearBackground(wi, m_peakWindowLeft[ipeak], m_peakWindowRight[ipeak], bkgd_a, bkgd_b);
    std::vector<double> bkgd_params;
    bkgd_params.push_back(bkgd_b);
    bkgd_params.push_back(bkgd_a);
    double max_value, peak_center;
    findMaxValue(wi, m_peakWindowLeft[ipeak], m_peakWindowRight[ipeak], bkgd_a, bkgd_b, peak_center, max_value);
    if (max_value < m_minPeakMaxValue)
    {
      ipeak_fail = true;
      continue;
    }
    else
    {
      lastPeakParameters[X0] = peak_center;
      lastPeakParameters[HEIGHT] = max_value;
    }

    // call Fit to fit peak and background
    fitSinglePeak(ipeak, wi, lastPeakParameters, bkgd_params, m_peakWindows[ipeak], m_peakRangeVec[ipeak]);


  } // END-FOR
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
   * FittedBackgroundParameterValues='3694.92,-3237.13', FitWindow='1.05,1.14', PeakRange='1.06,1.09',
   * MinGuessedPeakWidth=10, MaxGuessedPeakWidth=20, GuessedPeakWidthStep=1, PeakPositionTolerance=0.02)
 *
 */
double FitPeaks::fitSinglePeak(size_t peakindex, size_t wsindex,
                               std::vector<double> &init_peak_values,
                               std::vector<double> &init_bkgd_values,
                               std::vector<double> &fit_window,
                               std::vector<double> &peak_range) {
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

  g_log.notice() << "[DB] wsindex " << wsindex << ", peak index " << peakindex << "\n";
  g_log.notice() << "[DB] Init peak parameters values: ";
  for (auto iter = init_peak_values.begin(); iter != init_peak_values.end(); ++iter)
      g_log.notice() << *iter << ", ";
  g_log.notice() << "\n[DB] Init background values: ";
  for (auto iter = init_bkgd_values.begin(); iter != init_bkgd_values.end(); ++iter)
    g_log.notice() << *iter << ", ";
  g_log.notice() << "\n[DB] Fit window: " << fit_window[0] << ", " << fit_window[1] << "\n";
  g_log.notice() << "[DB] Peak range: " << peak_range[0] << ", " << peak_range[1];
  g_log.notice() << "... ... ||\n";

  fit_peak->setPropertyValue("InputWorkspace", m_inputWS->getName());
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
  if (!fit_peak->isExecuted()) {
    std::stringstream errss;
    errss << "Unable to fit peak of workspace index " << wsindex << "'s " << peakindex << "-th peak";
    g_log.error(errss.str());
    return false;
  }

  // get the information back
  DataObjects::TableWorkspace_sptr param_table = fit_peak->getProperty("ParameterTableWorkspace");
  if (!param_table)
      g_log.notice() << "Unable to get fitted parameters\n";
  else
  {
      g_log.notice() << "Good to have fitted data\n";

      std::vector<double> param_values(7);
      std::vector<double> param_errors(7);
    processFitResult(param_table, param_values, param_errors);
  }

  return true;
}

void FitPeaks::estimateLinearBackground(size_t wi, double left_window_boundary,
                                        double right_window_boundary,
                                        double &bkgd_a1, double &bkgd_a0) {

  bkgd_a0 = 0.;
  bkgd_a1 = 0.;

  g_log.notice() << "[DB] Estimate background between " << left_window_boundary
                 << " to " << right_window_boundary << "\n";

  auto &vecX = m_inputWS->x(wi);
  auto &vecY = m_inputWS->y(wi);
  size_t istart = findXIndex(vecX, left_window_boundary);
  size_t istop = findXIndex(vecX, right_window_boundary);

  double left_x = 0.;
  double left_y = 0.;
  double right_x = 0.;
  double right_y = 0.;
  for (size_t i = 0; i < 3; ++i)
  {
    left_x += vecX[istart+i] / 3.;
    left_y += vecY[istart+i] / 3.;
    right_x += vecX[istop-i] / 3.;
    right_y += vecY[istop-1] / 3.;
  }

  bkgd_a1 = (left_y - right_y) / (left_x - right_x);
  bkgd_a0 = (left_y * right_x - right_y * left_x) / (right_x - left_x);

  return;
}

void FitPeaks::findMaxValue(size_t wi, double left_window_boundary,
                              double right_window_boundary, double b1,
                              double b0, double &peak_center, double &max_value) {

  auto vecX = m_inputWS->x(wi);
  size_t istart = findXIndex(vecX, left_window_boundary);
  size_t istop = findXIndex(vecX, right_window_boundary);
  auto vecY = m_inputWS->y(wi);

  max_value = 0;
  for (size_t i = istart; i < istop; ++i)
  {
    double x = vecX[i];
    double y = vecY[i] - (b1*x + b0);
    if (y > max_value)
    {
      max_value = y;
      peak_center = x;
    }
  }

  g_log.notice() << "[DB] wsindex " << wi << " between " << left_window_boundary
                 << " and " << right_window_boundary << ": max Y " << max_value
                 << " at x = " << peak_center << "\n";

  return;
}

void FitPeaks::generateOutputWorkspaces(){
  // MatrixWorkspace_sptr outputWS
  //  size_t NVectors, size_t XLength,
  //  size_t YLength)
  size_t num_hist = m_inputWS->getNumberHistograms();
  m_peakPosWS = WorkspaceFactory::Instance().create("Workspace2D", num_hist, m_numPeaksToFit, m_numPeaksToFit);
  m_peakParamsWS = WorkspaceFactory::Instance().create("Workspace2D", m_numPeaksToFit*6, (m_stopWorkspaceIndex-m_startWorkspaceIndex),
                                                       (m_stopWorkspaceIndex-m_startWorkspaceIndex));

}

/**
 * @brief FitPeaks::processFitResult
 * @param param_table
 * @param param_values: sequence is I,A,B,X0,S,A0,A1
 * @param param_erros
 * @return
 */
double FitPeaks::processFitResult(DataObjects::TableWorkspace_sptr param_table,
                                std::vector<double> &param_values, std::vector<double> &param_errors)
{
    if (param_table->rowCount() != 10)
        throw std::runtime_error("Expected 10 rows in the returned table workspace.");

    // clear
    param_values.clear();
    param_errors.clear();

//    g_log.notice() << "Number of rows " << param_table->rowCount() << ", columns "
//                   << param_table->columnCount() << "\n";

    // chi2
    double chi2 = param_table->cell<double>(0, 1);

    size_t iparam = 0;
    for (size_t irow = 2; irow < param_table->rowCount(); ++irow)
    {
        if (irow == 7)
            continue;

        const std::string & parname = param_table->cell<std::string>(irow, 0);
        double param_value = param_table->cell<double>(irow, 1);
        double param_error = param_table->cell<double>(irow, 2);
        g_log.notice() << "Row " << irow << ": " << parname << " = "
                       << param_value << " +/- " << param_error << "\n";

        param_values[iparam] = param_value;
        param_errors[iparam] = param_error;
        ++ iparam;
    }

//    const std::string &cell00 = param_table->cell<std::string>(0, 0);
//    double chi2 = param_table->cell<double>(0, 1);
//    g_log.notice() << "Row 0: " << cell00 << ": " << chi2 << "\n";
//    const std::string &cell01 = param_table->cell<std::string>(1, 0);
//    double param0value = param_table->cell<double>(1, 1);
//    double param0error = param_table->cell<double>(1, 2);
//    g_log.notice() << cell01 << ": " << param0value << " +/- " << param0error << "\n";
//    const std::string &cell02 = param_table->cell<std::string>(2, 0);
//    double param1value = param_table->cell<double>(2, 1);
//    double param1error = param_table->cell<double>(2, 2);
//    g_log.notice() << cell02 << ": " << param1value << " +/- " << param1error << "\n";

    return chi2;
}

void FitPeaks::setOutputProperties() {
  setProperty("OutputWorkspace", m_peakPosWS);
  setProperty("OutputPeakParametersWorkspace", m_peakParamsWS);
}

DECLARE_ALGORITHM(FitPeaks)


} // namespace Algorithms
} // namespace Mantid
