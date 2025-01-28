// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/RefinePowderInstrumentParameters.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Statistics.h"

#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/Functions/BackgroundFunction.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/Polynomial.h"

#include "MantidGeometry/Crystal/UnitCell.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fstream>
#include <iomanip>
#include <utility>

#include <gsl/gsl_sf_erf.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting::Constraints;
using namespace Mantid::HistogramData;

using namespace std;

namespace Mantid::CurveFitting::Algorithms {

DECLARE_ALGORITHM(RefinePowderInstrumentParameters)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
RefinePowderInstrumentParameters::RefinePowderInstrumentParameters()
    : m_BestGSLChi2(0.0), m_MinSigma(0.0), m_MinNumFittedPeaks(0), m_MaxNumberStoredParameters(0) {
  this->useAlgorithm("RefinePowderInstrumentParameters", 3);
}

//----------------------------------------------------------------------------------------------
/** Parameter declaration
 */
void RefinePowderInstrumentParameters::init() {
  // Input/output peaks table workspace
  declareProperty(std::make_unique<API::WorkspaceProperty<DataObjects::TableWorkspace>>("BraggPeakParameterWorkspace",
                                                                                        "Anonymous", Direction::Input),
                  "TableWorkspace containg all peaks' parameters.");

  // Input and output instrument parameters table workspace
  declareProperty(std::make_unique<API::WorkspaceProperty<DataObjects::TableWorkspace>>(
                      "InstrumentParameterWorkspace", "AnonymousInstrument", Direction::InOut),
                  "TableWorkspace containg instrument's parameters.");

  // Output workspace
  declareProperty(std::make_unique<API::WorkspaceProperty<DataObjects::Workspace2D>>("OutputWorkspace", "AnonymousOut",
                                                                                     Direction::Output),
                  "Output Workspace2D for the d-TOF curves. ");

  // Workspace to output fitted peak parameters
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("OutputInstrumentParameterWorkspace",
                                                                      "AnonymousOut2", Direction::Output),
                  "Output TableWorkspace for the fitted peak parameters for each peak.");

  // Workspace to output N best MC parameters
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("OutputBestResultsWorkspace", "",
                                                                      Direction::Output, PropertyMode::Optional),
                  "Output TableWorkspace for the N best MC fitting results. ");

  // Lower limit on number of peaks for fitting
  declareProperty("MinNumberFittedPeaks", 5, "Minimum number of fitted peaks for refining instrument parameters.");

  // Refinement algorithm
  vector<string> algoptions{"DirectFit", "MonteCarlo"};
  auto validator = std::make_shared<Kernel::StringListValidator>(algoptions);
  declareProperty("RefinementAlgorithm", "MonteCarlo", validator, "Algorithm to refine the instrument parameters.");

  declareProperty("RandomWalkSteps", 10000, "Number of Monte Carlo random walk steps. ");

  // Parameters to fit
  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>("ParametersToFit"),
                  "Names of the parameters to fit. ");

  // Mininum allowed peak's sigma (avoid wrong fitting peak with very narrow
  // width)
  declareProperty("MinSigma", 1.0, "Minimum allowed value for Sigma of a peak.");

  // Method to calcualte the standard error of peaks
  vector<string> stdoptions{"ConstantValue", "InvertedPeakHeight"};
  auto listvalidator = std::make_shared<Kernel::StringListValidator>(stdoptions);
  declareProperty("StandardError", "ConstantValue", listvalidator,
                  "Algorithm to calculate the standard error of peak positions.");

  declareProperty("NumberBestFitRecorded", 1, "Number of best fits (Monte Carlo) recorded and output. ");

  declareProperty("MonteCarloRandomSeed", 0, "Random seed for Monte Carlo simulation. ");
}

//----------------------------------------------------------------------------------------------
/** Main execution
 */
void RefinePowderInstrumentParameters::exec() {
  enum { DirectFit, MonteCarlo } refinealgorithm;

  // 1. Get input
  DataObjects::TableWorkspace_sptr peakWS = this->getProperty("BraggPeakParameterWorkspace");
  DataObjects::TableWorkspace_sptr parameterWS = this->getProperty("InstrumentParameterWorkspace");

  m_MinSigma = getProperty("MinSigma");

  int tempint = getProperty("MinNumberFittedPeaks");
  if (tempint <= 1) {
    g_log.error() << "Input MinNumberFittedPeaks = " << tempint << " is too small. \n";
    throw std::invalid_argument("Input MinNumberFittedPeaks is too small.");
  }
  m_MinNumFittedPeaks = static_cast<size_t>(tempint);

  tempint = getProperty("NumberBestFitRecorded");
  if (tempint <= 0)
    throw runtime_error("Input NumberBestFitRecorded cannot be less and equal to 0. ");
  m_MaxNumberStoredParameters = static_cast<size_t>(tempint);

  string algoption = getProperty("RefinementAlgorithm");
  if (algoption == "DirectFit")
    refinealgorithm = DirectFit;
  else if (algoption == "MonteCarlo")
    refinealgorithm = MonteCarlo;
  else
    throw runtime_error("RefinementAlgorithm other than DirectFit and "
                        "MonteCarlo are not supported.");

  // 2. Parse input table workspace
  genPeaksFromTable(peakWS);
  importParametersFromTable(parameterWS, m_FuncParameters);
  m_OrigParameters = m_FuncParameters;

  // 3. Generate a cener workspace as function of d-spacing.
  bool usemc = false;
  if (refinealgorithm == MonteCarlo)
    usemc = true;
  genPeakCentersWorkspace(usemc, m_MaxNumberStoredParameters);

  // 4. Fit instrument geometry function
  stringstream errss;
  TableWorkspace_sptr mcresultws;

  switch (refinealgorithm) {
  case DirectFit:
    // a) Simple (directly) fit all parameters
    fitInstrumentParameters();
    break;

  case MonteCarlo:
    // b) Use Monte Carlo/Annealing method to search global minimum
    refineInstrumentParametersMC(parameterWS, true);
    mcresultws = genMCResultTable();
    setProperty("OutputBestResultsWorkspace", mcresultws);
    break;

  default:
    // c) Unsupported
    errss << "Refinement algorithm " << algoption << " is not supported.  Quit!";
    g_log.error(errss.str());
    throw invalid_argument(errss.str());
    break;
  }

  // 5. Set output workspace
  this->setProperty("OutputWorkspace", m_dataWS);

  // 6. Output new instrument parameters
  DataObjects::TableWorkspace_sptr fitparamws = genOutputInstrumentParameterTable();
  this->setProperty("OutputInstrumentParameterWorkspace", fitparamws);
}

