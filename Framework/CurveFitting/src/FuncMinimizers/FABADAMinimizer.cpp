#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidCurveFitting//Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/FuncMinimizers/FABADAMinimizer.h"

#include "MantidHistogramData/LinearGenerator.h"

#include "MantidKernel/Logger.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/version.hpp>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

namespace {
// static logger object
Kernel::Logger g_log("FABADAMinimizer");
// number of iterations when convergence isn't expected
const size_t lowerConvergenceLimit = 350;
// very large number
const double largeNumber = 1e100;
// jump checking rate
const size_t jumpCheckingRate = 200;
// low jump limit
const double lowJumpLimit = 1e-25;
}

DECLARE_FUNCMINIMIZER(FABADAMinimizer, FABADA)

/// Constructor
FABADAMinimizer::FABADAMinimizer()
    : m_counter(0), m_ChainIterations(0), m_changes(), m_jump(), m_parameters(),
      m_chain(), m_chi2(0.), m_converged(false), m_conv_point(0),
      m_par_converged(), m_lower(), m_upper(), m_bound(), m_criteria(),
      m_max_iter(0), m_par_changed(), m_Temperature(0.), m_counterGlobal(0),
      m_SimAnnealingItStep(0), m_LeftRefrPoints(0), m_TempStep(0.),
      m_Overexploration(false), m_nParams(0), m_InnactConvCriterion(0),
      m_NumInactiveRegenerations(), m_changesOld() {
  declareProperty("ChainLength", static_cast<size_t>(10000),
                  "Length of the converged chain.");
  declareProperty("StepsBetweenValues", 10,
                  "Steps done between chain points to avoid correlation"
                  " between them.");
  declareProperty(
      "ConvergenceCriteria", 0.01,
      "Variance in Cost Function for considering convergence reached.");
  declareProperty("InnactiveConvergenceCriterion", static_cast<size_t>(5),
                  "Number of Innactive Regenerations to consider"
                  " a certain parameter to be converged");
  declareProperty("JumpAcceptanceRate", 0.6666666,
                  "Desired jumping acceptance rate");
  // Simulated Annealing properties
  declareProperty("SimAnnealingApplied", false,
                  "If minimization should be run with Simulated"
                  " Annealing or not");
  declareProperty("MaximumTemperature", 10.0,
                  "Simulated Annealing maximum temperature");
  declareProperty("NumRefrigerationSteps", static_cast<size_t>(5),
                  "Simulated Annealing number of temperature changes");
  declareProperty("SimAnnealingIterations", static_cast<size_t>(10000),
                  "Number of iterations for the Simulated Annealig");
  declareProperty("Overexploration", false,
                  "If Temperatures < 1 are desired for overexploration,"
                  " no error will jump for that (The temperature is"
                  " constant during the convergence period)."
                  " Useful to find the exact minimum.");
  // Output Properties
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      "PDF", "PDF", Kernel::Direction::Output),
                  "The name to give the output workspace for the"
                  " Probability Density Functions");
  declareProperty("NumberBinsPDF", 20,
                  "Number of bins used for the output PDFs");
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      "Chains", "", Kernel::Direction::Output),
                  "The name to give the output workspace for the"
                  " complete chains.");
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      "ConvergedChain", "", Kernel::Direction::Output,
                      API::PropertyMode::Optional),
                  "The name to give the output workspace for just the"
                  "converged chain");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          "CostFunctionTable", "", Kernel::Direction::Output),
      "The name to give the output workspace");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          "Parameters", "", Kernel::Direction::Output),
      "The name to give the output workspace (Parameter values and errors)");

  // To be implemented in the future
  /*declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<>>(
          "Chi-squareLandscape", "", Kernel::Direction::Output),
      "The name to give the output workspace containing the chi-square"
      " landscape");*/
}

