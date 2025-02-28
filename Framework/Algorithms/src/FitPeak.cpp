// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <algorithm>
#include <iterator>
#include <utility>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/FitPeak.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StartsWithValidator.h"

#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/trim.hpp"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using Mantid::HistogramData::HistogramX;

using namespace std;

const double MAGICNUMBER = 2.0;

namespace Mantid::Algorithms {
//----------------------------------------------------------------------------------------------
/** Constructor for FitOneSinglePeak
 */
FitOneSinglePeak::FitOneSinglePeak()
    : API::Algorithm(), m_fitMethodSet(false), m_peakRangeSet(false), m_peakWidthSet(false), m_peakWindowSet(false),
      m_usePeakPositionTolerance(false), m_peakFunc(), m_bkgdFunc(), m_dataWS(), m_wsIndex(0), m_minFitX(0.),
      m_maxFitX(0.), i_minFitX(0), i_maxFitX(0), m_minPeakX(0.), m_maxPeakX(0.), i_minPeakX(0), i_maxPeakX(0),
      m_bestPeakFunc(), m_bestBkgdFunc(), m_bkupPeakFunc(), m_bkupBkgdFunc(), m_fitErrorPeakFunc(),
      m_fitErrorBkgdFunc(), m_minimizer("Levenberg-MarquardtMD"), m_costFunction(), m_vecFWHM(),
      m_peakPositionTolerance(0.), m_userPeakCentre(0.), m_bestRwp(0.), m_finalGoodnessValue(0.), m_numFitCalls(0),
      m_sstream("") {}

//----------------------------------------------------------------------------------------------
/** Set workspaces
 */
void FitOneSinglePeak::setWorskpace(const API::MatrixWorkspace_sptr &dataws, size_t wsindex) {
  if (dataws) {
    m_dataWS = dataws;
  } else {
    throw runtime_error("Input dataws is null. ");
  }

  if (wsindex < m_dataWS->getNumberHistograms()) {
    m_wsIndex = wsindex;
  } else {
    throw runtime_error("Input workspace index is out of range.");
  }
}

//----------------------------------------------------------------------------------------------
/** Set peaks
 */
void FitOneSinglePeak::setFunctions(const IPeakFunction_sptr &peakfunc, const IBackgroundFunction_sptr &bkgdfunc) {
  if (peakfunc)
    m_peakFunc = peakfunc;

  if (bkgdfunc)
    m_bkgdFunc = bkgdfunc;
}

//----------------------------------------------------------------------------------------------
/** Set fit range
 * */
void FitOneSinglePeak::setFitWindow(double leftwindow, double rightwindow) {
  m_minFitX = leftwindow;
  m_maxFitX = rightwindow;

  const auto &vecX = m_dataWS->x(m_wsIndex);

  i_minFitX = getIndex(vecX, m_minFitX);
  i_maxFitX = getIndex(vecX, m_maxFitX);

  m_peakWindowSet = true;
}

//----------------------------------------------------------------------------------------------
/** Set the range of peak, which served as
 * (1) range of valid peak centre
 * (2) removing peak for fitting background
 * @param xpeakleft :: position (x-value) of the left end of peak
 * @param xpeakright :: position (x-value) of the right end of peak
 */
void FitOneSinglePeak::setPeakRange(double xpeakleft, double xpeakright) {
  m_minPeakX = xpeakleft;
  m_maxPeakX = xpeakright;

  const auto &vecX = m_dataWS->x(m_wsIndex);

  i_minPeakX = getIndex(vecX, m_minPeakX);
  i_maxPeakX = getIndex(vecX, m_maxPeakX);

  m_peakRangeSet = true;
}

//----------------------------------------------------------------------------------------------
/** Set up fitting method other than default
 * @param minimizer :: GSL minimizer (string)
 * @param costfunction :: string of the name of the cost function
 */
void FitOneSinglePeak::setFittingMethod(std::string minimizer, const std::string &costfunction) {
  m_minimizer = std::move(minimizer);
  if (costfunction == "Chi-Square") {
    m_costFunction = "Least squares";
  } else if (costfunction == "Rwp") {
    m_costFunction = "Rwp";
  } else if (costfunction == "Least squares") {
    m_costFunction = costfunction;
  } else {
    stringstream errss;
    errss << "FitOneSinglePeak: cost function " << costfunction << " is not supported. ";
    throw runtime_error(errss.str());
  }

  m_fitMethodSet = true;
}

//----------------------------------------------------------------------------------------------
/** Set FWHM of the peak by guessing.
 * Result is stored to m_vecFWHM
 * @param usrwidth :: peak FWHM given by user (in input peak function)
 * @param minfwhm :: minimim FWHM in unit of pixel
 * @param maxfwhm :: maximum FWHM in unit of pixel
 * @param stepsize :: step of FWHM in unit of pixel
 * @param fitwithsteppedfwhm :: boolean flag whether setting a series of FWHM
 * to guess with
 */
void FitOneSinglePeak::setupGuessedFWHM(double usrwidth, int minfwhm, int maxfwhm, int stepsize,
                                        bool fitwithsteppedfwhm) {
  m_vecFWHM.clear();

  // From user specified guess value
  if (usrwidth <= 0) {
    // Set up default FWHM if user does not give reasonable peak width
    m_sstream << "Client inputs user-defined peak width = " << usrwidth << "; Automatically reset to 4 as default."
              << "\n";

    if (!fitwithsteppedfwhm) {
      fitwithsteppedfwhm = true;
      minfwhm = 4;
      maxfwhm = 4;
      stepsize = 1;
    } else {
      if (minfwhm > 4) {
        minfwhm = 4;
      }
      if (maxfwhm < minfwhm)
        maxfwhm = 4;
    }
  } else {
    m_vecFWHM.emplace_back(usrwidth);
    m_sstream << "Add user defined FWHM = " << usrwidth << "\n";
  }

  m_peakWidthSet = true;

  // From user specified minimum value to maximim value
  if (!fitwithsteppedfwhm) {
    if (m_vecFWHM.empty())
      throw runtime_error("Logic error in setup guessed FWHM.  ");
    m_sstream << "No FWHM is not guessed by stepped FWHM. "
              << "\n";
    return;
  }

  auto &vecX = m_dataWS->x(m_wsIndex);

  auto i_centre = static_cast<int>(getIndex(vecX, m_peakFunc->centre()));
  int i_maxindex = static_cast<int>(vecX.size()) - 1;

  m_sstream << "FWHM to guess. Range = " << minfwhm << ", " << maxfwhm << "; Step = " << stepsize << "\n";
  if (stepsize == 0 || maxfwhm < minfwhm)
    throw runtime_error("FWHM is not given right.");

  for (int iwidth = minfwhm; iwidth <= maxfwhm; iwidth += stepsize) {
    // There are 3 possible situation: peak at left edge, peak in proper range,
    // peak at righ edge
    int ileftside = i_centre - iwidth / 2;
    if (ileftside < 0)
      ileftside = 0;

    int irightside = i_centre + iwidth / 2;
    if (irightside > i_maxindex)
      irightside = i_maxindex;

    double in_fwhm = vecX[irightside] - vecX[ileftside];

    if (in_fwhm < 1.0E-20) {
      m_sstream << "It is impossible to have zero peak width as iCentre = " << i_centre << ", iWidth = " << iwidth
                << "\n"
                << "More information: Spectrum = " << m_wsIndex << "; Range of X is " << vecX.front() << ", "
                << vecX.back() << "; Peak centre = " << vecX[i_centre] << "\n";
    } else {
      m_sstream << "Setup: i_width = " << iwidth << ", i_left = " << ileftside << ", i_right = " << irightside
                << ", FWHM = " << in_fwhm << ", i_centre = " << i_centre << ".\n";
    }

    m_vecFWHM.emplace_back(in_fwhm);
  }
}

//----------------------------------------------------------------------------------------------
/** Set fitted peak parameters' criterial including
 * (a) peak position tolerance to the given one, which is more restricted than
 * peak range
 * @param usepeakpostol :: boolean as the flag to have this restriction
 * @param peakpostol :: double as the tolerance of the peak position
 */
void FitOneSinglePeak::setFitPeakCriteria(bool usepeakpostol, double peakpostol) {
  m_usePeakPositionTolerance = usepeakpostol;
  if (usepeakpostol) {
    m_peakPositionTolerance = std::abs(peakpostol);
    if (peakpostol < 1.0E-13)
      g_log.warning("Peak position tolerance is very tight. ");
  }
}

//----------------------------------------------------------------------------------------------
/** Check whether the class object is ready to fit peak
 */
bool FitOneSinglePeak::hasSetupToFitPeak(std::string &errmsg) {
  errmsg = "";

  if (!m_fitMethodSet)
    errmsg += "Fitting method ";
  if (!m_peakRangeSet)
    errmsg += "Peak range  ";
  if (!m_peakWidthSet)
    errmsg += "Peak width ";
  if (!m_peakFunc)
    errmsg += "Peak function ";
  if (!m_bkgdFunc)
    errmsg += "Background function ";
  if (!m_dataWS)
    errmsg += "Data workspace ";

  if (!errmsg.empty()) {
    errmsg = "These parameters have not been set for fitting peak: " + errmsg;
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------------------------
/** Get debug message
 */
std::string FitOneSinglePeak::getDebugMessage() { return m_sstream.str(); }

//----------------------------------------------------------------------------------------------
/** Fit peak with simple schemem
 */
bool FitOneSinglePeak::simpleFit() {
  m_numFitCalls = 0;
  string errmsg;
  if (!hasSetupToFitPeak(errmsg)) {
    g_log.error(errmsg);
    throw runtime_error("Object has not been set up completely to fit peak.");
  }

  // Initialize refinement state parameters
  m_bestRwp = DBL_MAX;

  // Set up a composite function
  CompositeFunction_sptr compfunc = std::make_shared<CompositeFunction>();
  compfunc->addFunction(m_peakFunc);
  compfunc->addFunction(m_bkgdFunc);

  m_sstream << "One-Step-Fit Function: " << compfunc->asString() << "\n";

  // Store starting setup
  m_bkupPeakFunc = backup(m_peakFunc);
  m_bkupBkgdFunc = backup(m_bkgdFunc);

  // Fit with different starting values of peak width
  size_t numfits = m_vecFWHM.size();

  Progress progress(this, 0.0, 1.0, numfits);

  for (size_t i = 0; i < numfits; ++i) {
    // set FWHM
    m_sstream << "[SingleStepFit] FWHM = " << m_vecFWHM[i] << "\n";
    m_peakFunc->setFwhm(m_vecFWHM[i]);

    // fit and process result
    double goodndess = fitFunctionSD(compfunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX);
    processNStoreFitResult(goodndess, true);

    // restore the function parameters
    if (i != numfits - 1) {
      pop(m_bkupPeakFunc, m_peakFunc);
      pop(m_bkupBkgdFunc, m_bkgdFunc);
    }

    progress.report();
  }

  // Retrieve the best result stored
  pop(m_bestPeakFunc, m_peakFunc);
  pop(m_bestBkgdFunc, m_bkgdFunc);

  m_finalGoodnessValue = m_bestRwp;

  m_sstream << "One-Step-Fit Best (Chi^2 = " << m_bestRwp << ") Fitted Function: " << compfunc->asString() << "\n"
            << "Number of calls of Fit = " << m_numFitCalls << "\n";

  return false;
}

//----------------------------------------------------------------------------------------------
/** Generate a new temporary workspace for removed background peak
 */
API::MatrixWorkspace_sptr FitOneSinglePeak::genFitWindowWS() {

  auto &vecY = m_dataWS->y(m_wsIndex);

  size_t size = i_maxFitX - i_minFitX + 1;
  size_t ysize = size;
  size_t ishift = i_maxFitX + 1;
  if (ishift >= vecY.size())
    ysize = vecY.size() - i_minFitX;

  HistogramBuilder builder;
  builder.setX(size);
  builder.setY(ysize);
  MatrixWorkspace_sptr purePeakWS = create<Workspace2D>(1, builder.build());

  auto &vecX = m_dataWS->x(m_wsIndex);
  auto &vecE = m_dataWS->e(m_wsIndex);
  auto &dataX = purePeakWS->mutableX(0);
  auto &dataY = purePeakWS->mutableY(0);
  auto &dataE = purePeakWS->mutableE(0);

  dataX.assign(vecX.cbegin() + i_minFitX, vecX.cbegin() + i_maxFitX + 1);
  if (ishift < vecY.size()) {
    dataY.assign(vecY.cbegin() + i_minFitX, vecY.cbegin() + i_maxFitX + 1);
    dataE.assign(vecE.cbegin() + i_minFitX, vecE.cbegin() + i_maxFitX + 1);
  } else {
    dataY.assign(vecY.cbegin() + i_minFitX, vecY.cend());
    dataE.assign(vecE.cbegin() + i_minFitX, vecE.cend());
  }

  return purePeakWS;
}

//----------------------------------------------------------------------------------------------
/** Estimate the peak height from a set of data containing pure peaks
 */
double FitOneSinglePeak::estimatePeakHeight(const API::IPeakFunction_const_sptr &peakfunc,
                                            const MatrixWorkspace_sptr &dataws, size_t wsindex, size_t ixmin,
                                            size_t ixmax) {
  // Get current peak height: from current peak centre (previously setup)
  double peakcentre = peakfunc->centre();
  vector<double> svvec(1, peakcentre);
  FunctionDomain1DVector svdomain(svvec);
  FunctionValues svvalues(svdomain);
  peakfunc->function(svdomain, svvalues);
  double curpeakheight = svvalues[0];

  const auto &vecX = dataws->x(wsindex);
  const auto &vecY = dataws->y(wsindex);
  double ymax = vecY[ixmin + 1];
  size_t iymax = ixmin + 1;
  for (size_t i = ixmin + 2; i < ixmax; ++i) {
    double tempy = vecY[i];
    if (tempy > ymax) {
      ymax = tempy;
      iymax = i;
    }
  }

  m_sstream << "Estimate-Peak-Height: Current peak height = " << curpeakheight
            << ". Estimate-Peak-Height: Maximum Y value between " << vecX[ixmin] << " and " << vecX[ixmax] << " is "
            << ymax << " at X = " << vecX[iymax] << ".\n";

  // Compute peak height (not the maximum peak intensity)
  double estheight = ymax / curpeakheight * peakfunc->height();

  return estheight;
}

//----------------------------------------------------------------------------------------------
/** Make a pure peak WS in the fit window region from m_background_function
 * @param purePeakWS :: workspace containing pure peak (w/ background removed)
 */
void FitOneSinglePeak::removeBackground(const MatrixWorkspace_sptr &purePeakWS) {
  // Calculate background
  // FIXME - This can be costly to use FunctionDomain and FunctionValue
  auto &vecX = purePeakWS->x(0);
  FunctionDomain1DVector domain(MantidVec(vecX.begin(), vecX.end()));
  FunctionValues bkgdvalues(domain);
  m_bkgdFunc->function(domain, bkgdvalues);

  // Calculate pure background and put weight on peak if using Rwp
  purePeakWS->mutableE(0).assign(purePeakWS->y(0).size(), 1.0);
  size_t i = 0;
  std::transform(purePeakWS->y(0).cbegin(), purePeakWS->y(0).cend(), purePeakWS->mutableY(0).begin(),
                 [=](const double &y) mutable {
                   double newY = y - bkgdvalues[i++];
                   return std::max(0.0, newY);
                 });
}

//----------------------------------------------------------------------------------------------
/** Fit peak function (only. so must be pure peak).
 * In this function, the fit result will be examined if fit is 'successful' in
 * order to rule out
 * some fit with unphysical result.
 * @return :: chi-square/Rwp
 */
double FitOneSinglePeak::fitPeakFunction(const API::IPeakFunction_sptr &peakfunc, const MatrixWorkspace_sptr &dataws,
                                         size_t wsindex, double startx, double endx) {
  // Check validity and debug output
  if (!peakfunc)
    throw std::runtime_error("fitPeakFunction's input peakfunc has not been initialized.");

  m_sstream << "Function (to fit): " << peakfunc->asString() << "  From " << startx << "  to " << endx << ".\n";

  double goodness = fitFunctionSD(peakfunc, dataws, wsindex, startx, endx);

  return goodness;
}

//-----------------------------------------------------------------------
//----------------------
/** Fit peak with high background
 * Procedure:
 * 1. Fit background
 * 2. Create a new workspace with limited region
 */
void FitOneSinglePeak::highBkgdFit() {
  m_numFitCalls = 0;

  // Check and initialization
  string errmsg;
  if (!hasSetupToFitPeak(errmsg)) {
    g_log.error(errmsg);
    throw runtime_error("Object has not been set up completely to fit peak.");
  } else {
    m_sstream << "F1158: Well-setup and good to go!\n";
  }

  m_bestRwp = DBL_MAX;

  // Fit background
  if (i_minFitX == i_minPeakX || i_maxPeakX == i_maxFitX) {
    // User's input peak range cannot be trusted.  Data might be noisy
    stringstream outss;
    outss << "User specified peak range cannot be trusted!  Because peak range "
             "overlap fit window. "
          << "Number of data points in fitting window = " << i_maxFitX - i_minFitX
          << ". A UNRELIABLE algorithm is used to guess peak range. ";
    g_log.warning(outss.str());

    size_t numpts = i_maxFitX - i_minFitX;
    auto shift = static_cast<size_t>(static_cast<double>(numpts) / 6.);
    i_minPeakX += shift;
    const auto &Xdata = m_dataWS->x(m_wsIndex);
    if (i_minPeakX >= Xdata.size())
      i_minPeakX = Xdata.size() - 1;
    m_minPeakX = Xdata[i_minPeakX];

    if (i_maxPeakX < shift) {
      i_maxPeakX = 0;
    } else {
      i_maxPeakX -= shift;
    }
    m_maxPeakX = Xdata[i_maxPeakX];
  }

  m_bkgdFunc = fitBackground(m_bkgdFunc);

  // Generate partial workspace within given fit window
  MatrixWorkspace_sptr purePeakWS = genFitWindowWS();

  // Remove background to make a pure peak
  removeBackground(purePeakWS);

  // Estimate the peak height
  double est_peakheight = estimatePeakHeight(m_peakFunc, purePeakWS, 0, 0, purePeakWS->x(0).size() - 1);
  m_peakFunc->setHeight(est_peakheight);

  // Store starting setup
  m_bkupPeakFunc = backup(m_peakFunc);

  Progress progress(this, 0.0, 1.0, m_vecFWHM.size());

  // Fit with different starting values of peak width
  for (size_t i = 0; i < m_vecFWHM.size(); ++i) {
    // Restore
    if (i > 0)
      pop(m_bkupPeakFunc, m_peakFunc);

    // Set FWHM
    m_peakFunc->setFwhm(m_vecFWHM[i]);
    m_sstream << "Round " << i << " of " << m_vecFWHM.size() << ". Using proposed FWHM = " << m_vecFWHM[i] << "\n";

    // Fit
    double rwp = fitPeakFunction(m_peakFunc, purePeakWS, 0, m_minFitX, m_maxFitX);

    m_sstream << "Fit peak function cost = " << rwp << "\n";

    // Store result
    processNStoreFitResult(rwp, false);

    progress.report();
  }

  // Get best fitting peak function and Make a combo fit
  pop(m_bestPeakFunc, m_peakFunc);

  // Fit the composite function as final
  double compcost = fitCompositeFunction(m_peakFunc, m_bkgdFunc, m_dataWS, m_wsIndex, m_minFitX, m_maxFitX);
  m_bestRwp = compcost;

  m_sstream << "MultStep-Fit: Best Fitted Peak: " << m_peakFunc->asString() << ". Final " << m_costFunction << " = "
            << compcost << "\n"
            << "Number of calls on Fit = " << m_numFitCalls << "\n";
}

//----------------------------------------------------------------------------------------------
/** Push/store a fit result (function) to storage
 * @param func :: function to get parameter values stored
 * @returns :: map to store function parameter's names and value
 */
std::map<std::string, double> FitOneSinglePeak::backup(const IFunction_const_sptr &func) {
  std::map<std::string, double> funcparammap;

  // Set up
  vector<string> funcparnames = func->getParameterNames();
  size_t nParam = funcparnames.size();
  for (size_t i = 0; i < nParam; ++i) {
    double parvalue = func->getParameter(i);
    funcparammap.emplace(funcparnames[i], parvalue);
  }

  return funcparammap;
}

//----------------------------------------------------------------------------------------------
/** Push/store function parameters' error resulted from fitting
 * @param func :: function to get parameter values stored
 * @returns :: map to store function parameter's names and fitting
 * error
 */
std::map<std::string, double> FitOneSinglePeak::storeFunctionError(const IFunction_const_sptr &func) {
  // output map
  std::map<std::string, double> paramerrormap;

  // Get function error and store in output map
  vector<string> funcparnames = func->getParameterNames();
  size_t nParam = funcparnames.size();
  for (size_t i = 0; i < nParam; ++i) {
    double parerror = func->getError(i);
    paramerrormap.emplace(funcparnames[i], parerror);
  }

  return paramerrormap;
}

//----------------------------------------------------------------------------------------------
/** Restore the parameters value to a function from a string/double map
 */
void FitOneSinglePeak::pop(const std::map<std::string, double> &funcparammap, const API::IFunction_sptr &func) {
  std::map<std::string, double>::const_iterator miter;
  for (miter = funcparammap.begin(); miter != funcparammap.end(); ++miter) {
    string parname = miter->first;
    double parvalue = miter->second;
    func->setParameter(parname, parvalue);
  }
}

//----------------------------------------------------------------------------------------------
/** Calcualte chi-square for single domain data
 * @brief FitOneSinglePeak::calChiSquareSD
 * @param fitfunc
 * @param dataws
 * @param wsindex
 * @param xmin
 * @param xmax
 * @return
 */
double FitOneSinglePeak::calChiSquareSD(const IFunction_sptr &fitfunc, const MatrixWorkspace_sptr &dataws,
                                        size_t wsindex, double xmin, double xmax) {
  // Set up sub algorithm fit
  IAlgorithm_sptr fit;
  try {
    fit = createChildAlgorithm("CalculateChiSquared", -1, -1, false);
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
  fit->setProperty("StartX", xmin);
  fit->setProperty("EndX", xmax);

  fit->executeAsChildAlg();
  if (!fit->isExecuted()) {
    g_log.error("Fit for background is not executed. ");
    throw std::runtime_error("Fit for background is not executed. ");
  }

  // Retrieve result
  const double chi2 = fit->getProperty("ChiSquaredWeightedDividedByNData");

  return chi2;
}

//----------------------------------------------------------------------------------------------
/** Fit function in single domain
 * @exception :: (1) Fit cannot be called. (2) Fit.isExecuted is false (cannot
 * be executed)
 * @return :: chi^2 or Rwp depending on input.  If fit is not SUCCESSFUL,
 * return DBL_MAX
 */
double FitOneSinglePeak::fitFunctionSD(IFunction_sptr fitfunc, const MatrixWorkspace_sptr &dataws, size_t wsindex,
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
  m_sstream << "[F1201] FitSingleDomain Fitted-Function " << fitfunc->asString() << ": Fit-status = " << fitStatus
            << ", chi^2 = " << chi2 << ".\n";

  return chi2;
}

//----------------------------------------------------------------------------------------------
/** Fit function in multi-domain
 * @param fitfunc :: function to fit
 * @param dataws :: matrix workspace to fit with
 * @param wsindex :: workspace index of the spectrum in matrix workspace
 * @param vec_xmin :: minimin values of domains
 * @param vec_xmax :: maximim values of domains
 */
double FitOneSinglePeak::fitFunctionMD(const IFunction_sptr &fitfunc, const MatrixWorkspace_sptr &dataws,
                                       size_t wsindex, vector<double> vec_xmin, vector<double> vec_xmax) {
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
  std::shared_ptr<MultiDomainFunction> funcmd = std::make_shared<MultiDomainFunction>();

  // After a change of the default value of NumDeriv in MultiDomainFunction
  // this needs to be set to false to preserve the original behaviour.
  // Results of this algorithm as well as algorithms that use it
  // seem to be very sensitive to the accuracy of the derivatives.
  funcmd->setAttributeValue("NumDeriv", false);

  // Set function first
  funcmd->addFunction(fitfunc);

  // set domain for function with index 0 covering both sides
  funcmd->clearDomainIndices();
  std::vector<size_t> ii(2);
  ii[0] = 0;
  ii[1] = 1;
  funcmd->setDomainIndices(0, ii);

  // Set the properties
  fit->setProperty("Function", std::dynamic_pointer_cast<IFunction>(funcmd));
  fit->setProperty("InputWorkspace", dataws);
  fit->setProperty("WorkspaceIndex", static_cast<int>(wsindex));
  fit->setProperty("StartX", vec_xmin[0]);
  fit->setProperty("EndX", vec_xmax[0]);
  fit->setProperty("InputWorkspace_1", dataws);
  fit->setProperty("WorkspaceIndex_1", static_cast<int>(wsindex));
  fit->setProperty("StartX_1", vec_xmin[1]);
  fit->setProperty("EndX_1", vec_xmax[1]);
  fit->setProperty("MaxIterations", 50);
  fit->setProperty("Minimizer", m_minimizer);
  fit->setProperty("CostFunction", "Least squares");

  m_sstream << "FitMultiDomain: Funcion " << funcmd->name() << ": "
            << "Range: (" << vec_xmin[0] << ", " << vec_xmax[0] << ") and (" << vec_xmin[1] << ", " << vec_xmax[1]
            << "); " << funcmd->asString() << "\n";

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
    m_sstream << "FitMultidomain: Successfully-Fitted Function " << fitfunc->asString() << ", Chi^2 = " << chi2 << "\n";
  }

  return chi2;
}

//----------------------------------------------------------------------------------------------
/** Fit peak function and background function as composite function
 * @param peakfunc :: peak function to fit
 * @param bkgdfunc :: background function to fit
 * @param dataws :: matrix workspace to fit with
 * @param wsindex :: workspace index of the spectrum in matrix workspace
 * @param startx :: minimum x value of the fitting window
 * @param endx :: maximum x value of the fitting window
 * @return :: Rwp/chi2
 */
double FitOneSinglePeak::fitCompositeFunction(const API::IPeakFunction_sptr &peakfunc,
                                              const API::IBackgroundFunction_sptr &bkgdfunc,
                                              const API::MatrixWorkspace_sptr &dataws, size_t wsindex, double startx,
                                              double endx) {
  // Construct composit function
  std::shared_ptr<CompositeFunction> compfunc = std::make_shared<CompositeFunction>();
  compfunc->addFunction(peakfunc);
  compfunc->addFunction(bkgdfunc);

  // Do calculation for starting chi^2/Rwp: as the assumption that the input the
  // so far the best Rwp
  // FIXME - This is not a good practise...
  double backRwp = calChiSquareSD(bkgdfunc, dataws, wsindex, startx, endx);
  m_sstream << "Background: Pre-fit Goodness = " << backRwp << "\n";
  m_bestRwp = calChiSquareSD(compfunc, dataws, wsindex, startx, endx);
  m_sstream << "Peak+Background: Pre-fit Goodness = " << m_bestRwp << "\n";

  auto bkuppeakmap = backup(peakfunc);
  auto bkupbkgdmap = backup(bkgdfunc);
  m_fitErrorPeakFunc = storeFunctionError(peakfunc);
  m_fitErrorBkgdFunc = storeFunctionError(bkgdfunc);

  // Fit
  double goodness = fitFunctionSD(compfunc, dataws, wsindex, startx, endx);
  string errorreason;

  // Check fit result
  goodness = checkFittedPeak(peakfunc, goodness, errorreason);

  if (!errorreason.empty())
    m_sstream << "Error reason of fit peak+background composite: " << errorreason << "\n";

  double goodness_final = DBL_MAX;
  if (goodness <= m_bestRwp && goodness <= backRwp) {
    // Fit for composite function renders a better result
    goodness_final = goodness;
    processNStoreFitResult(goodness_final, true);
  } else if (goodness > m_bestRwp && m_bestRwp < DBL_MAX && m_bestRwp <= backRwp) {
    // A worse result is got.  Revert to original function parameters
    m_sstream << "Fit peak/background composite function FAILS to render a "
                 "better solution. "
              << "Input cost function value = " << m_bestRwp << ", output cost function value = " << goodness << "\n";

    pop(bkuppeakmap, peakfunc);
    pop(bkupbkgdmap, bkgdfunc);
    goodness_final = m_bestRwp;
  } else {
    m_sstream << "Fit peak-background function fails in all approaches! \n";
  }

  return goodness_final;
}

//----------------------------------------------------------------------------------------------
/** Check the fitted peak value to see whether it is valid
 * @return :: Rwp/chi2
 */
double FitOneSinglePeak::checkFittedPeak(const IPeakFunction_sptr &peakfunc, double costfuncvalue,
                                         std::string &errorreason) {
  if (costfuncvalue < DBL_MAX) {
    // Fit is successful.  Check whether the fit result is physical
    stringstream errorss;
    double peakcentre = peakfunc->centre();
    if (peakcentre < m_minPeakX || peakcentre > m_maxPeakX) {
      errorss << "Peak centre (at " << peakcentre << " ) is out of specified range )" << m_minPeakX << ", "
              << m_maxPeakX << "). ";
      costfuncvalue = DBL_MAX;
    }

    double peakheight = peakfunc->height();
    if (peakheight < 0) {
      errorss << "Peak height (" << peakheight << ") is negative. ";
      costfuncvalue = DBL_MAX;
    }
    double peakfwhm = peakfunc->fwhm();
    if (peakfwhm > (m_maxFitX - m_minFitX) * MAGICNUMBER) {
      errorss << "Peak width is unreasonably wide. ";
      costfuncvalue = DBL_MAX;
    }
    errorreason = errorss.str();
  } else {
    // Fit is not successful
    errorreason = "Fit() on peak function is NOT successful.";
  }

  return costfuncvalue;
}

//----------------------------------------------------------------------------------------------
/** Fit background of a given peak in a given range
 * @param bkgdfunc :: background function to fit
 * @return :: background function fitted
 */
API::IBackgroundFunction_sptr FitOneSinglePeak::fitBackground(API::IBackgroundFunction_sptr bkgdfunc) {
  // Back up background function
  m_bkupBkgdFunc = backup(bkgdfunc);

  // Fit in multiple domain
  vector<double> vec_xmin(2);
  vector<double> vec_xmax(2);
  vec_xmin[0] = m_minFitX;
  vec_xmin[1] = m_maxPeakX;
  vec_xmax[0] = m_minPeakX;
  vec_xmax[1] = m_maxFitX;
  double chi2 = fitFunctionMD(std::dynamic_pointer_cast<IFunction>(bkgdfunc), m_dataWS, m_wsIndex, vec_xmin, vec_xmax);

  // Process fit result
  if (chi2 < DBL_MAX - 1) {
    // Store fitting result
    m_bestBkgdFunc = backup(bkgdfunc);
    m_fitErrorBkgdFunc = storeFunctionError(bkgdfunc);
  } else {
    // Restore background function
    pop(m_bkupBkgdFunc, bkgdfunc);
  }

  return bkgdfunc;
}

//----------------------------------------------------------------------------------------------
/** Process and store fitting reuslt
 * @param rwp :: Rwp of the fitted function to the data
 * @param storebkgd :: flag to store the background function value or not
 */
void FitOneSinglePeak::processNStoreFitResult(double rwp, bool storebkgd) {
  bool fitsuccess = true;
  string failreason;

  if (rwp < DBL_MAX) {
    // A valid Rwp returned from Fit

    // Check non-negative height
    double f_height = m_peakFunc->height();
    if (f_height <= 0.) {
      rwp = DBL_MAX;
      failreason += "Negative peak height. ";
      fitsuccess = false;
    }

    // Check peak position
    double f_centre = m_peakFunc->centre();
    if (m_usePeakPositionTolerance) {
      // Peak position criteria is on position tolerance
      if (!Kernel::withinAbsoluteDifference(f_centre, m_userPeakCentre, m_peakPositionTolerance)) {
        rwp = DBL_MAX;
        failreason = "Peak centre out of tolerance. ";
        fitsuccess = false;
      }
    } else if (f_centre < m_minPeakX || f_centre > m_maxPeakX) {
      rwp = DBL_MAX;
      failreason += "Peak centre out of input peak range ";
      m_sstream << "Peak centre " << f_centre << " is out of peak range: " << m_minPeakX << ", " << m_maxPeakX << "\n";
      fitsuccess = false;
    }

  } // RWP fine
  else {
    failreason = "(Single-step) Fit returns a DBL_MAX.";
    fitsuccess = false;
  }

  m_sstream << "Process fit result: "
            << "Rwp = " << rwp << ", best Rwp = " << m_bestRwp << ", Fit success = " << fitsuccess << ". ";

  // Store result if
  if (rwp < m_bestRwp && fitsuccess) {
    m_bestPeakFunc = backup(m_peakFunc);
    m_fitErrorPeakFunc = storeFunctionError(m_peakFunc);
    if (storebkgd) {
      m_bestBkgdFunc = backup(m_bkgdFunc);
      m_fitErrorBkgdFunc = storeFunctionError(m_bkgdFunc);
    }
    m_bestRwp = rwp;

    m_sstream << "Store result and new Best RWP = " << m_bestRwp << ".\n";
  } else if (!fitsuccess) {
    m_sstream << "Reason of fit's failure: " << failreason << "\n";
  }
}

//----------------------------------------------------------------------------------------------
/** Get the cost function value of the best fit
 */
double FitOneSinglePeak::getFitCostFunctionValue() { return m_bestRwp; }

//----------------------------------------------------------------------------------------------
void FitOneSinglePeak::exec() { throw runtime_error("Not used."); }

//----------------------------------------------------------------------------------------------
void FitOneSinglePeak::init() { throw runtime_error("Not used."); }

//----------------------------------------------------------------------------------------------
// FitPeak Methods
//----------------------------------------------------------------------------------------------

DECLARE_ALGORITHM(FitPeak)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
FitPeak::FitPeak()
    : API::Algorithm(), m_dataWS(), m_wsIndex(0), m_peakFunc(), m_bkgdFunc(), m_minFitX(0.), m_maxFitX(0.),
      m_minPeakX(0.), m_maxPeakX(0.), i_minFitX(0), i_maxFitX(0), i_minPeakX(0), i_maxPeakX(0), m_fitBkgdFirst(false),
      m_outputRawParams(false), m_userGuessedFWHM(0.), m_userPeakCentre(0.), m_minGuessedPeakWidth(0),
      m_maxGuessedPeakWidth(0), m_fwhmFitStep(0), m_fitWithStepPeakWidth(false), m_usePeakPositionTolerance(false),
      m_peakPositionTolerance(0.), m_peakParameterTableWS(), m_bkgdParameterTableWS(), m_peakParameterNames(),
      m_bkgdParameterNames(), m_minimizer("Levenberg-MarquardtMD"), m_bkupBkgdFunc(), m_bkupPeakFunc(),
      m_bestPeakFunc(), m_bestBkgdFunc(), m_bestRwp(DBL_MAX), m_finalGoodnessValue(0.), m_vecybkup(), m_vecebkup(),
      m_costFunction(), m_lightWeightOutput(false) {}

//----------------------------------------------------------------------------------------------
/** Declare properties
 */
void FitPeak::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Name of the input workspace for peak fitting.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace containing fitted peak.");

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("ParameterTableWorkspace", "", Direction::Output),
                  "Name of the table workspace containing the fitted parameters. ");

  std::shared_ptr<BoundedValidator<int>> mustBeNonNegative = std::make_shared<BoundedValidator<int>>();
  mustBeNonNegative->setLower(0);
  declareProperty("WorkspaceIndex", 0, mustBeNonNegative, "Workspace index ");

  std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
  vector<string> peakFullNames = addFunctionParameterNames(peakNames);
  declareProperty("PeakFunctionType", "", std::make_shared<StringListValidator>(peakFullNames), "Peak function type. ");

  declareProperty(std::make_unique<ArrayProperty<string>>("PeakParameterNames"), "List of peak parameter names. ");

  declareProperty(std::make_unique<ArrayProperty<double>>("PeakParameterValues"),
                  "List of peak parameter values.  They must have a 1-to-1 "
                  "mapping to PeakParameterNames list. ");

  declareProperty(std::make_unique<ArrayProperty<double>>("FittedPeakParameterValues", Direction::Output),
                  "Fitted peak parameter values. ");

  vector<string> bkgdtypes{"Flat", "Flat (A0)", "Linear", "Linear (A0, A1)", "Quadratic", "Quadratic (A0, A1, A2)"};
  declareProperty("BackgroundType", "Linear", std::make_shared<StringListValidator>(bkgdtypes), "Type of Background.");

  declareProperty(std::make_unique<ArrayProperty<string>>("BackgroundParameterNames"),
                  "List of background parameter names. ");

  declareProperty(std::make_unique<ArrayProperty<double>>("BackgroundParameterValues"),
                  "List of background parameter values.  "
                  "They must have a 1-to-1 mapping to BackgroundParameterNames list. ");

  declareProperty(std::make_unique<ArrayProperty<double>>("FittedBackgroundParameterValues", Direction::Output),
                  "Fitted background parameter values. ");

  declareProperty(std::make_unique<ArrayProperty<double>>("FitWindow"),
                  "Enter a comma-separated list of the expected X-position of "
                  "windows to fit. "
                  "The number of values must be 2.");

  declareProperty(std::make_unique<ArrayProperty<double>>("PeakRange"),
                  "Enter a comma-separated list of expected x-position as peak range. "
                  "The number of values must be 2.");

  declareProperty("FitBackgroundFirst", true,
                  "If true, then the algorithm will fit background first. "
                  "And then the peak. ");

  declareProperty("RawParams", true,
                  "If true, then the output table workspace contains the raw "
                  "profile parameter. "
                  "Otherwise, the effective parameters will be written. ");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  declareProperty("MinGuessedPeakWidth", 2, mustBePositive,
                  "Minimum guessed peak width for fit. It is in unit of number of pixels.");

  declareProperty("MaxGuessedPeakWidth", 10, mustBePositive,
                  "Maximum guessed peak width for fit. It is in unit of number of pixels.");

  declareProperty("GuessedPeakWidthStep", EMPTY_INT(), mustBePositive,
                  "Step of guessed peak width. It is in unit of number of pixels.");

  auto mustBePostiveDbl = std::make_shared<BoundedValidator<double>>();
  mustBePostiveDbl->setLower(DBL_MIN);
  declareProperty("PeakPositionTolerance", EMPTY_DBL(), mustBePostiveDbl,
                  "Peak position tolerance.  If fitted peak's position differs "
                  "from proposed value more than "
                  "the given value, fit is treated as failure. ");

  std::array<string, 2> costFuncOptions = {{"Chi-Square", "Rwp"}};
  declareProperty("CostFunction", "Chi-Square",
                  Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(costFuncOptions)), "Cost functions");