//------- Related to Fitting Instrument Geometry Function  -------------------

/** Fit instrument parameters.  It is a straight forward fitting to
 */
void RefinePowderInstrumentParameters::fitInstrumentParameters() {
  g_log.debug() << "=========== Method [FitInstrumentParameters] ===============\n";

  // 1. Initialize the fitting function
  m_Function = std::make_shared<ThermalNeutronDtoTOFFunction>();
  m_Function->initialize();

  API::FunctionDomain1DVector domain(m_dataWS->x(1).rawData());
  API::FunctionValues values(domain);
  const auto &rawY = m_dataWS->y(0);
  const auto &rawE = m_dataWS->e(0);

  // 2. Set up parameters values
  std::vector<std::string> funparamnames = m_Function->getParameterNames();

  std::vector<std::string> paramtofit = getProperty("ParametersToFit");
  std::sort(paramtofit.begin(), paramtofit.end());

  stringstream msgss;
  msgss << "Set Instrument Function Parameter : \n";
  std::map<std::string, double>::iterator paramiter;
  for (const auto &parname : funparamnames) {
    paramiter = m_FuncParameters.find(parname);
    if (paramiter == m_FuncParameters.end()) {
      // Not found and thus skip
      continue;
    }

    double parvalue = paramiter->second;
    m_Function->setParameter(parname, parvalue);
    msgss << setw(10) << parname << " = " << parvalue << '\n';
  }
  g_log.debug() << msgss.str();

  // 2b. Calculate the statistic of the starting values
  double gslchi2 = calculateFunctionStatistic(m_Function, m_dataWS, 0);
  double homchi2 = calculateD2TOFFunction(m_Function, domain, values, rawY, rawE);
  g_log.debug() << "Fit Starting Value:  Chi^2 (GSL) = " << gslchi2 << ",  Chi2^2 (Home) = " << homchi2 << '\n';

  // 3. Fix parameters that are not listed in parameter-to-fit.  Unfix the rest
  size_t numparams = funparamnames.size();
  for (size_t i = 0; i < numparams; ++i) {
    string parname = funparamnames[i];
    vector<string>::iterator vsiter;
    vsiter = std::find(paramtofit.begin(), paramtofit.end(), parname);

    if (vsiter == paramtofit.end())
      m_Function->fix(i);
    else
      m_Function->unfix(i);
  }

  // 4. Select minimizer.  Use Simplex for more than 1 parameters to fit.
  // Levenberg-MarquardtMD otherwise
  string minimizer("Levenberg-MarquardtMD");
  if (paramtofit.size() > 1) {
    minimizer = "Simplex";
  }
  g_log.information() << "Fit use minizer: " << minimizer << '\n';

  // 5. Create and setup fit algorithm
  g_log.information() << "Fit instrument geometry: " << m_Function->asString() << '\n';

  stringstream outss;
  for (size_t i = 0; i < m_dataWS->x(0).size(); ++i)
    outss << m_dataWS->x(0)[i] << "\t\t" << m_dataWS->y(0)[i] << "\t\t" << m_dataWS->e(0)[i] << '\n';
  g_log.debug() << "Input Peak Position Workspace To Fit: \n" << outss.str() << '\n';

  auto fitalg = createChildAlgorithm("Fit", 0.0, 0.2, true);
  fitalg->initialize();

  fitalg->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(m_Function));
  fitalg->setProperty("InputWorkspace", m_dataWS);
  fitalg->setProperty("WorkspaceIndex", 0);
  fitalg->setProperty("Minimizer", minimizer);
  fitalg->setProperty("CostFunction", "Least squares");
  fitalg->setProperty("MaxIterations", 1000);

  bool successfulfit = fitalg->execute();
  if (!fitalg->isExecuted() || !successfulfit) {
    // Early return due to bad fit
    g_log.error() << "Fitting to instrument geometry function failed. \n";
    throw std::runtime_error("Fitting failed.");
  }

  double chi2 = fitalg->getProperty("OutputChi2overDoF");
  std::string fitstatus = fitalg->getProperty("OutputStatus");

  g_log.debug() << "Fit Result (GSL):  Chi^2 = " << chi2 << "; Fit Status = " << fitstatus << '\n';

  API::IFunction_sptr fitfunc = fitalg->getProperty("Function");

  // 4. Set the output data (model and diff)
  m_Function->function(domain, values);

  for (size_t i = 0; i < domain.size(); ++i) {
    m_dataWS->mutableY(1)[i] = values[i];
    m_dataWS->mutableY(2)[i] = m_dataWS->y(0)[i] - values[i];
  }

  double selfchi2 = calculateD2TOFFunction(m_Function, domain, values, rawY, rawE);
  g_log.debug() << "Homemade Chi^2 = " << selfchi2 << '\n';

  // 5. Update fitted parameters
  for (const auto &parname : funparamnames) {
    double parvalue = fitfunc->getParameter(parname);
    m_FuncParameters[parname] = parvalue;
  }

  // 6. Pretty screen output
  stringstream dbss;
  dbss << "************ Fit Parameter Result *************\n";
  for (paramiter = m_FuncParameters.begin(); paramiter != m_FuncParameters.end(); ++paramiter) {
    std::string parname = paramiter->first;
    double inpparvalue = m_OrigParameters[parname];
    double parvalue = paramiter->second;
    dbss << setw(20) << parname << " = " << setw(15) << setprecision(6) << parvalue << "\t\tFrom " << setw(15)
         << setprecision(6) << inpparvalue << "\t\tDiff = " << inpparvalue - parvalue << '\n';
  }
  dbss << "*********************************************\n";
  g_log.debug() << dbss.str();

  // 7. Play with Zscore:     template<typename TYPE>
  //    std::vector<double> getZscore(const std::vector<TYPE>& data, const bool
  //    sorted=false);
  vector<double> z0 = Kernel::getZscore(m_dataWS->y(0).rawData());
  vector<double> z1 = Kernel::getZscore(m_dataWS->y(1).rawData());
  vector<double> z2 = Kernel::getZscore(m_dataWS->y(2).rawData());
  stringstream zss;
  zss << setw(20) << "d_h" << setw(20) << "Z DataY" << setw(20) << "Z ModelY" << setw(20) << "Z DiffY" << setw(20)
      << "DiffY\n";
  const auto &X = m_dataWS->x(0);
  const auto &Y = m_dataWS->y(2);
  for (size_t i = 0; i < z0.size(); ++i) {
    double d_h = X[i];
    double zdatay = z0[i];
    double zmodely = z1[i];
    double zdiffy = z2[i];
    double diffy = Y[i];
    zss << setw(20) << d_h << setw(20) << zdatay << setw(20) << zmodely << setw(20) << zdiffy << setw(20) << diffy
        << '\n';
  }
  g_log.debug() << "Zscore Survey: \b" << zss.str();
}