/// Initialize minimizer. Set initial values for all private members.
void FABADAMinimizer::initialize(API::ICostFunction_sptr function,
                                 size_t maxIterations) {

  m_leastSquares =
      boost::dynamic_pointer_cast<CostFunctions::CostFuncLeastSquares>(
          function);
  if (!m_leastSquares) {
    throw std::invalid_argument("FABADA works only with least squares."
                                " Different function was given.");
  }

  m_FitFunction = m_leastSquares->getFittingFunction();

  m_counter = 0;
  m_counterGlobal = 0;

  // The "real" parametersare got (not the active ones)
  m_nParams = m_FitFunction->nParams();
  // The initial parameters are saved
  if (m_parameters.size() != m_nParams) {
    m_parameters.resize(m_nParams);
  }
  for (size_t i = 0; i < m_nParams; ++i) {
    m_parameters.set(i, m_FitFunction->getParameter(i));
  }

  if (m_nParams == 0) {
    throw std::invalid_argument("Function has 0 fitting parameters.");
  }

  // Variable to calculate the total number of iterations required by the
  // SimulatedAnnealing and the posterior chain plus the burn in required
  // for the adaptation of the jump
  size_t TotalRequiredIterations = 350;

  size_t n = getProperty("ChainLength");
  m_ChainIterations = size_t(ceil(double(n) / double(m_nParams)));

  TotalRequiredIterations += m_ChainIterations;

  // Save parameter constraints
  for (size_t i = 0; i < m_nParams; ++i) {
    double p = m_parameters.get(i);
    m_bound.push_back(false);
    API::IConstraint *iconstr = m_FitFunction->getConstraint(i);
    if (iconstr) {
      Constraints::BoundaryConstraint *bcon =
          dynamic_cast<Constraints::BoundaryConstraint *>(iconstr);
      if (bcon) {
        m_bound[i] = true;
        if (bcon->hasLower()) {
          m_lower.push_back(bcon->lower());
        } else {
          m_lower.push_back(-largeNumber);
        }
        if (bcon->hasUpper()) {
          m_upper.push_back(bcon->upper());
        } else {
          m_upper.push_back(largeNumber);
        }
        if (p < m_lower[i]) {
          p = m_lower[i];
          m_parameters.set(i, p);
        }
        if (p > m_upper[i]) {
          p = m_upper[i];
          m_parameters.set(i, p);
        }
      }
    } else {
      m_lower.push_back(-largeNumber);
      m_upper.push_back(largeNumber);
    }

    // Initialize chains
    std::vector<double> v{p};
    m_chain.push_back(v);
    m_max_iter = maxIterations;

    // Initilize convergence and jump parameters
    m_changes.push_back(0);
    m_NumInactiveRegenerations.push_back(0);
    m_par_converged.push_back(false);
    m_criteria.push_back(getProperty("ConvergenceCriteria"));
    if (p != 0.0) {
      m_jump.push_back(std::abs(p / 10));
    } else {
      m_jump.push_back(0.01);
    }
    m_par_changed.push_back(false);
  }
  m_changesOld = m_changes;
  m_chi2 = m_leastSquares->val();
  std::vector<double> v{m_chi2};
  m_chain.push_back(v);
  m_converged = false;
  m_max_iter = maxIterations;
  m_InnactConvCriterion = getProperty("InnactiveConvergenceCriterion");

  // Simulated Annealing
  // Obs: Simulated Annealing with maximum temperature = 1.0, 1step,
  // could be used to increase the "burn-in" period before beginning to
  // check for convergence (Not the ideal way -> Think something)
  if (getProperty("SimAnnealingApplied")) {

    m_Temperature = getProperty("MaximumTemperature");
    if (m_Temperature == 0.0) {
      g_log.warning() << "MaximumTemperature not a valid temperature"
                         " (T = 0). Default (T = 10.0) taken.\n";
      m_Temperature = 10.0;
    }
    if (m_Temperature < 0) {
      g_log.warning() << "MaximumTemperature not a temperature"
                         " (< 0), absolute value taken\n";
      m_Temperature = -m_Temperature;
    }
    m_Overexploration = getProperty("Overexploration");
    if (!m_Overexploration && m_Temperature < 1) {
      m_Temperature = 1 / m_Temperature;
      g_log.warning() << "MaximumTemperature reduces proper"
                         " exploration (0 < T < 1), product inverse taken ("
                      << m_Temperature << ")\n";
    }
    if (m_Overexploration && m_Temperature > 1) {
      m_Overexploration = false;
      g_log.warning()
          << "Overexploration wrong temperature. Not"
             " overexploring. Applying usual Simulated Annealing.\n";
    }
    // Obs: The result is truncated to not have more iterations than
    // the chosen by the user and for all temperatures have the same
    // number of iterations
    m_LeftRefrPoints = getProperty("NumRefrigerationSteps");
    if (m_LeftRefrPoints <= 0) {
      g_log.warning()
          << "Wrong value for the number of refrigeration"
             " points (<= 0). Therefore, default value (5 points) taken.\n";
      m_LeftRefrPoints = 5;
    }

    m_SimAnnealingItStep = getProperty("SimAnnealingIterations");
    m_SimAnnealingItStep /= m_LeftRefrPoints;

    m_TempStep = pow(m_Temperature, 1.0 / double(m_LeftRefrPoints));

    // m_SimAnnealingItStep stores the number of iterations per step
    if (!m_Overexploration)
      TotalRequiredIterations += m_SimAnnealingItStep * m_LeftRefrPoints;

    // 50 for pseudo-continuous temperature decrease
    // without hindering the fitting algorithm itself
    if (m_SimAnnealingItStep < 50 && !m_Overexploration) {
      g_log.warning()
          << "SimAnnealingIterations/NumRefrigerationSteps too small"
             " (< 50 it). Simulated Annealing not applied\n";
      m_LeftRefrPoints = 0;
      m_Temperature = 1.0;
    }

    // During Overexploration, the temperature will not be changed
    if (m_Overexploration)
      m_LeftRefrPoints = 0;
  } else {
    m_Temperature = 1.0;
    m_LeftRefrPoints = 0;
  }

  // Throw error if there are not enough iterations
  if (TotalRequiredIterations >= maxIterations) {
    throw std::length_error(
        "Too few iterations to perform the"
        " Simulated Annealing and/or the posterior chain plus"
        " 350 iterations for the burn-in period. Increase"
        " MaxIterations property");
  }
}