  std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();

  declareProperty("Minimizer", "Levenberg-Marquardt",
                  Kernel::IValidator_sptr(new Kernel::StartsWithValidator(minimizerOptions)),
                  "Minimizer to use for fitting. Minimizers available are "
                  "\"Levenberg-Marquardt\", \"Simplex\","
                  "\"Conjugate gradient (Fletcher-Reeves imp.)\", \"Conjugate "
                  "gradient (Polak-Ribiere imp.)\", \"BFGS\", and "
                  "\"Levenberg-MarquardtMD\"");

  declareProperty("CostFunctionValue", DBL_MAX, "Value of cost function of the fitted peak. ",
                  Kernel::Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Declare properties
 */
void FitPeak::exec() {
  // Get input properties
  processProperties();

  // Create functions
  createFunctions();

  // Check input function, guessed value, and etc.
  prescreenInputData();

  // Set parameters to fit
  FitOneSinglePeak fit1peakalg;

  fit1peakalg.setFunctions(m_peakFunc, m_bkgdFunc);
  fit1peakalg.setWorskpace(m_dataWS, m_wsIndex);

  fit1peakalg.setFittingMethod(m_minimizer, m_costFunction);
  fit1peakalg.setFitWindow(m_minFitX, m_maxFitX);
  fit1peakalg.setPeakRange(m_minPeakX, m_maxPeakX);
  fit1peakalg.setupGuessedFWHM(m_peakFunc->fwhm(), m_minGuessedPeakWidth, m_maxGuessedPeakWidth, m_fwhmFitStep,
                               m_fitWithStepPeakWidth);

  fit1peakalg.setFitPeakCriteria(m_usePeakPositionTolerance, m_peakPositionTolerance);

  if (m_fitBkgdFirst) {
    fit1peakalg.highBkgdFit();
  } else {
    fit1peakalg.simpleFit();
  }
  string dbmsg = fit1peakalg.getDebugMessage();
  g_log.information(dbmsg);

  m_finalGoodnessValue = fit1peakalg.getFitCostFunctionValue();

  // Output
  setupOutput(fit1peakalg.getPeakError(), fit1peakalg.getBackgroundError());
}

//----------------------------------------------------------------------------------------------
/** Add function's parameter names after peak function name
 */
std::vector<std::string> FitPeak::addFunctionParameterNames(const std::vector<std::string> &funcnames) {
  vector<string> vec_funcparnames;

  for (auto &funcname : funcnames) {
    // Add original name in
    vec_funcparnames.emplace_back(funcname);

    // Add a full function name and parameter names in
    IFunction_sptr tempfunc = FunctionFactory::Instance().createFunction(funcname);

    stringstream parnamess;
    parnamess << funcname << " (";
    vector<string> funcpars = tempfunc->getParameterNames();
    for (size_t j = 0; j < funcpars.size(); ++j) {
      parnamess << funcpars[j];
      if (j != funcpars.size() - 1)
        parnamess << ", ";
    }
    parnamess << ")";

    vec_funcparnames.emplace_back(parnamess.str());
  }

  return vec_funcparnames;
}

//----------------------------------------------------------------------------------------------
/** Process input properties
 * Including: dataWS, wsIdex, fitWindow, peak range
 */
void FitPeak::processProperties() {
  // Data workspace (input)
  m_dataWS = getProperty("InputWorkspace");
  int tempint = getProperty("WorkspaceIndex");
  m_wsIndex = static_cast<size_t>(tempint);

  // Fit window
  auto &vecX = m_dataWS->x(m_wsIndex);

  vector<double> fitwindow = getProperty("FitWindow");
  if (fitwindow.size() != 2) {
    throw runtime_error("Must enter 2 and only 2 items in fit window. ");
  }
  m_minFitX = fitwindow[0];
  if (m_minFitX < vecX.front())
    m_minFitX = vecX.front();
  m_maxFitX = fitwindow[1];
  if (m_maxFitX > vecX.back())
    m_maxFitX = vecX.back();

  if (m_maxFitX <= m_minFitX) {
    stringstream errss;
    errss << "Minimum X (" << m_minFitX << ") is larger and equal to maximum X (" << m_maxFitX
          << ") to fit.  It is not allowed. ";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  // Peak range
  vector<double> peakrange = getProperty("PeakRange");
  if (peakrange.size() != 2) {
    throw runtime_error("Must enter 2 and only 2 items for PeakRange in fit window. ");
  }
  m_minPeakX = peakrange[0];
  m_maxPeakX = peakrange[1];
  if (m_maxFitX <= m_minFitX) {
    stringstream errss;
    errss << "Minimum peak range (" << m_minPeakX << ") is larger and equal to maximum X (" << m_maxPeakX
          << ") of the range of peak.  It is not allowed. ";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  if (m_minPeakX < m_minFitX) {
    m_minPeakX = m_minFitX;
    g_log.warning() << "Minimum peak range is out side of the lower boundary "
                       "of fit window.  ";
  }
  if (m_maxPeakX > m_maxFitX) {
    m_maxPeakX = m_maxFitX;
    g_log.warning() << "Maximum peak range is out side of the upper boundary "
                       "of fit window. ";
  }

  // Fit strategy
  m_fitBkgdFirst = getProperty("FitBackgroundFirst");

  // Trying FWHM in a certain range
  m_minGuessedPeakWidth = getProperty("MinGuessedPeakWidth");
  m_maxGuessedPeakWidth = getProperty("MaxGuessedPeakWidth");
  m_fwhmFitStep = getProperty("GuessedPeakWidthStep");
  if (isEmpty(m_fwhmFitStep))
    m_fitWithStepPeakWidth = false;
  else {
    m_fitWithStepPeakWidth = true;
    if (m_minGuessedPeakWidth > m_maxGuessedPeakWidth) {
      std::stringstream errss;
      errss << "User specified wrong guessed peak width parameters (must be "
               "postive and make sense). "
            << "User inputs are min = " << m_minGuessedPeakWidth << ", max = " << m_maxGuessedPeakWidth
            << ", step = " << m_fwhmFitStep;
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }
  }

  // Tolerance
  m_peakPositionTolerance = getProperty("PeakPositionTolerance");
  if (isEmpty(m_peakPositionTolerance))
    m_usePeakPositionTolerance = false;
  else
    m_usePeakPositionTolerance = true;

  // Cost function
  string costfunname = getProperty("CostFunction");
  if (costfunname == "Chi-Square") {
    m_costFunction = "Least squares";
  } else if (costfunname == "Rwp") {
    m_costFunction = "Rwp";
  } else {
    g_log.error() << "Cost function " << costfunname << " is not supported. "
                  << "\n";
    throw runtime_error("Cost function is not supported. ");
  }

  // Minimizer
  m_minimizer = getPropertyValue("Minimizer");

  // Output option
  m_outputRawParams = getProperty("RawParams");
}

//----------------------------------------------------------------------------------------------
/** Create functions from input properties
 */
void FitPeak::createFunctions() {
  //=========================================================================
  // Generate background function
  //=========================================================================
  // Get background type
  string bkgdtyperaw = getPropertyValue("BackgroundType");
  bool usedefaultbkgdparorder = false;
  string bkgdtype = parseFunctionTypeFull(bkgdtyperaw, usedefaultbkgdparorder);

  // FIXME - Fix the inconsistency in nameing the background
  if (bkgdtype == "Flat" || bkgdtype == "Linear")
    bkgdtype += "Background";

  // Generate background function
  m_bkgdFunc = std::dynamic_pointer_cast<IBackgroundFunction>(FunctionFactory::Instance().createFunction(bkgdtype));

  // Set background function parameter values
  m_bkgdParameterNames = getProperty("BackgroundParameterNames");
  if (usedefaultbkgdparorder && m_bkgdParameterNames.empty()) {
    m_bkgdParameterNames = m_bkgdFunc->getParameterNames();
  } else if (m_bkgdParameterNames.empty()) {
    throw runtime_error("In the non-default background parameter name mode, "
                        "user must give out parameter names. ");
  }

  vector<double> vec_bkgdparvalues = getProperty("BackgroundParameterValues");
  if (m_bkgdParameterNames.size() != vec_bkgdparvalues.size()) {
    stringstream errss;
    errss << "Input background properties' arrays are incorrect: # of "
             "parameter names = "
          << m_bkgdParameterNames.size() << ", # of parameter values = " << vec_bkgdparvalues.size() << "\n";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  // Set parameter values
  for (size_t i = 0; i < m_bkgdParameterNames.size(); ++i) {
    m_bkgdFunc->setParameter(m_bkgdParameterNames[i], vec_bkgdparvalues[i]);
  }

  //=========================================================================
  // Generate peak function
  //=========================================================================
  string peaktypeprev = getPropertyValue("PeakFunctionType");
  bool defaultparorder = true;
  string peaktype = parseFunctionTypeFull(peaktypeprev, defaultparorder);
  m_peakFunc = std::dynamic_pointer_cast<IPeakFunction>(FunctionFactory::Instance().createFunction(peaktype));

  // Peak parameters' names
  m_peakParameterNames = getProperty("PeakParameterNames");
  if (m_peakParameterNames.empty()) {
    if (defaultparorder) {
      // Use default peak parameter names' order
      m_peakParameterNames = m_peakFunc->getParameterNames();
    } else {
      throw runtime_error("Peak parameter names' input is not in default mode. "
                          "It cannot be left empty. ");
    }
  }

  // Peak parameters' value
  vector<double> vec_peakparvalues = getProperty("PeakParameterValues");
  if (m_peakParameterNames.size() != vec_peakparvalues.size()) {
    stringstream errss;
    errss << "Input peak properties' arrays are incorrect: # of parameter "
             "names = "
          << m_peakParameterNames.size() << ", # of parameter values = " << vec_peakparvalues.size() << "\n";
    throw runtime_error(errss.str());
  }

  // Set peak parameter values
  for (size_t i = 0; i < m_peakParameterNames.size(); ++i) {
    m_peakFunc->setParameter(m_peakParameterNames[i], vec_peakparvalues[i]);
  }
}

//----------------------------------------------------------------------------------------------
/** Parse peak type from full peak type/parameter names string
 */
std::string FitPeak::parseFunctionTypeFull(const std::string &fullstring, bool &defaultparorder) {
  string peaktype;

  size_t n = std::count(fullstring.begin(), fullstring.end(), '(');
  if (n > 0) {
    peaktype = fullstring.substr(0, fullstring.find('('));
    boost::algorithm::trim(peaktype);
    defaultparorder = true;
  } else {
    peaktype = fullstring;
    defaultparorder = false;
  }

  return peaktype;
}

//----------------------------------------------------------------------------------------------
/** Check input data and get some information parameters
 */
void FitPeak::prescreenInputData() {
  // Check functions
  if (!m_peakFunc || !m_bkgdFunc)
    throw runtime_error("Either peak function or background function has not been set up.");

  // Check validity on peak centre
  double centre_guess = m_peakFunc->centre();
  if (m_minFitX >= centre_guess || m_maxFitX <= centre_guess)
    throw runtime_error("Peak centre is out side of fit window. ");

  // Peak width and centre: from user input
  m_userGuessedFWHM = m_peakFunc->fwhm();
  m_userPeakCentre = m_peakFunc->centre();
}

//----------------------------------------------------------------------------------------------
/** Set up the output workspaces
 * including (1) data workspace (2) function parameter workspace
 */
void FitPeak::setupOutput(const std::map<std::string, double> &m_fitErrorPeakFunc,
                          const std::map<std::string, double> &m_fitErrorBkgdFunc) {
  // TODO - Need to retrieve useful information from FitOneSinglePeak object
  // (think of how)
  const auto &vecX = m_dataWS->x(m_wsIndex);
  const size_t i_minFitX = getIndex(vecX, m_minFitX);
  const size_t i_maxFitX = getIndex(vecX, m_maxFitX);

  // Data workspace
  const size_t nspec = 3;
  // Get a vector for fit window

  vector<double> vecoutx(i_maxFitX - i_minFitX + 1);
  for (size_t i = i_minFitX; i <= i_maxFitX; ++i)
    vecoutx[i - i_minFitX] = vecX[i];

  // Create workspace
  const size_t sizex = vecoutx.size();
  const auto sizey = sizex;
  HistogramBuilder builder;
  builder.setX(sizex);
  builder.setY(sizey);
  MatrixWorkspace_sptr outws = create<Workspace2D>(nspec, builder.build());
  // Calculate again
  FunctionDomain1DVector domain(vecoutx);
  FunctionValues values(domain);

  CompositeFunction_sptr compfunc = std::make_shared<CompositeFunction>();
  compfunc->addFunction(m_peakFunc);
  compfunc->addFunction(m_bkgdFunc);
  compfunc->function(domain, values);

  const auto domainVec = domain.toVector();
  outws->mutableX(0).assign(domainVec.cbegin(), domainVec.cend());
  outws->setSharedX(1, outws->sharedX(0));
  outws->setSharedX(2, outws->sharedX(0));

  auto &vecY = m_dataWS->y(m_wsIndex);
  const auto valvec = values.toVector();
  outws->mutableY(0).assign(vecY.cbegin() + i_minFitX, vecY.cbegin() + i_minFitX + sizey);
  outws->mutableY(1).assign(valvec.cbegin(), valvec.cbegin() + sizey);
  std::transform(outws->y(0).cbegin(), outws->y(0).cbegin() + sizey, outws->y(1).cbegin(), outws->mutableY(2).begin(),
                 std::minus<double>());
  // Set property
  setProperty("OutputWorkspace", outws);

  // Function parameter table workspaces
  TableWorkspace_sptr peaktablews = genOutputTableWS(m_peakFunc, m_fitErrorPeakFunc, m_bkgdFunc, m_fitErrorBkgdFunc);
  setProperty("ParameterTableWorkspace", peaktablews);

  // Parameter vector
  vector<double> vec_fitpeak;
  vec_fitpeak.reserve(m_peakParameterNames.size());
  std::transform(m_peakParameterNames.cbegin(), m_peakParameterNames.cend(), std::back_inserter(vec_fitpeak),
                 [this](const auto &peakParameterName) { return m_peakFunc->getParameter(peakParameterName); });

  setProperty("FittedPeakParameterValues", vec_fitpeak);

  // Background
  vector<double> vec_fitbkgd;
  vec_fitbkgd.reserve(m_bkgdParameterNames.size());
  std::transform(m_bkgdParameterNames.cbegin(), m_bkgdParameterNames.cend(), std::back_inserter(vec_fitbkgd),
                 [this](const auto &bkgdParameterName) { return m_bkgdFunc->getParameter(bkgdParameterName); });

  setProperty("FittedBackgroundParameterValues", vec_fitbkgd);

  // Output chi^2 or Rwp
  setProperty("CostFunctionValue", m_finalGoodnessValue);
}

//----------------------------------------------------------------------------------------------
/** Get an index of a value in a sorted vector.  The index should be the item
 * with value nearest to X
 */
size_t getIndex(const HistogramX &vecx, double x) {
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
/** Generate table workspace
 */
TableWorkspace_sptr FitPeak::genOutputTableWS(const IPeakFunction_sptr &peakfunc, map<string, double> peakerrormap,
                                              const IBackgroundFunction_sptr &bkgdfunc,
                                              map<string, double> bkgderrormap) {
  // Empty table
  TableWorkspace_sptr outtablews = std::make_shared<TableWorkspace>();
  outtablews->addColumn("str", "Name");
  outtablews->addColumn("double", "Value");
  outtablews->addColumn("double", "Error");

  // Set chi^2
  TableRow newrow = outtablews->appendRow();
  newrow << "ChiSquare" << m_finalGoodnessValue;

  // Set peak paraemters
  newrow = outtablews->appendRow();
  newrow << peakfunc->name();

  if (m_outputRawParams) {
    vector<string> peakparnames = peakfunc->getParameterNames();
    for (auto &parname : peakparnames) {
      double parvalue = peakfunc->getParameter(parname);
      double error = peakerrormap[parname];
      newrow = outtablews->appendRow();
      newrow << parname << parvalue << error;
    }
  } else {
    newrow = outtablews->appendRow();
    newrow << "centre" << peakfunc->centre();

    newrow = outtablews->appendRow();
    newrow << "width" << peakfunc->fwhm();

    newrow = outtablews->appendRow();
    newrow << "height" << peakfunc->height();
  }

  // Set background paraemters
  newrow = outtablews->appendRow();
  newrow << bkgdfunc->name();

  if (m_outputRawParams) {
    vector<string> bkgdparnames = bkgdfunc->getParameterNames();
    for (auto &parname : bkgdparnames) {
      double parvalue = bkgdfunc->getParameter(parname);
      double error = bkgderrormap[parname];
      newrow = outtablews->appendRow();
      newrow << parname << parvalue << error;
    }
  } else {
    string bkgdtype = getProperty("BackgroundType");

    newrow = outtablews->appendRow();
    newrow << "backgroundintercept" << bkgdfunc->getParameter("A0");
    if (bkgdtype != "Flat") {
      newrow = outtablews->appendRow();
      newrow << "backgroundintercept" << bkgdfunc->getParameter("A1");
    }
    if (bkgdtype == "Quadratic") {
      newrow = outtablews->appendRow();
      newrow << "A2" << bkgdfunc->getParameter("A2");
    }
  }

  return outtablews;
}

} // namespace Mantid::Algorithms