/** Fit function to data
 */
bool RefinePowderInstrumentParameters::fitFunction(const IFunction_sptr &func, double &gslchi2) {
  auto fitalg = createChildAlgorithm("Fit", 0.0, 0.2, true);
  fitalg->initialize();

  fitalg->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(func));
  fitalg->setProperty("InputWorkspace", m_dataWS);
  fitalg->setProperty("WorkspaceIndex", 0);
  fitalg->setProperty("Minimizer", "Simplex");
  fitalg->setProperty("CostFunction", "Least squares");
  fitalg->setProperty("MaxIterations", 1000);

  bool successfulfit = fitalg->execute();
  if (!fitalg->isExecuted() || !successfulfit) {
    // Early return due to bad fit
    g_log.error() << "Fitting to instrument geometry function failed. \n";
    throw std::runtime_error("Fitting failed.");
  }

  gslchi2 = fitalg->getProperty("OutputChi2overDoF");
  std::string fitstatus = fitalg->getProperty("OutputStatus");

  g_log.debug() << "Function Fit:  Chi^2 = " << gslchi2 << "; Fit Status = " << fitstatus << '\n';

  bool fitgood = (fitstatus == "success");

  return fitgood;
}

/** Calculate function's statistic
 */
double RefinePowderInstrumentParameters::calculateFunctionStatistic(const IFunction_sptr &func,
                                                                    const MatrixWorkspace_sptr &dataws,
                                                                    size_t workspaceindex) {
  // 1. Fix all parameters of the function
  vector<string> funcparameters = func->getParameterNames();
  size_t numparams = funcparameters.size();
  for (size_t i = 0; i < numparams; ++i) {
    func->fix(i);
  }

  // 2. Call a non fit refine
  auto fitalg = createChildAlgorithm("Fit", 0.0, 0.2, true);
  fitalg->initialize();

  fitalg->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(func));
  fitalg->setProperty("InputWorkspace", dataws);
  fitalg->setProperty("WorkspaceIndex", static_cast<int>(workspaceindex));
  fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
  fitalg->setProperty("CostFunction", "Least squares");
  fitalg->setProperty("MaxIterations", 2);

  bool successfulfit = fitalg->execute();
  if (!fitalg->isExecuted() || !successfulfit) {
    // Early return due to bad fit
    g_log.error() << "Fitting to instrument geometry function failed. \n";
    throw std::runtime_error("Fitting failed.");
  }

  double chi2 = fitalg->getProperty("OutputChi2overDoF");
  std::string fitstatus = fitalg->getProperty("OutputStatus");

  g_log.debug() << "Function calculation [L.M]:  Chi^2 = " << chi2 << "; Fit Status = " << fitstatus << '\n';

  return chi2;
}

/** Refine instrument parameters by Monte Carlo method
 */