/// Do one iteration. Returns true if iterations to be continued,
/// false if they must stop.
bool FABADAMinimizer::iterate(size_t) {

  if (!m_leastSquares) {
    throw std::runtime_error("Cost function isn't set up.");
  }

  size_t m = m_nParams;

  // Just for the last iteration. For doing exactly the indicated
  // number of iterations.
  if (m_converged && m_counter == m_ChainIterations - 1) {
    size_t t = getProperty("ChainLength");
    m = t % m_nParams;
    if (m == 0)
      m = m_nParams;
  }

  // Do one iteration of FABADA's algorithm for each parameter.
  for (size_t i = 0; i < m; i++) {

    GSLVector new_parameters = m_parameters;

    // Calculate the step from a Gaussian
    double step = GaussianStep(m_jump[i]);

    // Calculate the new value of the parameter
    double new_value = m_parameters.get(i) + step;

    // Checks if it is inside the boundary constrinctions.
    // If not, changes it.
    BoundApplication(i, new_value, step);
    // Obs: As well as checking whether the ties are not contradictory is
    // too constly, if there are tied parameters that are bounded,
    // checking that the boundedness is fulfilled for all the parameters
    // is costly too (extremely costly if the relations get complex,
    // which is plausible). Therefore it is not yet implemented and
    // the user should be aware of that.

    // Set the new value in order to calculate the new Chi square value
    if (std::isnan(new_value)) {
      throw std::runtime_error("Parameter value is NaN.");
    }
    new_parameters.set(i, new_value);

    // Update the new value through the IFunction
    m_FitFunction->setParameter(i, new_value);

    // First, it fulfills the other ties, finally the current parameter tie
    // It notices m_leastSquares (the CostFuncLeastSquares) that we have
    // modified the parameters
    TieApplication(i, new_parameters, new_value);

    // To track "unmovable" parameters (=> cannot converge)
    if (!m_par_changed[i] && new_parameters.get(i) != m_parameters.get(i))
      m_par_changed[i] = true;

    // Calculate the new chi2 value
    double chi2_new = m_leastSquares->val();
    // Save the old one to check convergence later on
    double chi2_old = m_chi2;

    // Given the new chi2, position, m_changes[ParameterIndex] and chains are
    // updated
    AlgorithmDisplacement(i, chi2_new, new_parameters);

    // Update the jump once each jumpCheckingRate iterations
    if (m_counter % jumpCheckingRate == 150) // JUMP CHECKING RATE IS 200, BUT
                                             // IS NOT CHECKED AT FIRST STEP, IT
                                             // IS AT 150
    {
      JumpUpdate(i);
    }

    // Check if the Chi square value has converged for parameter i.
    //(Obs: const int lowerConvergenceLimit = 350 := The iteration
    // since it starts to check if convergence is reached)

    // Take the unmovable parameters to be converged
    if (m_LeftRefrPoints == 0 && !m_par_changed[i] &&
        m_counter > lowerConvergenceLimit)
      m_par_converged[i] = true;

    if (m_LeftRefrPoints == 0 && !m_par_converged[i] &&
        m_counter > lowerConvergenceLimit) {
      if (chi2_old != m_chi2) {
        double chi2_quotient = fabs(m_chi2 - chi2_old) / chi2_old;
        if (chi2_quotient < m_criteria[i]) {
          m_par_converged[i] = true;
        }
      }
    }
  } // for i

  // Update the counter, after finishing the iteration for each parameter
  m_counter += 1;
  m_counterGlobal += 1;

  // Check if Chi square has converged for all the parameters
  // if overexploring or Simulated Annealing completed
  ConvergenceCheck(); // updates m_converged

  // Check wheather it is refrigeration time or not (for Simulated Annealing)
  if (m_LeftRefrPoints != 0 && m_counter == m_SimAnnealingItStep) {
    SimAnnealingRefrigeration();
  }

  // Evaluates if iterations should continue or not
  return IterationContinuation();

} // Iterate() end

