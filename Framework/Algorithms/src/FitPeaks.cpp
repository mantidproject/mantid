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

}

void FitPeaks::exec()
{

}

void FitPeaks::fitPeaks()
{
  for (size_t wi = 0; wi < m_inputWorkspace; ++wi)
  {
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    this->fitSpectraPeaks(wi);
  }
}

void FitPeaks::fitSpectraPeaks(size_t wi)
{
  for (size_t ipeak = 0; ipeak < m_numPeaksToFit; ++ipeak)
  {
    // definition
    double bkgd_a, bkgd_b;

    // get peak parameter
    if (ipeak == 0)
    {
      peakparams = this->getInitParameters();
    }
    else
    {
      peakparams = lastPeakParameters();
    }

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
      peakparams.height = max_value * 1.E-2;
    }

    // call Fit to fit peak and background


  }
}

double FitOneSinglePeak::fitFunctionSD(IFunction_sptr fitfunc,
                                       MatrixWorkspace_sptr dataws,
                                       size_t wsindex, double xmin,
                                       double xmax) {
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



DECLARE_ALGORITHM(FitPeaks)


} // namespace Algorithms
} // namespace Mantid