void RefinePowderInstrumentParameters::refineInstrumentParametersMC(const TableWorkspace_sptr &parameterWS, bool fit2) {
  // 1. Get function's parameter names
  getD2TOFFuncParamNames(m_PeakFunctionParameterNames);

  // 2. Parse parameter (table) workspace
  vector<double> stepsizes, lowerbounds, upperbounds;
  importMonteCarloParametersFromTable(parameterWS, m_PeakFunctionParameterNames, stepsizes, lowerbounds, upperbounds);

  stringstream dbss;
  for (size_t i = 0; i < m_PeakFunctionParameterNames.size(); ++i) {
    dbss << setw(20) << m_PeakFunctionParameterNames[i] << ": Min = " << setw(15) << setprecision(6) << lowerbounds[i]
         << ", Max = " << setw(15) << setprecision(6) << upperbounds[i] << ", Step Size = " << setw(15)
         << setprecision(6) << stepsizes[i] << '\n';
  }
  g_log.notice() << "Monte Carlo Parameters: \n" << dbss.str();

  // 3. Maximum step size
  size_t maxsteps;
  int tempint = getProperty("RandomWalkSteps");
  if (tempint > 0)
    maxsteps = static_cast<size_t>(tempint);
  else
    throw runtime_error("RandomwWalkSteps cannot be less than or equal to 0. ");

  // 4. Random seed and step size rescale factor
  int randomseed = getProperty("MonteCarloRandomSeed");
  srand(randomseed);

  double stepsizescalefactor = 1.1;

  // 5. Monte Carlo simulation
  doParameterSpaceRandomWalk(m_PeakFunctionParameterNames, lowerbounds, upperbounds, stepsizes, maxsteps,
                             stepsizescalefactor, fit2);

  // 6. Record the result
  const auto &X = m_dataWS->x(0);
  const auto &Y = m_dataWS->y(0);
  const auto &E = m_dataWS->e(0);
  FunctionDomain1DVector domain(X.rawData());
  FunctionValues values(domain);
  for (size_t i = 0; i < m_BestFitParameters.size(); ++i) {
    // a. Set the function with the
    for (size_t j = 0; j < m_PeakFunctionParameterNames.size(); ++j) {
      m_Function->setParameter(m_PeakFunctionParameterNames[j], m_BestFitParameters[i].second[j]);
    }

    // b. Calculate
    calculateD2TOFFunction(m_Function, domain, values, Y, E);

    vector<double> vec_n;
    calculateThermalNeutronSpecial(m_Function, X, vec_n);

    // c. Put the data to output workspace
    auto &newY = m_dataWS->mutableY(3 * i + 1);
    auto &newD = m_dataWS->mutableY(3 * i + 2);
    auto &newN = m_dataWS->mutableY(3 * i + 3);
    for (size_t j = 0; j < newY.size(); ++j) {
      newY[j] = values[j];
      newD[j] = Y[j] - values[j];
      newN[j] = vec_n[j];
    }
  }
}

/** Core Monte Carlo random walk on parameter-space
 * Arguments
 * - fit2: boolean.  if True,then do Simplex fit for each step
 */