double FABADAMinimizer::costFunctionVal() { return m_chi2; }

/// When all the iterations have been done, calculate and show all the
/// results.
void FABADAMinimizer::finalize() {

  // Creating the reduced chain (considering only one each
  // "Steps between values" values)
  size_t ChainLength = getProperty("ChainLength");
  int n_steps = getProperty("StepsBetweenValues");
  if (n_steps <= 0) {
    g_log.warning() << "StepsBetweenValues has a non valid value"
                       " (<= 0). Default one used"
                       " (StepsBetweenValues = 10).\n";
    n_steps = 10;
  }
  size_t conv_length = size_t(double(ChainLength) / double(n_steps));
  std::vector<std::vector<double>> red_conv_chain;

  // Declaring vectors for best values
  std::vector<double> BestParameters(m_nParams);
  std::vector<double> error_left(m_nParams);
  std::vector<double> error_rigth(m_nParams);

  // In case of reduced chain
  if (conv_length > 0) {
    // Write first element of the reduced chain
    for (size_t e = 0; e <= m_nParams; ++e) {
      std::vector<double> v{m_chain[e][m_conv_point]};
      red_conv_chain.push_back(v);
    }

    // Calculate the red_conv_chain for the cost fuction.
    auto first = m_chain[m_nParams].begin() + m_conv_point;
    auto last = m_chain[m_nParams].end();
    std::vector<double> conv_chain(first, last);
    for (size_t k = 1; k < conv_length; ++k) {
      red_conv_chain[m_nParams].push_back(conv_chain[n_steps * k]);
    }

    // Calculate the position of the minimum Chi square value
    auto position_min_chi2 = std::min_element(red_conv_chain[m_nParams].begin(),
                                              red_conv_chain[m_nParams].end());
    m_chi2 = *position_min_chi2;

    // Calculate the parameter value and the errors
    for (size_t j = 0; j < m_nParams; ++j) {
      auto first = m_chain[j].begin() + m_conv_point;
      auto last = m_chain[j].end();
      // red_conv_chain calculated for each parameter
      std::vector<double> conv_chain(first, last);
      auto &rc_chain_j = red_conv_chain[j];
      // Obs: Starts at 1 (0 already added)
      for (size_t k = 1; k < conv_length; ++k) {
        rc_chain_j.push_back(conv_chain[n_steps * k]);
      }
      // best fit parameters taken
      BestParameters[j] =
          rc_chain_j[position_min_chi2 - red_conv_chain[m_nParams].begin()];
      std::sort(rc_chain_j.begin(), rc_chain_j.end());
      auto pos_par =
          std::find(rc_chain_j.begin(), rc_chain_j.end(), BestParameters[j]);
      auto pos_left = rc_chain_j.begin();
      auto pos_right = rc_chain_j.end() - 1;
      // sigma characaterization for a Gaussian (0.34 comes from
      // percentage of area under the curve of a Gaussian from 0 to sigma)
      size_t sigma = static_cast<size_t>(0.34 * double(conv_length));

      // make sure the iterator is valid in any case
      if (sigma < static_cast<size_t>(std::distance(pos_left, pos_par))) {
        pos_left = pos_par - sigma;
      }
      if (sigma < static_cast<size_t>(std::distance(pos_par, pos_right))) {
        pos_right = pos_par + sigma;
      }
      error_left[j] = *pos_left - *pos_par;
      error_rigth[j] = *pos_right - *pos_par;
    }
  } // End if there is converged chain

  // If the converged chain is empty
  else {
    g_log.warning() << "There is no converged chain."
                       " Thus the parameters' errors are not"
                       " computed.\n";
    for (size_t k = 0; k < m_nParams; ++k) {
      BestParameters[k] = *(m_chain[k].end() - 1);
    }
  }

  const bool outputParametersTable = !getPropertyValue("Parameters").empty();

  if (outputParametersTable) {

    // Create the workspace for the parameters' value and errors.
    API::ITableWorkspace_sptr wsPdfE =
        API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    wsPdfE->addColumn("str", "Name");
    wsPdfE->addColumn("double", "Value");
    wsPdfE->addColumn("double", "Left's error");
    wsPdfE->addColumn("double", "Rigth's error");

    for (size_t j = 0; j < m_nParams; ++j) {
      API::TableRow row = wsPdfE->appendRow();
      row << m_FitFunction->parameterName(j) << BestParameters[j]
          << error_left[j] << error_rigth[j];
    }
    // Set and name the Parameter Errors workspace.
    setProperty("Parameters", wsPdfE);
  }

  // Set the best parameter values
  // Again, we need to modify the CostFunction through the IFunction

  for (size_t j = 0; j < m_nParams; ++j) {
    m_FitFunction->setParameter(j, BestParameters[j]);
  }
  // Convert type to setDirty the cost function
  boost::shared_ptr<MaleableCostFunction> leastSquaresMaleable =
      boost::static_pointer_cast<MaleableCostFunction>(m_leastSquares);
  leastSquaresMaleable->setDirtyInherited();
  // Convert back to base class
  m_leastSquares =
      boost::dynamic_pointer_cast<CostFunctions::CostFuncLeastSquares>(
          leastSquaresMaleable);

  // If required, output the complete chain
  const bool outputChains = !getPropertyValue("Chains").empty();

  if (outputChains) {

    // Create the workspace for the complete parameters' chain (the last
    // histogram is for the Chi square).
    size_t chain_length = m_chain[0].size();
    API::MatrixWorkspace_sptr wsC = API::WorkspaceFactory::Instance().create(
        "Workspace2D", m_nParams + 1, chain_length, chain_length);

    // Do one iteration for each parameter plus one for Chi square.
    for (size_t j = 0; j < m_nParams + 1; ++j) {
      wsC->setPoints(j, chain_length, HistogramData::LinearGenerator(0.0, 1.0));
      wsC->mutableY(j) = m_chain[j];
    }

    // Set and name the workspace for the complete chain
    setProperty("Chains", wsC);
  }

  // Create the workspace for the Probability Density Functions
  int pdf_length = getProperty(
      "NumberBinsPDF"); // histogram length for the PDF output workspace
  if (pdf_length <= 0) {
    g_log.warning() << "Non valid Number of bins for the PDF (<= 0)."
                       " Default value (20 bins) taken\n";
    pdf_length = 20;
  }
  API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
      "Workspace2D", m_nParams + 1, pdf_length + 1, pdf_length);

  // To store the most probable chi square value
  double mostPchi2;

  // Calculate the cost function Probability Density Function
  if (conv_length > 0) {
    std::sort(red_conv_chain[m_nParams].begin(),
              red_conv_chain[m_nParams].end());
    std::vector<double> pdf_y(pdf_length, 0);
    double start = red_conv_chain[m_nParams][0];
    double bin = (red_conv_chain[m_nParams][conv_length - 1] - start) /
                 double(pdf_length);
    size_t step = 0;
    auto &Y = ws->mutableY(m_nParams);
    ws->setBinEdges(m_nParams, pdf_length + 1,
                    HistogramData::LinearGenerator(start, bin));
    const auto &X = ws->x(m_nParams);
    for (size_t i = 1; i < static_cast<size_t>(pdf_length) + 1; i++) {
      const double bin_end = X[i];
      while (step < conv_length && red_conv_chain[m_nParams][step] <= bin_end) {
        pdf_y[i - 1] += 1;
        ++step;
      }
      // Divided by conv_length * bin to normalize
      Y[i - 1] = pdf_y[i - 1] / (double(conv_length) * bin);
    }

    auto pos_MPchi2 = std::max_element(pdf_y.begin(), pdf_y.end());

    mostPchi2 = X[pos_MPchi2 - pdf_y.begin()] + (bin / 2.0);

    // Do one iteration for each parameter.
    for (size_t j = 0; j < m_nParams; ++j) {
      // Calculate the Probability Density Function
      std::vector<double> pdf_y(pdf_length, 0);
      double start = red_conv_chain[j][0];
      double bin =
          (red_conv_chain[j][conv_length - 1] - start) / double(pdf_length);
      size_t step = 0;
      auto &Y = ws->mutableY(j);
      ws->setBinEdges(j, pdf_length + 1,
                      HistogramData::LinearGenerator(start, bin));
      const auto &X = ws->x(j);
      for (size_t i = 1; i < static_cast<size_t>(pdf_length) + 1; i++) {
        double bin_end = X[i];
        while (step < conv_length && red_conv_chain[j][step] <= bin_end) {
          pdf_y[i - 1] += 1;
          ++step;
        }
        Y[i - 1] = pdf_y[i - 1] / (double(conv_length) * bin);
      }

      // Calculate the most probable value, from the PDF.
      //*Not used (by the moment)
      //*auto pos_MP = std::max_element(pdf_y.begin(), pdf_y.end());
      //*double mostP = X[pos_MP - pdf_y.begin()] + (bin / 2.0);
      //*m_leastSquares->setParameter(j, mostP);
    }
  } // if conv_length > 0
  else {
    g_log.warning() << "No points to create PDF. Empty Wokspace returned.\n";
    mostPchi2 = -1;
  }

  // Set and name the PDF workspace.
  setProperty("PDF", ws);

  // Read if necessary to show the workspace for the converged part of the
  // chain.
  const bool outputConvergedChains =
      !getPropertyValue("ConvergedChain").empty();

  // OK eventhough conv_length = 0
  if (outputConvergedChains) {
    // Create the workspace for the converged part of the chain.
    API::MatrixWorkspace_sptr wsConv;
    if (conv_length > 0) {
      wsConv = API::WorkspaceFactory::Instance().create(
          "Workspace2D", m_nParams + 1, conv_length, conv_length);
    } else {
      g_log.warning() << "Empty converged chain, empty Workspace returned.";
      wsConv = API::WorkspaceFactory::Instance().create("Workspace2D",
                                                        m_nParams + 1, 1, 1);
    }

    // Do one iteration for each parameter plus one for Chi square.
    for (size_t j = 0; j < m_nParams + 1; ++j) {
      std::vector<double>::const_iterator first =
          m_chain[j].begin() + m_conv_point;
      std::vector<double>::const_iterator last = m_chain[j].end();
      std::vector<double> conv_chain(first, last);
      auto &X = wsConv->mutableX(j);
      auto &Y = wsConv->mutableY(j);
      for (size_t k = 0; k < conv_length; ++k) {
        X[k] = double(k);
        Y[k] = conv_chain[n_steps * k];
      }
    }

    // Set and name the workspace for the converged part of the chain.
    setProperty("ConvergedChain", wsConv);
  }

  // Read if necessary to show the workspace for the Chi square values.
  const bool outputCostFunctionTable =
      !getPropertyValue("CostFunctionTable").empty();

  if (outputCostFunctionTable) {

    // Create the workspace for the Chi square values.
    API::ITableWorkspace_sptr wsChi2 =
        API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    wsChi2->addColumn("double", "Chi2min");
    wsChi2->addColumn("double", "Chi2MP");
    wsChi2->addColumn("double", "Chi2min_red");
    wsChi2->addColumn("double", "Chi2MP_red");

    // Obtain the quantity of the initial data.
    API::FunctionDomain_sptr domain = m_leastSquares->getDomain();
    size_t data_number = domain->size();

    // Calculate the value for the reduced Chi square.
    double Chi2min_red =
        m_chi2 / (double(data_number - m_nParams)); // For de minimum value.
    double mostPchi2_red;
    if (conv_length > 0)
      mostPchi2_red = mostPchi2 / (double(data_number - m_nParams));
    else {
      g_log.warning()
          << "Most probable chi square not calculated. -1 is returned.\n"
          << "Most probable reduced chi square not calculated. -1 is "
             "returned.\n";
      mostPchi2_red = -1;
    }

    // Add the information to the workspace and name it.
    API::TableRow row = wsChi2->appendRow();
    row << m_chi2 << mostPchi2 << Chi2min_red << mostPchi2_red;
    setProperty("CostFunctionTable", wsChi2);
  }

  // Set the best parameter values
  // Not used (needed if we want to return the most probable values too,
  // so saved as commented out)
  /*for (size_t j = 0; j < m_nParams; ++j) {
    m_leastSquares->setParameter(j, BestParameters[j]);
  }*/
}

