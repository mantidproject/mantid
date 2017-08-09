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
const size_t X0 = 0;
const size_t HEIGHT = 1;

namespace Mantid {
namespace Algorithms {

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
  *FitPeak(InputWorkspace='diamond_high_res_d', OutputWorkspace='peak0_19999',
  * ParameterTableWorkspace='peak0_19999_Param', WorkspaceIndex=19999,
  * PeakFunctionType='BackToBackExponential', PeakParameterNames='I,A,B,X0,S', PeakParameterValues='2.5e+06,5400,1700,1.07,0.000355', FittedPeakParameterValues='145.234,1.07953e+10,772.662,1.07432,0.000641613', BackgroundParameterNames='A0,A1', BackgroundParameterValues='-3500,3000', FittedBackgroundParameterValues='1499.37,-1296.47', FitWindow='1.05,1.14', PeakRange='1.05,1.09', MinGuessedPeakWidth=10, MaxGuessedPeakWidth=30, GuessedPeakWidthStep=1, PeakPositionTolerance=0.02)
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
    double max_value = findMaxValue(wi, m_peakWindowLeft[ipeak], m_peakWindowRight[ipeak], bkgd_a, bkgd_b) * 1.E-2;
    if (max_value < m_minPeakMaxValue)
    {
      ipeak_fail = true;
      continue;
    }
    else
    {
      lastPeakParameters[HEIGHT] = max_value * 1.E-2;
    }

    // call Fit to fit peak and background


  }
}

double FitPeaks::fitSinglePeak(IFunction_sptr fitfunc,
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
  fit->setProperty("Minimizer", "Levenberg-MarquardtMD");
  fit->setProperty("CostFunction", "Chi-Square");
  fit->setProperty("CalcErrors", true);

  // Execute fit and get result of fitting background
  // m_sstream << "FitSingleDomain: " << fit->asString() << ".\n";

  fit->executeAsChildAlg();
  if (!fit->isExecuted()) {
    g_log.error("Fit for background is not executed. ");
    throw std::runtime_error("Fit for background is not executed. ");
  }
  //  ++m_numFitCalls;

  // Retrieve result
  std::string fitStatus = fit->getProperty("OutputStatus");
  double chi2 = EMPTY_DBL();
  if (fitStatus == "success") {
    chi2 = fit->getProperty("OutputChi2overDoF");
    fitfunc = fit->getProperty("Function");
  }

  // Debug information
  //  m_sstream << "[F1201] FitSingleDomain Fitted-Function " <<
  //  fitfunc->asString()
  //            << ": Fit-status = " << fitStatus << ", chi^2 = " << chi2 <<
  //            ".\n";

  return chi2;
}

void FitPeaks::estimateLinearBackground(size_t wi, double left_window_boundary,
                                        double right_window_boundary,
                                        double &bkgd_a1, double &bkgd_a0) {

  bkgd_a0 = 0.;
  bkgd_a1 = 0.;

  g_log.notice() << "[DB] Estimate background between " << left_window_boundary
                 << " to " << right_window_boundary << "\n";

  return;
}

double FitPeaks::findMaxValue(size_t wi, double left_window_boundary,
                              double right_window_boundary, double b1,
                              double b0) {
  return 0;
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

void FitPeaks::setOutputProperties() {
  setProperty("OutputWorkspace", m_peakPosWS);
  setProperty("OutputPeakParametersWorkspace", m_peakParamsWS);
}

DECLARE_ALGORITHM(FitPeaks)


} // namespace Algorithms
} // namespace Mantid