void RefinePowderInstrumentParameters::doParameterSpaceRandomWalk(vector<string> &parnames, vector<double> &lowerbounds,
                                                                  vector<double> &upperbounds,
                                                                  vector<double> &stepsizes, size_t maxsteps,
                                                                  double stepsizescalefactor, bool fit2) {

  // 1. Set up starting values, esp. to m_Function
  size_t numparameters = parnames.size();
  vector<double> paramvalues;
  for (size_t i = 0; i < numparameters; ++i) {
    string parname = parnames[i];
    double parvalue = m_FuncParameters[parname];
    paramvalues.emplace_back(parvalue);
    m_Function->setParameter(parname, parvalue);
  }

  // Calcualte the function's initial statistic
  m_BestGSLChi2 = calculateFunctionStatistic(m_Function, m_dataWS, 0);
  g_log.debug() << "Function with starting values has Chi2 = " << m_BestGSLChi2 << " (GSL L.M) \n";

  const auto &X = m_dataWS->x(0);
  const auto &rawY = m_dataWS->y(0);
  const auto &rawE = m_dataWS->e(0);
  FunctionDomain1DVector domain(X.rawData());
  FunctionValues values(domain);

  // 2. Determine the parameters to fit
  vector<string> paramstofit = getProperty("ParametersToFit");
  set<string> paramstofitset;
  bool refineallparams;
  if (paramstofit.empty()) {
    // Default case to refine all parameters
    refineallparams = true;
  } else {
    // Refine part of the parameters
    refineallparams = false;
    vector<string>::iterator vsiter;
    for (vsiter = paramstofit.begin(); vsiter != paramstofit.end(); ++vsiter) {
      paramstofitset.insert(*vsiter);
    }
  }

  stringstream dbss;
  set<string>::iterator setiter;
  for (setiter = paramstofitset.begin(); setiter != paramstofitset.end(); ++setiter) {
    string paramName = *setiter;
    dbss << setw(20) << paramName;
  }
  g_log.notice() << "Parameters to refine: " << dbss.str() << '\n';

  // 3. Create a local function for fit and set the parameters unfixed
  ThermalNeutronDtoTOFFunction_sptr func4fit =
      std::shared_ptr<ThermalNeutronDtoTOFFunction>(new ThermalNeutronDtoTOFFunction());
  func4fit->initialize();
  for (size_t i = 0; i < numparameters; ++i) {
    string parname = parnames[i];
    // Fit or fix
    if (paramstofitset.count(parname) > 0)
      func4fit->unfix(i);
    else
      func4fit->fix(i);
    // Constraint
    double lowerb = lowerbounds[i];
    double upperb = upperbounds[i];
    auto newconstraint = std::make_unique<BoundaryConstraint>(func4fit.get(), parname, lowerb, upperb);
    func4fit->addConstraint(std::move(newconstraint));
  }
  g_log.debug() << "Function for fitting in MC: " << func4fit->asString() << '\n';

  // 4. Do MC loops
  double curchi2 = calculateD2TOFFunction(m_Function, domain, values, rawY, rawE);

  g_log.notice() << "Monte Carlo Random Walk Starting Chi^2 = " << curchi2 << '\n';

  size_t paramindex = 0;
  size_t numacceptance = 0;
  for (size_t istep = 0; istep < maxsteps; ++istep) {
    // a. Determine whether to refine this parameter
    if (!refineallparams) {
      if (paramstofitset.count(parnames[paramindex]) == 0) {
        ++paramindex;
        if (paramindex >= parnames.size())
          paramindex = 0;
        continue;
      }
    }

    // b. Propose for a new value
    double randomnumber = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
    double newvalue = paramvalues[paramindex] + (randomnumber - 1.0) * stepsizes[paramindex];
    if (newvalue > upperbounds[paramindex]) {
      newvalue = lowerbounds[paramindex] + (newvalue - upperbounds[paramindex]);
    } else if (newvalue < lowerbounds[paramindex]) {
      newvalue = upperbounds[paramindex] - (lowerbounds[paramindex] - newvalue);
    }

    try {
      m_Function->setParameter(parnames[paramindex], newvalue);
    } catch (runtime_error &) {
      stringstream errss;
      errss << "New Value = " << newvalue << ", Random Number = " << randomnumber
            << "Step Size = " << stepsizes[paramindex] << ", Step size rescale factor = " << stepsizescalefactor;
      g_log.error(errss.str());
      throw;
    }

    // b. Calcualte the new
    double newchi2 = calculateD2TOFFunction(m_Function, domain, values, rawY, rawE);

    // Optionally fit
    if (fit2) {
      // i.   Copy the parameters
      for (size_t i = 0; i < numparameters; ++i) {
        double parvalue = m_Function->getParameter(i);
        func4fit->setParameter(i, parvalue);
      }

      // ii.  Fit function
      double gslchi2;
      bool fitgood = fitFunction(func4fit, gslchi2);

      if (fitgood) {
        // iii. Caculate
        double homchi2 = calculateD2TOFFunction(func4fit, domain, values, rawY, rawE);

        if (gslchi2 < m_BestGSLChi2)
          m_BestGSLChi2 = gslchi2;

        // iv.  Archive
        vector<double> newparvalues;
        newparvalues.reserve(numparameters);
        for (size_t i = 0; i < numparameters; ++i) {
          double parvalue = func4fit->getParameter(i);
          newparvalues.emplace_back(parvalue);
        }
        m_BestFitParameters.emplace_back(homchi2, newparvalues);
        m_BestFitChi2s.emplace_back(homchi2, gslchi2);

        // v.  Sort and keep in size
        sort(m_BestFitParameters.begin(), m_BestFitParameters.end());
        sort(m_BestFitChi2s.begin(), m_BestFitChi2s.end());
        if (m_BestFitParameters.size() > m_MaxNumberStoredParameters) {
          m_BestFitParameters.pop_back();
          m_BestFitChi2s.pop_back();
        }
      }
    }

    // c. Accept?
    bool accept;
    double prob = exp(-(newchi2 - curchi2) / curchi2);
    double randnumber = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
    accept = randnumber < prob;

    // d. Adjust step size
    if (false) {
      if (newchi2 < curchi2) {
        // Decrease step size if it approaches a minima
        stepsizes[paramindex] = stepsizes[paramindex] / stepsizescalefactor;
      } else {
        // Increase step size if it diverges
        double newstepsize = stepsizes[paramindex] * stepsizescalefactor;
        double maxstepsize = upperbounds[paramindex] - lowerbounds[paramindex];
        if (newstepsize >= maxstepsize) {
          newstepsize = maxstepsize;
        }
        stepsizes[paramindex] = newstepsize;
      }
    }

    // e. Record the solution
    if (accept) {
      // i.   Accept the new value
      paramvalues[paramindex] = newvalue;

      // ii.  Add the new values to vector
      m_BestMCParameters.emplace_back(newchi2, paramvalues);

      // iii. Sort and delete the last if necessary
      sort(m_BestMCParameters.begin(), m_BestMCParameters.end());
      if (m_BestMCParameters.size() > m_MaxNumberStoredParameters)
        m_BestMCParameters.pop_back();

      // iv.  Update chi2 and ...
      curchi2 = newchi2;
      ++numacceptance;
    }

    // z. Update the parameter index for next movement
    ++paramindex;
    if (paramindex >= numparameters)
      paramindex = 0;

  } // FOREACH MC Step

  // 3. Debug output
  stringstream mcresult;
  mcresult << "Monte Carlo Result for " << m_BestMCParameters.size() << " Best Results\n";
  mcresult << "Number of acceptance = " << numacceptance << ", out of " << maxsteps << " MC steps."
           << "Accept ratio = " << static_cast<double>(numacceptance) / static_cast<double>(maxsteps) << '\n';
  mcresult << "Best " << m_BestMCParameters.size() << " Monte Carlo (no fit) results: \n";
  for (size_t i = 0; i < m_BestMCParameters.size(); ++i) {
    mcresult << setw(3) << i << ":  Chi^2 = " << m_BestMCParameters[i].first << '\n';
  }
  mcresult << "Best " << m_BestMCParameters.size() << " fitting results.  Best Chi^2 =  " << m_BestGSLChi2 << '\n';
  for (size_t i = 0; i < m_BestFitParameters.size(); ++i) {
    mcresult << setw(3) << i << ":  Chi^2 = " << m_BestFitParameters[i].first
             << ", GSL Chi^2 = " << m_BestFitChi2s[i].second << '\n';
  }
  g_log.notice() << mcresult.str();
}