// Returns the step from a Gaussian given sigma = Jump
double FABADAMinimizer::GaussianStep(const double &Jump) {
  boost::mt19937 mt;
  mt.seed(123 * (int(m_counter) + 45 * int(Jump)) +
          14 * int(time_t())); // Numbers for the seed
  boost::normal_distribution<double> distr(0.0, std::abs(Jump));
  boost::variate_generator<boost::mt19937, boost::normal_distribution<double>>
      step(mt, distr);
  return step();
}

// If the new point is out of its bounds, it is changed to fit in the bound
// limits
void FABADAMinimizer::BoundApplication(const size_t &ParameterIndex,
                                       double &new_value, double &step) {
  // Checks if it is inside the boundary constrinctions.
  // If not, changes it.
  const size_t &i = ParameterIndex;
  if (m_bound[i]) {
    while (new_value < m_lower[i]) {
      if (std::abs(step) > m_upper[i] - m_lower[i]) {
        new_value = m_parameters.get(i) + step / 10.0;
        step = step / 10;
        m_jump[i] = m_jump[i] / 10;
      } else {
        new_value =
            m_lower[i] + std::abs(step) - (m_parameters.get(i) - m_lower[i]);
      }
    }
    while (new_value > m_upper[i]) {
      if (std::abs(step) > m_upper[i] - m_lower[i]) {
        new_value = m_parameters.get(i) + step / 10.0;
        step = step / 10;
        m_jump[i] = m_jump[i] / 10;
      } else {
        new_value =
            m_upper[i] - (std::abs(step) + m_parameters.get(i) - m_upper[i]);
      }
    }
  }
}

void FABADAMinimizer::TieApplication(const size_t &ParameterIndex,
                                     GSLVector &new_parameters,
                                     double &new_value) {
  const size_t &i = ParameterIndex;
  // Fulfill the ties of the other parameters
  for (size_t j = 0; j < m_nParams; ++j) {
    if (j != i) {
      API::ParameterTie *tie = m_FitFunction->getTie(j);
      if (tie) {
        new_value = tie->eval();
        if (std::isnan(new_value)) { // maybe not needed
          throw std::runtime_error("Parameter value is NaN.");
        }
        new_parameters.set(j, new_value);
        m_FitFunction->setParameter(j, new_value);
      }
    }
  }
  // After all the other variables, the current one is updated to the ties
  API::ParameterTie *tie = m_FitFunction->getTie(i);
  if (tie) {
    new_value = tie->eval();
    if (std::isnan(new_value)) { // maybe not needed
      throw std::runtime_error("Parameter value is NaN.");
    }
    new_parameters.set(i, new_value);
    m_FitFunction->setParameter(i, new_value);
  }
  //*ALTERNATIVE CODE
  //*To avoid creating the new class (way too slow)
  //*We could also setValue a certain parameter setting its own value
  //*on m_leastSquares (thus no real change)
  /*try{
          m_leastSquares->drop();
  }

  catch (...) {
          m_leastSquares->push();
          m_leastSquares->drop();
  }*/

  // Convert type to setDirty the cost function
  //(to notify the CostFunction we have modified the IFunction)
  boost::shared_ptr<MaleableCostFunction> leastSquaresMaleable =
      boost::static_pointer_cast<MaleableCostFunction>(m_leastSquares);

  leastSquaresMaleable->setDirtyInherited();

  // Convert back to base class
  m_leastSquares =
      boost::dynamic_pointer_cast<CostFunctions::CostFuncLeastSquares>(
          leastSquaresMaleable);
}