/** Get the names of the parameters of D-TOF conversion function
 */
void RefinePowderInstrumentParameters::getD2TOFFuncParamNames(vector<string> &parnames) {
  // 1. Clear output
  parnames.clear();

  // 2. Get the parameter names from function
  m_Function = std::make_shared<ThermalNeutronDtoTOFFunction>();
  std::vector<std::string> funparamnames = m_Function->getParameterNames();

  // 3. Copy
  parnames = funparamnames;
}

/** Calculate the function
 */
double RefinePowderInstrumentParameters::calculateD2TOFFunction(const API::IFunction_sptr &func,
                                                                const API::FunctionDomain1DVector &domain,
                                                                API::FunctionValues &values,
                                                                const Mantid::HistogramData::HistogramY &rawY,
                                                                const Mantid::HistogramData::HistogramE &rawE) {
  // 1. Check validity
  if (!func) {
    throw std::runtime_error("m_Function has not been initialized!");
  }

  if (domain.size() != values.size() || domain.size() != rawY.size() || rawY.size() != rawE.size()) {
    throw std::runtime_error("Input domain, values and raw data have different sizes.");
  }

  // 2. Calculate vlaues
  func->function(domain, values);

  // 3. Calculate the difference
  double chi2 = 0;
  for (size_t i = 0; i < domain.size(); ++i) {
    double temp = (values[i] - rawY[i]) / rawE[i];
    chi2 += temp * temp;
    // cout << "Peak " << i << ": Model = " << values[i] << ", Data = " <<
    // rawY[i] << ".  Standard Error = " << rawE[i] << '\n';
  }

  return chi2;
}

//------------------------------- Processing Inputs
//----------------------------------------
/** Genearte peaks from input workspace
 * m_Peaks are stored in a map.  (HKL) is the key
 */
void RefinePowderInstrumentParameters::genPeaksFromTable(const DataObjects::TableWorkspace_sptr &peakparamws) {
  // 1. Check and clear input and output
  if (!peakparamws) {
    g_log.error() << "Input tableworkspace for peak parameters is invalid!\n";
    throw std::invalid_argument("Invalid input table workspace for peak parameters");
  }

  m_Peaks.clear();

  // 2. Parse table workspace rows to generate peaks
  vector<string> colnames = peakparamws->getColumnNames();
  size_t numrows = peakparamws->rowCount();

  for (size_t ir = 0; ir < numrows; ++ir) {
    // a) Generate peak
    BackToBackExponential_sptr newpeakptr = std::make_shared<BackToBackExponential>();
    newpeakptr->initialize();

    // b) Parse parameters
    int h, k, l;
    double alpha, beta, tof_h, sigma, sigma2, chi2, height, dbtemp;
    string strtemp;
    sigma2 = -1;

    API::TableRow row = peakparamws->getRow(ir);
    for (const auto &colname : colnames) {
      if (colname == "H")
        row >> h;
      else if (colname == "K")
        row >> k;
      else if (colname == "L")
        row >> l;
      else if (colname == "Alpha")
        row >> alpha;
      else if (colname == "Beta")
        row >> beta;
      else if (colname == "Sigma2")
        row >> sigma2;
      else if (colname == "Sigma")
        row >> sigma;
      else if (colname == "Chi2")
        row >> chi2;
      else if (colname == "Height")
        row >> height;
      else if (colname == "TOF_h")
        row >> tof_h;
      else {
        try {
          row >> dbtemp;
        } catch (runtime_error &) {
          row >> strtemp;
        }
      }
    }

    if (sigma2 > 0)
      sigma = sqrt(sigma2);

    // c) Set peak parameters and etc.
    newpeakptr->setParameter("A", alpha);
    newpeakptr->setParameter("B", beta);
    newpeakptr->setParameter("S", sigma);
    newpeakptr->setParameter("X0", tof_h);
    newpeakptr->setParameter("I", height);

    std::vector<int> hkl;
    hkl.emplace_back(h);
    hkl.emplace_back(k);
    hkl.emplace_back(l);

    m_Peaks.emplace(hkl, newpeakptr);

    m_PeakErrors.emplace(hkl, chi2);

    g_log.information() << "[Generatem_Peaks] Peak " << ir << " HKL = [" << hkl[0] << ", " << hkl[1] << ", " << hkl[2]
                        << "], Input Center = " << setw(10) << setprecision(6) << newpeakptr->centre() << '\n';

  } // ENDFOR Each potential peak
}

/** Import TableWorkspace containing the instrument parameters for fitting
 * the diffrotometer geometry parameters
 */
void RefinePowderInstrumentParameters::importParametersFromTable(const DataObjects::TableWorkspace_sptr &parameterWS,
                                                                 std::map<std::string, double> &parameters) {
  // 1. Check column orders
  std::vector<std::string> colnames = parameterWS->getColumnNames();
  if (colnames.size() < 2) {
    g_log.error() << "Input parameter table workspace does not have enough "
                     "number of columns. "
                  << " Number of columns = " << colnames.size() << " < 3 as required. \n";
    throw std::runtime_error("Input parameter workspace is wrong. ");
  }

  if (colnames[0] != "Name" || colnames[1] != "Value") {
    g_log.error() << "Input parameter table workspace does not have the "
                     "columns in order.  "
                  << " It must be Name, Value, FitOrTie.\n";
    throw std::runtime_error("Input parameter workspace is wrong. ");
  }

  // 2. Import data to maps
  std::string parname;
  double value;

  size_t numrows = parameterWS->rowCount();

  for (size_t ir = 0; ir < numrows; ++ir) {
    try {
      API::TableRow trow = parameterWS->getRow(ir);
      trow >> parname >> value;
      parameters.emplace(parname, value);
    } catch (runtime_error &) {
      g_log.error() << "Import table workspace " << parameterWS->getName() << " error in line " << ir << ".  "
                    << " Requires [string, double] in the first 2 columns.\n";
      throw;
    }
  }
}