void FABADAMinimizer::AlgorithmDisplacement(const size_t &ParameterIndex,
                                            const double &chi2_new,
                                            GSLVector &new_parameters) {

  const size_t &i = ParameterIndex;

  // If new Chi square value is lower, jumping directly to new parameter
  if (chi2_new < m_chi2) {
    for (size_t j = 0; j < m_nParams; j++) {
      m_chain[j].push_back(new_parameters.get(j));
    }
    m_chain[m_nParams].push_back(chi2_new);
    m_parameters = new_parameters;
    m_chi2 = chi2_new;
    m_changes[i] += 1;
  }

  // If new Chi square value is higher, it depends on the probability
  else {
    // Calculate probability of change
    double prob = exp((m_chi2 - chi2_new) / (2.0 * m_Temperature));

    // Decide if changing or not
    boost::mt19937 mt;
    mt.seed(int(time_t()) + 48 * (int(m_counter) + 76 * int(i)));
    boost::uniform_real<> distr(0.0, 1.0);
    double p = distr(mt);
    if (p <= prob) {
      for (size_t j = 0; j < m_nParams; j++) {
        m_chain[j].push_back(new_parameters.get(j));
      }
      m_chain[m_nParams].push_back(chi2_new);
      m_parameters = new_parameters;
      m_chi2 = chi2_new;
      m_changes[i] += 1;
    } else {
      for (size_t j = 0; j < m_nParams; j++) {
        m_chain[j].push_back(m_parameters.get(j));
      }
      m_chain[m_nParams].push_back(m_chi2);
      // Old parameters taken again
      for (size_t j = 0; j < m_nParams; ++j) {
        m_FitFunction->setParameter(j, m_parameters.get(j));
      }
      // Convert type to setDirty the cost function
      //(to notify the CostFunction we have modified the FittingFunction)
      boost::shared_ptr<MaleableCostFunction> leastSquaresMaleable =
          boost::static_pointer_cast<MaleableCostFunction>(m_leastSquares);

      leastSquaresMaleable->setDirtyInherited();

      // Convert back to base class
      m_leastSquares =
          boost::dynamic_pointer_cast<CostFunctions::CostFuncLeastSquares>(
              leastSquaresMaleable);
    }
  }
}