/** Import the Monte Carlo related parameters from table
 * Arguments
 */
void RefinePowderInstrumentParameters::importMonteCarloParametersFromTable(const TableWorkspace_sptr &tablews,
                                                                           const vector<string> &parameternames,
                                                                           vector<double> &stepsizes,
                                                                           vector<double> &lowerbounds,
                                                                           vector<double> &upperbounds) {
  // 1. Get column information
  vector<string> colnames = tablews->getColumnNames();
  size_t imax, imin, istep;
  imax = colnames.size() + 1;
  imin = imax;
  istep = imax;

  for (size_t i = 0; i < colnames.size(); ++i) {
    if (colnames[i] == "Max")
      imax = i;
    else if (colnames[i] == "Min")
      imin = i;
    else if (colnames[i] == "StepSize")
      istep = i;
  }

  if (imax > colnames.size() || imin > colnames.size() || istep > colnames.size()) {
    stringstream errss;
    errss << "Input parameter workspace misses information for Monte Carlo "
             "minimizer. "
          << "One or more of the following columns are missing (Max, Min, "
             "StepSize).";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  // 2. Parse input to a map
  stepsizes.clear();
  lowerbounds.clear();
  upperbounds.clear();

  map<string, vector<double>> mcparameters;
  size_t numrows = tablews->rowCount();
  for (size_t ir = 0; ir < numrows; ++ir) {
    TableRow row = tablews->getRow(ir);
    string parname;
    double tmax = 0, tmin = 0, tstepsize = 0;
    row >> parname;
    for (size_t ic = 1; ic < colnames.size(); ++ic) {
      double tmpdbl = std::numeric_limits<float>::quiet_NaN();
      try {
        row >> tmpdbl;
      } catch (runtime_error &) {
        g_log.error() << "Import MC parameter " << colnames[ic] << " error in row " << ir << " of workspace "
                      << tablews->getName() << '\n';
        string tmpstr;
        row >> tmpstr;
        g_log.error() << "Should be " << tmpstr << '\n';
      }

      if (ic == imax)
        tmax = tmpdbl;
      else if (ic == imin)
        tmin = tmpdbl;
      else if (ic == istep)
        tstepsize = tmpdbl;
    }
    vector<double> tmpvec;
    tmpvec.emplace_back(tmin);
    tmpvec.emplace_back(tmax);
    tmpvec.emplace_back(tstepsize);
    mcparameters.emplace(parname, tmpvec);
  }

  // 3. Retrieve the information for geometry parameters
  for (const auto &parname : parameternames) {
    // a) Get on hold of the MC parameter vector
    auto mit = mcparameters.find(parname);
    if (mit == mcparameters.end()) {
      // Not found the parameter.  raise error!
      stringstream errss;
      errss << "Input instrument parameter workspace does not have parameter " << parname
            << ".  Information is incomplete for Monte Carlo simulation.\n";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }
    vector<double> mcparvalues = mit->second;

    // b) Build for the output
    lowerbounds.emplace_back(mcparvalues[0]);
    upperbounds.emplace_back(mcparvalues[1]);
    stepsizes.emplace_back(mcparvalues[2]);
  }
}

/** Calculate thermal neutron's d-spacing
double RefinePowderInstrumentParameters::calculateDspaceValue(std::vector<int>
hkl, double lattice)
{
  // FIXME  It only works for the assumption that the lattice is cubical
  double h = static_cast<double>(hkl[0]);
  double k = static_cast<double>(hkl[1]);
  double l = static_cast<double>(hkl[2]);

  double d = lattice/sqrt(h*h+k*k+l*l);

  return d;
}
*/

/** Calculate value n for thermal neutron peak profile
 */
void RefinePowderInstrumentParameters::calculateThermalNeutronSpecial(const IFunction_sptr &m_Function,
                                                                      const HistogramX &xVals, vector<double> &vec_n) {
  if (m_Function->name() != "ThermalNeutronDtoTOFFunction") {
    g_log.warning() << "Function (" << m_Function->name()
                    << " is not ThermalNeutronDtoTOFFunction.  And it is not "
                       "required to calculate n.\n";
    for (size_t i = 0; i < xVals.size(); ++i)
      vec_n.emplace_back(0);
  }

  double width = m_Function->getParameter("Width");
  double tcross = m_Function->getParameter("Tcross");

  for (double dh : xVals) {
    double n = 0.5 * gsl_sf_erfc(width * (tcross - 1 / dh));
    vec_n.emplace_back(n);
  }
}

//------- Related to Algorith's Output
//-------------------------------------------------------------------

/** Get peak positions from peak functions
 * Arguments:
 * Output: outWS  1 spectrum .  dspacing - peak center
 */
void RefinePowderInstrumentParameters::genPeakCentersWorkspace(bool montecarlo, size_t numbestfit) {
  // 1. Collect values in a vector for sorting
  double lattice = m_FuncParameters["LatticeConstant"];
  if (lattice < 1.0E-5) {
    std::stringstream errmsg;
    errmsg << "Input Lattice constant = " << lattice << " is wrong or not set up right. ";
    throw std::invalid_argument(errmsg.str());
  }

  string stdoption = getProperty("StandardError");
  enum { ConstantValue, InvertedPeakHeight } stderroroption;
  if (stdoption == "ConstantValue") {
    stderroroption = ConstantValue;
  } else if (stdoption == "InvertedPeakHeight") {
    stderroroption = InvertedPeakHeight;
  } else {
    stringstream errss;
    errss << "Input StandardError (" << stdoption << ") is not supported. ";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  std::map<std::vector<int>, BackToBackExponential_sptr>::iterator peakiter;
  std::vector<std::pair<double, std::pair<double, double>>> peakcenters; // d_h [TOF_h, CHI2]

  Geometry::UnitCell unitcell(lattice, lattice, lattice, 90.0, 90.0, 90.0);

  for (peakiter = m_Peaks.begin(); peakiter != m_Peaks.end(); ++peakiter) {
    vector<int> hkl = peakiter->first;
    BackToBackExponential_sptr peak = peakiter->second;

    double sigma = peak->getParameter("S");
    if (sigma < m_MinSigma) {
      g_log.information() << "Peak (" << hkl[0] << ", " << hkl[1] << ", " << hkl[2]
                          << ") has unphysically small Sigma = " << sigma << ".  "
                          << "It is thus excluded. \n";
      continue;
    }

    /* Replaced by UnitCell
    double dh = calculateDspaceValue(hkl, lattice);
    */
    double dh = unitcell.d(hkl[0], hkl[1], hkl[2]);
    double center = peak->centre();
    double height = peak->height();
    double chi2;
    if (stderroroption == ConstantValue) {
      chi2 = 1.0;
    } else if (stderroroption == InvertedPeakHeight) {
      chi2 = sqrt(1.0 / height);
    } else {
      throw runtime_error("Standard error option is not supported. ");
    }

    peakcenters.emplace_back(dh, make_pair(center, chi2));
  }

  // 2. Sort by d-spacing value
  std::sort(peakcenters.begin(), peakcenters.end());

  // 3. Create output workspace
  size_t size = peakcenters.size();
  size_t nspec;
  if (montecarlo) {
    // Monte Carlo, 1 + 2*N spectra:  raw, N x (refined, diff, n)
    nspec = 1 + 3 * numbestfit;
  } else {
    // Regular fit, 3 spectra:  raw, refined, diff, n
    nspec = 1 + 3;
  }

  m_dataWS = std::dynamic_pointer_cast<DataObjects::Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", nspec, size, size));
  m_dataWS->getAxis(0)->setUnit("dSpacing");

  // 4. Put data to output workspace
  for (size_t i = 0; i < peakcenters.size(); ++i) {
    for (size_t j = 0; j < nspec; ++j) {
      m_dataWS->mutableX(j)[i] = peakcenters[i].first;
    }
    m_dataWS->mutableY(0)[i] = peakcenters[i].second.first;
    m_dataWS->mutableE(0)[i] = peakcenters[i].second.second;
  }
}

/** Generate a Monte Carlo result table containing the N best results
 * Column: chi2, parameter1, parameter2, ... ...
 */
DataObjects::TableWorkspace_sptr RefinePowderInstrumentParameters::genMCResultTable() {
  // 1. Create table workspace
  DataObjects::TableWorkspace_sptr tablews = std::make_shared<TableWorkspace>();

  tablews->addColumn("double", "Chi2");
  tablews->addColumn("double", "GSLChi2");
  for (auto &peakFunctionParameterName : m_PeakFunctionParameterNames) {
    tablews->addColumn("double", peakFunctionParameterName);
  }

  // 2. Put values in
  for (size_t ib = 0; ib < m_BestFitParameters.size(); ++ib) {
    TableRow newrow = tablews->appendRow();
    double chi2 = m_BestFitParameters[ib].first;
    double gslchi2 = m_BestFitChi2s[ib].second;
    newrow << chi2 << gslchi2;
    for (size_t ip = 0; ip < m_PeakFunctionParameterNames.size(); ++ip) {
      double tempdbl = m_BestFitParameters[ib].second[ip];
      newrow << tempdbl;
    }
  } // ENDFOR 1 Best Answer

  return tablews;
}

/** Generate an output table workspace containing the fitted instrument
 * parameters.
 * Requirement (1): The output table workspace should be usable by Le Bail
 * Fitting
 */
DataObjects::TableWorkspace_sptr RefinePowderInstrumentParameters::genOutputInstrumentParameterTable() {
  //  TableWorkspace is not copyable (default CC is incorrect and no point in
  //  writing a non-default one)
  DataObjects::TableWorkspace_sptr newtablews =
      std::shared_ptr<DataObjects::TableWorkspace>(new DataObjects::TableWorkspace());
  newtablews->addColumn("str", "Name");
  newtablews->addColumn("double", "Value");
  newtablews->addColumn("str", "FitOrTie");
  newtablews->addColumn("double", "Min");
  newtablews->addColumn("double", "Max");
  newtablews->addColumn("double", "StepSize");

  std::map<std::string, double>::iterator pariter;

  for (pariter = m_FuncParameters.begin(); pariter != m_FuncParameters.end(); ++pariter) {
    API::TableRow newrow = newtablews->appendRow();
    std::string parname = pariter->first;
    double parvalue = pariter->second;
    newrow << parname << parvalue;
  }

  return newtablews;
}

} // namespace Mantid::CurveFitting::Algorithms