void FABADAMinimizer::JumpUpdate(const size_t &ParameterIndex) {
  const size_t &i = ParameterIndex;
  const double jumpAR = getProperty("JumpAcceptanceRate");
  double jnew;

  if (m_LeftRefrPoints == 0 && m_changes[i] == m_changesOld[i])
    ++m_NumInactiveRegenerations[i];
  else
    m_changesOld[i] = m_changes[i];

  if (m_changes[i] == 0.0) {
    jnew = m_jump[i] / jumpCheckingRate;
    // JUST FOR THE CASE THERE HAS NOT BEEN ANY CHANGE
    //(treated as if only one acceptance).
  } else {
    m_NumInactiveRegenerations[i] = 0;
    double f = m_changes[i] / double(m_counter);

    //*ALTERNATIVE CODE
    //*Current acceptance rate evaluated
    //*double f = m_changes[i] / double(jumpCheckingRate);
    //*Obs: should be quicker to explore, but less stable (maybe not ergodic)

    jnew = m_jump[i] * f / jumpAR;

    //*ALTERNATIVE CODE
    //*Reset the m_changes value to get the information
    //*for the current jump, not the whole history (maybe not ergodic)
    //*m_changes[i] = 0;
  }

  m_jump[i] = jnew;

  // Check if the new jump is too small. It means that it has been a wrong
  // convergence.
  if (std::abs(m_jump[i]) < lowJumpLimit) {
    g_log.warning()
        << "Wrong convergence might be reached for parameter " +
               m_FitFunction->parameterName(i) +
               ". Try to set a proper initial value for this parameter\n";
  }
}

// Check if Chi square has converged for all the parameters
// if overexploring or Simulated Annealing completed
void FABADAMinimizer::ConvergenceCheck() {
  if (m_LeftRefrPoints == 0 && m_counter > lowerConvergenceLimit &&
      !m_converged) {
    size_t t = 0;
    bool ImmobilityConv = false;
    for (size_t i = 0; i < m_nParams; i++) {
      if (m_par_converged[i]) {
        t += 1;
      } else if (m_NumInactiveRegenerations[i] >= m_InnactConvCriterion) {
        ++t;
        ImmobilityConv = true;
      }
    }
    // If all parameters have converged (usually or through observed
    // immobility):
    // It sets up both the counter and the changes' vector to 0, in order to
    // consider only the data of the converged part of the chain, when updating
    // the jump.
    if (t == m_nParams) {
      m_converged = true;

      if (ImmobilityConv)
        g_log.warning() << "Convergence detected through immobility."
                           " It might be a bad convergence.\n";

      m_conv_point = m_counterGlobal * m_nParams + 1;
      m_counter = 0;
      for (size_t i = 0; i < m_nParams; ++i) {
        m_changes[i] = 0;
      }

      // If done with a different temperature, the error would be
      // wrongly obtained (because the temperature modifies the
      // chi-square landscape)
      // Although keeping ergodicity, more iterations will be needed
      // because a wrong step is initially used.
      m_Temperature = 1.0;
    }

    // All parameters should converge at the same iteration
    else {
      // The not converged parameters can be identified at the last iteration
      if (m_counterGlobal < m_max_iter - m_ChainIterations)
        for (size_t i = 0; i < m_nParams; ++i)
          m_par_converged[i] = false;
    }
  }
}

void FABADAMinimizer::SimAnnealingRefrigeration() {
  // Update jump to separate different temperatures
  for (size_t i = 0; i < m_nParams; ++i)
    JumpUpdate(i);

  // Resetting variables for next temperature
  //(independent jump calculation for different temperatures)
  m_counter = 0;
  for (size_t j = 0; j < m_nParams; ++j) {
    m_changes[j] = 0;
  }
  // Simulated Annealing variables updated
  --m_LeftRefrPoints;
  // To avoid numerical error accumulation
  if (m_LeftRefrPoints == 0)
    m_Temperature = 1.0;
  else
    m_Temperature /= m_TempStep;
}

bool FABADAMinimizer::IterationContinuation() {

  // If still through Simulated Annealing
  if (m_LeftRefrPoints != 0)
    return true;

  if (!m_converged) {

    // If there is not convergence continue the iterations.
    if (m_counterGlobal < m_max_iter - m_ChainIterations) {
      return true;
    }
    // If there is not convergence, but it has been made
    // convergenceMaxIterations iterations, stop and throw the error.
    else {
      std::string failed = "";
      for (size_t i = 0; i < m_nParams; ++i) {
        if (!m_par_converged[i]) {
          failed.append(m_FitFunction->parameterName(i)).append(", ");
        }
      }
      failed.replace(failed.end() - 2, failed.end(), ".");
      throw std::runtime_error(
          "Convegence NOT reached after " +
          std::to_string(m_max_iter - m_ChainIterations) +
          " iterations.\n   Try to set better initial values for parameters: " +
          failed + " Or increase the maximum number of iterations "
                   "(MaxIterations property).");
    }
  } else {
    // If convergence has been reached, continue until we complete the chain
    // length. Otherwise, stop interations.
    return m_counter < m_ChainIterations;
  }
  // can we even get here? -> Nope (we should not, so we do not want it to
  // continue)
  return false;
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
