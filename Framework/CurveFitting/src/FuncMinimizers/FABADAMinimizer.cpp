// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/FuncMinimizers/FABADAMinimizer.h"

#include "MantidHistogramData/LinearGenerator.h"

#include "MantidKernel/Logger.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/normal_distribution.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <random>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {
using namespace Mantid::API;

namespace {

std::string const PDF_GROUP_NAME = "__PDF_Workspace";

// static logger object
Kernel::Logger g_log("FABADAMinimizer");
// number of iterations when convergence isn't expected
const size_t LOWER_CONVERGENCE_LIMIT = 350;
// jump checking rate
const size_t JUMP_CHECKING_RATE = 200;
// low jump limit
const double LOW_JUMP_LIMIT = 1e-25;
// random number generator
std::mt19937 rng;

API::MatrixWorkspace_sptr
createWorkspace(std::vector<double> const &xValues,
                std::vector<double> const &yValues, int const numberOfSpectra,
                std::vector<std::string> const &verticalAxisNames) {
  auto createWorkspaceAlgorithm =
      AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
  createWorkspaceAlgorithm->initialize();
  createWorkspaceAlgorithm->setChild(true);
  createWorkspaceAlgorithm->setLogging(false);
  createWorkspaceAlgorithm->setProperty("DataX", xValues);
  createWorkspaceAlgorithm->setProperty("DataY", yValues);
  createWorkspaceAlgorithm->setProperty("NSpec", numberOfSpectra);
  createWorkspaceAlgorithm->setProperty("VerticalAxisUnit", "Text");
  createWorkspaceAlgorithm->setProperty("VerticalAxisValues",
                                        verticalAxisNames);
  createWorkspaceAlgorithm->setProperty("OutputWorkspace", "__PDF");
  createWorkspaceAlgorithm->execute();
  return createWorkspaceAlgorithm->getProperty("OutputWorkspace");
}

} // namespace

DECLARE_FUNCMINIMIZER(FABADAMinimizer, FABADA)

/// Constructor
FABADAMinimizer::FABADAMinimizer()
    : m_counter(0), m_chainIterations(0), m_changes(), m_jump(), m_parameters(),
      m_chain(), m_chi2(0.), m_converged(false), m_convPoint(0),
      m_parConverged(), m_criteria(), m_maxIter(0), m_parChanged(),
      m_temperature(0.), m_counterGlobal(0), m_simAnnealingItStep(0),
      m_leftRefrPoints(0), m_tempStep(0.), m_overexploration(false),
      m_nParams(0), m_numInactiveRegenerations(), m_changesOld() {
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
  declareProperty("PDF", true, "If the PDF's should be calculated or not.");
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

/** Initialize minimizer. Set initial values for all private members
 *
 * @param function :: the fit function
 * @param maxIterations :: maximum number of iterations
 */
void FABADAMinimizer::initialize(API::ICostFunction_sptr function,
                                 size_t maxIterations) {

  m_leastSquares =
      boost::dynamic_pointer_cast<CostFunctions::CostFuncLeastSquares>(
          function);
  if (!m_leastSquares) {
    throw std::invalid_argument("FABADA works only with least squares."
                                " Different function was given.");
  }

  m_fitFunction = m_leastSquares->getFittingFunction();
  m_counter = 0;
  m_counterGlobal = 0;
  m_converged = false;
  m_maxIter = maxIterations;

  // Initialize member variables related to fitting parameters, such as
  // m_chains, m_jump, etc
  initChainsAndParameters();

  // Initialize member variables related to simulated annealing, such as
  // m_temperature, m_overexploration, etc
  initSimulatedAnnealing();

  // Variable to calculate the total number of iterations required by the
  // SimulatedAnnealing and the posterior chain plus the burn in required
  // for the adaptation of the jump
  size_t totalRequiredIterations = 350 + m_chainIterations;
  if (!m_overexploration)
    totalRequiredIterations += m_simAnnealingItStep * m_leftRefrPoints;

  // Throw error if there are not enough iterations
  if (totalRequiredIterations >= maxIterations) {
    throw std::length_error(
        "Too few iterations to perform the"
        " Simulated Annealing and/or the posterior chain plus"
        " 350 iterations for the burn-in period. Increase"
        " MaxIterations property");
  }
}

/** Do one iteration.
 *
 * @return :: true if iterations must be continued, false otherwise
 */
bool FABADAMinimizer::iterate(size_t /*iteration*/) {

  if (!m_leastSquares) {
    throw std::runtime_error("Cost function isn't set up.");
  }

  size_t m = m_nParams;

  // Just for the last iteration. For doing exactly the indicated
  // number of iterations.
  if (m_converged && m_counter == m_chainIterations - 1) {
    size_t t = getProperty("ChainLength");
    m = t % m_nParams;
    if (m == 0)
      m = m_nParams;
  }

  // Do one iteration of FABADA's algorithm for each parameter.
  for (size_t i = 0; i < m; i++) {

    GSLVector newParameters = m_parameters;

    if (!m_fitFunction->isFixed(i)) {
      // Calculate the step from a Gaussian
      double step = gaussianStep(m_jump[i]);

      // Calculate the new value of the parameter
      double newValue = m_parameters.get(i) + step;

      // Checks if it is inside the boundary constrinctions.
      // If not, changes it.
      boundApplication(i, newValue, step);
      // Obs: As well as checking whether the ties are not contradictory is
      // too constly, if there are tied parameters that are bounded,
      // checking that the boundedness is fulfilled for all the parameters
      // is costly too (extremely costly if the relations get complex,
      // which is plausible). Therefore it is not yet implemented and
      // the user should be aware of that.

      // Set the new value in order to calculate the new Chi square value
      if (std::isnan(newValue))
        throw std::runtime_error("Parameter value is NaN.");
      newParameters.set(i, newValue);

      // Update the new value through the IFunction
      m_fitFunction->setParameter(i, newValue);

      // First, it fulfills the other ties, finally the current parameter tie
      // It notices m_leastSquares (the CostFuncLeastSquares) that we have
      // modified the parameters
      tieApplication(i, newParameters, newValue);
      m_fitFunction->applyTies();
    }

    // To track "unmovable" parameters (=> cannot converge)
    if (!m_parChanged[i] && newParameters.get(i) != m_parameters.get(i))
      m_parChanged[i] = true;

    // Calculate the new chi2 value
    double newChi2 = m_leastSquares->val();
    // Save the old one to check convergence later on
    double oldChi2 = m_chi2;

    // Given the new chi2, position, m_changes[parameterIndex] and chains are
    // updated
    algorithmDisplacement(i, newChi2, newParameters);

    // Update the jump once each JUMP_CHECKING_RATE iterations
    if (m_counter % JUMP_CHECKING_RATE == 150) // JUMP CHECKING RATE IS 200, BUT
    // IS NOT CHECKED AT FIRST STEP, IT
    // IS AT 150
    {
      jumpUpdate(i);
    }

    // Check if the Chi square value has converged for parameter i.
    //(Obs: const int LOWER_CONVERGENCE_LIMIT = 350 := The iteration
    // since it starts to check if convergence is reached)

    // Take the unmovable parameters to be converged
    if (m_leftRefrPoints == 0 && !m_parChanged[i] &&
        m_counter > LOWER_CONVERGENCE_LIMIT)
      m_parConverged[i] = true;

    if (m_leftRefrPoints == 0 && !m_parConverged[i] &&
        m_counter > LOWER_CONVERGENCE_LIMIT) {
      if (oldChi2 != m_chi2) {
        double chi2Quotient = fabs(m_chi2 - oldChi2) / oldChi2;
        if (chi2Quotient < m_criteria[i]) {
          m_parConverged[i] = true;
        }
      }
    }
  } // for i

  // Update the counter, after finishing the iteration for each parameter
  m_counter += 1;
  m_counterGlobal += 1;

  // Check if Chi square has converged for all the parameters
  // if overexploring or Simulated Annealing completed
  convergenceCheck(); // updates m_converged

  // Check wheather it is refrigeration time or not (for Simulated Annealing)
  if (m_leftRefrPoints != 0 && m_counter == m_simAnnealingItStep) {
    simAnnealingRefrigeration();
  }

  // Evaluates if iterations should continue or not
  return iterationContinuation();

} // Iterate() end

double FABADAMinimizer::costFunctionVal() { return m_chi2; }

/** When all the iterations have been done, calculate and show all the results.
 *
 */
void FABADAMinimizer::finalize() {

  // Creating the reduced chain (considering only one each
  // "Steps between values" values)
  size_t chainLength = getProperty("ChainLength");
  int nSteps = getProperty("StepsBetweenValues");
  if (nSteps <= 0) {
    g_log.warning() << "StepsBetweenValues has a non valid value"
                       " (<= 0). Default one used"
                       " (StepsBetweenValues = 10).\n";
    nSteps = 10;
  }
  size_t convLength = size_t(double(chainLength) / double(nSteps));

  // Reduced chain
  std::vector<std::vector<double>> reducedConvergedChain;
  // Declaring vectors for best values
  std::vector<double> bestParameters(m_nParams);
  std::vector<double> errorLeft(m_nParams);
  std::vector<double> errorRight(m_nParams);

  calculateConvChainAndBestParameters(convLength, nSteps, reducedConvergedChain,
                                      bestParameters, errorLeft, errorRight);

  if (!getPropertyValue("Parameters").empty()) {
    outputParameterTable(bestParameters, errorLeft, errorRight);
  }

  // Set the best parameter values
  // Again, we need to modify the CostFunction through the IFunction

  for (size_t j = 0; j < m_nParams; ++j) {
    m_fitFunction->setParameter(j, bestParameters[j]);
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
  if (!getPropertyValue("Chains").empty()) {
    outputChains();
  }

  double mostPchi2 = outputPDF(convLength, reducedConvergedChain);

  if (!getPropertyValue("ConvergedChain").empty()) {
    outputConvergedChains(convLength, nSteps);
  }

  if (!getPropertyValue("CostFunctionTable").empty()) {
    outputCostFunctionTable(convLength, mostPchi2);
  }

  // Set the best parameter values
  // Not used (needed if we want to return the most probable values too,
  // so saved as commented out)
  /*for (size_t j = 0; j < m_nParams; ++j) {
    m_leastSquares->setParameter(j, BestParameters[j]);
  }*/
}

/** Returns the step from a Gaussian given sigma = jump
 *
 * @param jump :: sigma
 * @return :: the step
 */
double FABADAMinimizer::gaussianStep(const double &jump) {
  return Kernel::normal_distribution<double>(0.0, std::abs(jump))(rng);
}

/** If the new point is out of its bounds, it is changed to fit in the bound
 * limits
 *
 * @param parameterIndex :: the index of the parameter
 * @param newValue :: the value of the parameter
 * @param step :: the step used to modify the parameter value
 */
void FABADAMinimizer::boundApplication(const size_t &parameterIndex,
                                       double &newValue, double &step) {
  API::IConstraint *iConstraint = m_fitFunction->getConstraint(parameterIndex);
  if (!iConstraint)
    return;
  Constraints::BoundaryConstraint *bcon =
      dynamic_cast<Constraints::BoundaryConstraint *>(iConstraint);
  if (!bcon)
    return;

  double lower = bcon->lower();
  double upper = bcon->upper();
  double delta = upper - lower;

  // Lower
  while (newValue < lower) {
    if (std::abs(step) > delta) {
      newValue = m_parameters.get(parameterIndex) + step / 10.0;
      step = step / 10;
      m_jump[parameterIndex] = m_jump[parameterIndex] / 10;
    } else {
      newValue =
          lower + std::abs(step) - (m_parameters.get(parameterIndex) - lower);
    }
  }
  // Upper
  while (newValue > upper) {
    if (std::abs(step) > delta) {
      newValue = m_parameters.get(parameterIndex) + step / 10.0;
      step = step / 10;
      m_jump[parameterIndex] = m_jump[parameterIndex] / 10;
    } else {
      newValue =
          upper - (std::abs(step) + m_parameters.get(parameterIndex) - upper);
    }
  }
}

/** Applies ties to parameters. Ties are applied to other parameters first and
 *sequentially, finally ties are applied to the current parameter
 *
 * @param parameterIndex :: the index of the parameter
 * @param newParameters :: the value of the parameters after applying ties
 * @param newValue :: new value of the current parameter
 */
void FABADAMinimizer::tieApplication(const size_t &parameterIndex,
                                     GSLVector &newParameters,
                                     double &newValue) {
  // Fulfill the ties of the other parameters
  for (size_t j = 0; j < m_nParams; ++j) {
    if (j != parameterIndex) {
      API::ParameterTie *tie = m_fitFunction->getTie(j);
      if (tie) {
        newValue = tie->eval();
        if (boost::math::isnan(newValue)) { // maybe not needed
          throw std::runtime_error("Parameter value is NaN.");
        }
        newParameters.set(j, newValue);
        m_fitFunction->setParameter(j, newValue);
      }
    }
  }
  // After all the other variables, the current one is updated to the ties
  API::ParameterTie *tie = m_fitFunction->getTie(parameterIndex);
  if (tie) {
    newValue = tie->eval();
    if (boost::math::isnan(newValue)) { // maybe not needed
      throw std::runtime_error("Parameter value is NaN.");
    }
    newParameters.set(parameterIndex, newValue);
    m_fitFunction->setParameter(parameterIndex, newValue);
  }

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

/** Given the new chi2, next position is calculated and updated.
 *
 * @param parameterIndex :: the index of the parameter
 * @param chi2New :: the new value of chi2
 * @param newParameters :: new value of the fitting parameters
 */
void FABADAMinimizer::algorithmDisplacement(const size_t &parameterIndex,
                                            const double &chi2New,
                                            GSLVector &newParameters) {

  // If new Chi square value is lower, jumping directly to new parameter
  if (chi2New < m_chi2) {
    for (size_t j = 0; j < m_nParams; j++) {
      m_chain[j].push_back(newParameters.get(j));
    }
    m_chain[m_nParams].push_back(chi2New);
    m_parameters = newParameters;
    m_chi2 = chi2New;
    m_changes[parameterIndex] += 1;
  }

  // If new Chi square value is higher, it depends on the probability
  else {
    // Calculate probability of change
    double prob = exp((m_chi2 - chi2New) / (2.0 * m_temperature));

    // Decide if changing or not
    double p = std::uniform_real_distribution<double>(0.0, 1.0)(rng);
    if (p <= prob) {
      for (size_t j = 0; j < m_nParams; j++) {
        m_chain[j].push_back(newParameters.get(j));
      }
      m_chain[m_nParams].push_back(chi2New);
      m_parameters = newParameters;
      m_chi2 = chi2New;
      m_changes[parameterIndex] += 1;
    } else {
      for (size_t j = 0; j < m_nParams; j++) {
        m_chain[j].push_back(m_parameters.get(j));
      }
      m_chain[m_nParams].push_back(m_chi2);
      // Old parameters taken again
      for (size_t j = 0; j < m_nParams; ++j) {
        m_fitFunction->setParameter(j, m_parameters.get(j));
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

/** Updates the parameterIndex-th parameter jump if appropriate
 *
 * @param parameterIndex :: the index of the current parameter
 */
void FABADAMinimizer::jumpUpdate(const size_t &parameterIndex) {
  const double jumpAR = getProperty("JumpAcceptanceRate");
  double newJump;

  if (m_leftRefrPoints == 0 &&
      m_changes[parameterIndex] == m_changesOld[parameterIndex])
    ++m_numInactiveRegenerations[parameterIndex];
  else
    m_changesOld[parameterIndex] = m_changes[parameterIndex];

  if (m_changes[parameterIndex] == 0) {
    newJump = m_jump[parameterIndex] / JUMP_CHECKING_RATE;
    // JUST FOR THE CASE THERE HAS NOT BEEN ANY CHANGE
    //(treated as if only one acceptance).
  } else {
    m_numInactiveRegenerations[parameterIndex] = 0;
    double f = m_changes[parameterIndex] / double(m_counter);

    //*ALTERNATIVE CODE
    //*Current acceptance rate evaluated
    //*double f = m_changes[parameterIndex] / double(JUMP_CHECKING_RATE);
    //*Obs: should be quicker to explore, but less stable (maybe not ergodic)

    newJump = m_jump[parameterIndex] * f / jumpAR;

    //*ALTERNATIVE CODE
    //*Reset the m_changes value to get the information
    //*for the current jump, not the whole history (maybe not ergodic)
    //*m_changes[parameterIndex] = 0;
  }

  m_jump[parameterIndex] = newJump;

  // Check if the new jump is too small. It means that it has been a wrong
  // convergence.
  if (std::abs(m_jump[parameterIndex]) < LOW_JUMP_LIMIT) {
    g_log.warning()
        << "Wrong convergence might be reached for parameter " +
               m_fitFunction->parameterName(parameterIndex) +
               ". Try to set a proper initial value for this parameter\n";
  }
}

/** Check if Chi square has converged for all the parameters if overexploring or
 * Simulated Annealing completed
 *
 */
void FABADAMinimizer::convergenceCheck() {
  size_t innactConvCriterion = getProperty("InnactiveConvergenceCriterion");

  if (m_leftRefrPoints == 0 && m_counter > LOWER_CONVERGENCE_LIMIT &&
      !m_converged) {
    size_t t = 0;
    bool ImmobilityConv = false;
    for (size_t i = 0; i < m_nParams; i++) {
      if (m_parConverged[i]) {
        t += 1;
      } else if (m_numInactiveRegenerations[i] >= innactConvCriterion) {
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

      m_convPoint = m_counterGlobal * m_nParams + 1;
      m_counter = 0;
      for (size_t i = 0; i < m_nParams; ++i) {
        m_changes[i] = 0;
      }

      // If done with a different temperature, the error would be
      // wrongly obtained (because the temperature modifies the
      // chi-square landscape)
      // Although keeping ergodicity, more iterations will be needed
      // because a wrong step is initially used.
      m_temperature = 1.0;
    }

    // All parameters should converge at the same iteration
    else {
      // The not converged parameters can be identified at the last iteration
      if (m_counterGlobal < m_maxIter - m_chainIterations)
        for (size_t i = 0; i < m_nParams; ++i)
          m_parConverged[i] = false;
    }
  }
}

/** Refrigerates the system if appropriate
 *
 */
void FABADAMinimizer::simAnnealingRefrigeration() {
  // Update jump to separate different temperatures
  for (size_t i = 0; i < m_nParams; ++i)
    jumpUpdate(i);

  // Resetting variables for next temperature
  //(independent jump calculation for different temperatures)
  m_counter = 0;
  for (size_t j = 0; j < m_nParams; ++j) {
    m_changes[j] = 0;
  }
  // Simulated Annealing variables updated
  --m_leftRefrPoints;
  // To avoid numerical error accumulation
  if (m_leftRefrPoints == 0)
    m_temperature = 1.0;
  else
    m_temperature /= m_tempStep;
}

/* @return :: true if iteration must continue, false otherwise.
 *
 */
bool FABADAMinimizer::iterationContinuation() {

  // If still through Simulated Annealing
  if (m_leftRefrPoints != 0)
    return true;

  if (!m_converged) {

    // If there is not convergence continue the iterations.
    if (m_counterGlobal < m_maxIter - m_chainIterations) {
      return true;
    }
    // If there is not convergence, but it has been made
    // convergenceMaxIterations iterations, stop and throw the error.
    else {
      std::string failed = "";
      for (size_t i = 0; i < m_nParams; ++i) {
        if (!m_parConverged[i]) {
          failed = failed + m_fitFunction->parameterName(i) + ", ";
        }
      }
      failed.replace(failed.end() - 2, failed.end(), ".");
      throw std::runtime_error(
          "Convegence NOT reached after " +
          std::to_string(m_maxIter - m_chainIterations) +
          " iterations.\n   Try to set better initial values for parameters: " +
          failed +
          " Or increase the maximum number of iterations "
          "(MaxIterations property).");
    }
  } else {
    // If convergence has been reached, continue until we complete the chain
    // length. Otherwise, stop interations.
    return m_counter < m_chainIterations;
  }
  // can we even get here? -> Nope (we should not, so we do not want it to
  // continue)
  return false;
}

/** Create the workspace for the complete parameters chain (the last histogram
 *is for the Chi square).
 *
 */
void FABADAMinimizer::outputChains() {

  size_t chainLength = m_chain[0].size();
  API::MatrixWorkspace_sptr wsC = API::WorkspaceFactory::Instance().create(
      "Workspace2D", m_nParams + 1, chainLength, chainLength);

  // Do one iteration for each parameter plus one for Chi square.
  for (size_t j = 0; j < m_nParams + 1; ++j) {
    auto &X = wsC->mutableX(j);
    auto &Y = wsC->mutableY(j);
    for (size_t k = 0; k < chainLength; ++k) {
      X[k] = double(k);
      Y[k] = m_chain[j][k];
    }
  }

  // Set and name the workspace for the complete chain
  setProperty("Chains", wsC);
}

/** Create the workspace containing the converged chain
 *
 * @param convLength :: length of the converged chain
 * @param nSteps :: number of steps done between chain points to avoid
 *correlation
 */
void FABADAMinimizer::outputConvergedChains(size_t convLength, int nSteps) {

  // Create the workspace for the converged part of the chain.
  API::MatrixWorkspace_sptr wsConv;
  if (convLength > 0) {
    wsConv = API::WorkspaceFactory::Instance().create(
        "Workspace2D", m_nParams + 1, convLength, convLength);
  } else {
    g_log.warning() << "Empty converged chain, empty Workspace returned.";
    wsConv = API::WorkspaceFactory::Instance().create("Workspace2D",
                                                      m_nParams + 1, 1, 1);
  }

  // Do one iteration for each parameter plus one for Chi square.
  for (size_t j = 0; j < m_nParams + 1; ++j) {
    std::vector<double>::const_iterator first =
        m_chain[j].begin() + m_convPoint;
    std::vector<double>::const_iterator last = m_chain[j].end();
    std::vector<double> convChain(first, last);
    auto &X = wsConv->mutableX(j);
    auto &Y = wsConv->mutableY(j);
    for (size_t k = 0; k < convLength; ++k) {
      X[k] = double(k);
      Y[k] = convChain[nSteps * k];
    }
  }

  // Set and name the workspace for the converged part of the chain.
  setProperty("ConvergedChain", wsConv);
}

/** Create the workspace containing chi2 values
 *
 * @param convLength :: length of the converged chain
 * @param mostProbableChi2 :: most probable chi2 value
 *correlation
 */
void FABADAMinimizer::outputCostFunctionTable(size_t convLength,
                                              double mostProbableChi2) {
  // Create the workspace for the Chi square values.
  API::ITableWorkspace_sptr wsChi2 =
      API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  wsChi2->addColumn("double", "Chi2 Minimum");
  wsChi2->addColumn("double", "Most Probable Chi2");
  wsChi2->addColumn("double", "reduced Chi2 Minimum");
  wsChi2->addColumn("double", "Most Probable reduced Chi2");

  // Obtain the quantity of the initial data.
  API::FunctionDomain_sptr domain = m_leastSquares->getDomain();
  size_t dataSize = domain->size();

  // Calculate the value for the reduced Chi square.
  double minimumChi2Red =
      m_chi2 / (double(dataSize - m_nParams)); // For de minimum value.
  double mostProbableChi2Red;
  if (convLength > 0)
    mostProbableChi2Red = mostProbableChi2 / (double(dataSize - m_nParams));
  else {
    g_log.warning()
        << "Most probable chi square not calculated. -1 is returned.\n"
        << "Most probable reduced chi square not calculated. -1 is "
           "returned.\n";
    mostProbableChi2Red = -1;
  }

  // Add the information to the workspace and name it.
  API::TableRow row = wsChi2->appendRow();
  row << m_chi2 << mostProbableChi2 << minimumChi2Red << mostProbableChi2Red;
  setProperty("CostFunctionTable", wsChi2);
}

/** Create the workspace containing probability density function (PDF)
 *
 * @param convLength :: length of the converged chain
 * @param reducedChain :: the reduced chain (will be sorted)
 * @return :: most probable chi square value
 */
double
FABADAMinimizer::outputPDF(std::size_t const &convLength,
                           std::vector<std::vector<double>> &reducedChain) {

  // To store the most probable chi square value
  double mostPchi2;

  // Create the workspace for the Probability Density Functions
  int pdfLength = getProperty(
      "NumberBinsPDF"); // histogram length for the PDF output workspace
  if (pdfLength <= 0) {
    g_log.warning() << "Non valid Number of bins for the PDF (<= 0)."
                       " Default value (20 bins) taken\n";
    pdfLength = 20;
  }

  // Calculate the cost function Probability Density Function
  if (convLength > 0) {
    std::sort(reducedChain[m_nParams].begin(), reducedChain[m_nParams].end());
    std::vector<double> xValues((m_nParams + 1) * (pdfLength + 1));
    std::vector<double> yValues((m_nParams + 1) * pdfLength);
    std::vector<double> PDFYAxis(pdfLength, 0);
    double const start = reducedChain[m_nParams][0];
    double const bin =
        (reducedChain[m_nParams][convLength - 1] - start) / double(pdfLength);

    mostPchi2 =
        getMostProbableChiSquared(convLength, reducedChain, pdfLength, xValues,
                                  yValues, PDFYAxis, start, bin);

    if (getProperty("PDF"))
      outputPDF(xValues, yValues, reducedChain, convLength, pdfLength);

  } // if convLength > 0
  else {
    g_log.warning() << "No points to create PDF. Empty Workspace returned.\n";
    mostPchi2 = -1;
  }

  return mostPchi2;
}

void FABADAMinimizer::outputPDF(std::vector<double> &xValues,
                                std::vector<double> &yValues,
                                std::vector<std::vector<double>> &reducedChain,
                                std::size_t const &convLength,
                                int const &pdfLength) {
  setParameterXAndYValuesForPDF(xValues, yValues, reducedChain, convLength,
                                pdfLength);
  auto parameterNames = m_fitFunction->getParameterNames();
  parameterNames.emplace_back("Chi Squared");
  auto const workspace =
      createWorkspace(xValues, yValues, int(m_nParams) + 1, parameterNames);

  if (AnalysisDataService::Instance().doesExist(PDF_GROUP_NAME)) {
    auto groupPDF = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        PDF_GROUP_NAME);
    groupPDF->addWorkspace(workspace);
    AnalysisDataService::Instance().addOrReplace(PDF_GROUP_NAME, groupPDF);
  } else {
    auto groupPDF = boost::make_shared<WorkspaceGroup>();
    groupPDF->addWorkspace(workspace);
    AnalysisDataService::Instance().addOrReplace(PDF_GROUP_NAME, groupPDF);
  }
}

double FABADAMinimizer::getMostProbableChiSquared(
    std::size_t const &convLength,
    std::vector<std::vector<double>> &reducedChain, int const &pdfLength,
    std::vector<double> &xValues, std::vector<double> &yValues,
    std::vector<double> &PDFYAxis, double const &start, double const &bin) {
  std::size_t step = 0;
  std::size_t const chiXStartPos = m_nParams * (pdfLength + 1);
  std::size_t const chiYStartPos = m_nParams * pdfLength;

  xValues[chiXStartPos] = start;
  for (std::size_t i = 1; i < static_cast<std::size_t>(pdfLength) + 1; i++) {
    double binEnd = start + double(i) * bin;
    xValues[chiXStartPos + i] = binEnd;
    while (step < convLength && reducedChain[m_nParams][step] <= binEnd) {
      PDFYAxis[i - 1] += 1;
      ++step;
    }
    // Divided by convLength * bin to normalize
    yValues[chiYStartPos + i - 1] =
        PDFYAxis[i - 1] / (double(convLength) * bin);
  }

  auto indexMostProbableChi2 =
      std::max_element(PDFYAxis.begin(), PDFYAxis.end());

  return xValues[indexMostProbableChi2 - PDFYAxis.begin()] + (bin / 2.0);
}

void FABADAMinimizer::setParameterXAndYValuesForPDF(
    std::vector<double> &xValues, std::vector<double> &yValues,
    std::vector<std::vector<double>> &reducedChain,
    std::size_t const &convLength, int const &pdfLength) {
  for (std::size_t j = 0; j < m_nParams; ++j) {

    // Calculate the Probability Density Function
    std::vector<double> PDFYAxis(pdfLength, 0);
    double start = reducedChain[j][0];
    double bin = (reducedChain[j][convLength - 1] - start) / double(pdfLength);
    std::size_t step = 0;
    std::size_t const startXPos = j * (pdfLength + 1);
    std::size_t const startYPos = j * pdfLength;

    xValues[startXPos] = start;
    for (std::size_t i = 1; i < static_cast<std::size_t>(pdfLength) + 1; i++) {
      double binEnd = start + double(i) * bin;
      xValues[startXPos + i] = binEnd;
      while (step < convLength && reducedChain[j][step] <= binEnd) {
        PDFYAxis[i - 1] += 1;
        ++step;
      }
      yValues[startYPos + i - 1] = PDFYAxis[i - 1] / (double(convLength) * bin);
    }
  }
}

/** Create the table workspace containing parameter values
 *
 * @param bestParameters :: vector containing best values for fitting parameters
 * @param errorLeft :: [output] vector containing the sqrt of the mean square
 *left deviation
 * @param errorRight :: [output] vector containing the sqrt of the mean square
 *right deviation
 */
void FABADAMinimizer::outputParameterTable(
    const std::vector<double> &bestParameters,
    const std::vector<double> &errorLeft,
    const std::vector<double> &errorRight) {

  // Create the workspace for the parameters' value and errors.
  API::ITableWorkspace_sptr wsPdfE =
      API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  wsPdfE->addColumn("str", "Name");
  wsPdfE->addColumn("double", "Value");
  wsPdfE->addColumn("double", "Left's error");
  wsPdfE->addColumn("double", "Right's error");

  for (size_t j = 0; j < m_nParams; ++j) {
    API::TableRow row = wsPdfE->appendRow();
    row << m_fitFunction->parameterName(j) << bestParameters[j] << errorLeft[j]
        << errorRight[j];
  }
  // Set and name the Parameter Errors workspace.
  setProperty("Parameters", wsPdfE);
}

/** Create the reduced convergence chain and calculate the best parameter values
 *and errors
 *
 * @param convLength :: length of the converged chain
 * @param nSteps :: number of steps done between chain points to avoid
 * @param reducedChain :: [output] the reduced chain
 * @param bestParameters :: [output] vector containing best values for fitting
 *parameters
 * @param errorLeft :: [output] vector containing the sqrt of the mean square
 *left deviation
 * @param errorRight :: [output] vector containing the sqrt of the mean square
 *right deviation
 */
void FABADAMinimizer::calculateConvChainAndBestParameters(
    size_t convLength, int nSteps,
    std::vector<std::vector<double>> &reducedChain,
    std::vector<double> &bestParameters, std::vector<double> &errorLeft,
    std::vector<double> &errorRight) {

  // In case of reduced chain
  if (convLength > 0) {
    // Write first element of the reduced chain
    for (size_t e = 0; e <= m_nParams; ++e) {
      std::vector<double> v;
      v.emplace_back(m_chain[e][m_convPoint]);
      reducedChain.emplace_back(std::move(v));
    }

    // Calculate the reducedConvergedChain for the cost fuction.
    for (size_t k = 1; k < convLength; ++k) {
      reducedChain[m_nParams].emplace_back(
          m_chain[m_nParams][nSteps * k + m_convPoint]);
    }

    // Calculate the position of the minimum Chi square value
    auto positionMinChi2 = std::min_element(reducedChain[m_nParams].begin(),
                                            reducedChain[m_nParams].end());
    m_chi2 = *positionMinChi2;

    // Calculate the parameter value and the errors
    for (size_t j = 0; j < m_nParams; ++j) {
      // Obs: Starts at 1 (0 already added)
      for (size_t k = 1; k < convLength; ++k) {
        reducedChain[j].push_back(m_chain[j][m_convPoint + nSteps * k]);
      }
      // best fit parameters taken
      bestParameters[j] =
          reducedChain[j][positionMinChi2 - reducedChain[m_nParams].begin()];
      std::sort(reducedChain[j].begin(), reducedChain[j].end());
      auto posBestPar = std::find(reducedChain[j].begin(),
                                  reducedChain[j].end(), bestParameters[j]);
      double varLeft = 0, varRight = 0;
      for (auto k = reducedChain[j].begin(); k < reducedChain[j].end();
           k += 2) {
        if (k < posBestPar)
          varLeft += (*k - bestParameters[j]) * (*k - bestParameters[j]);
        else if (k > posBestPar)
          varRight += (*k - bestParameters[j]) * (*k - bestParameters[j]);
      }
      if (posBestPar != reducedChain[j].begin())
        varLeft /= double(posBestPar - reducedChain[j].begin());
      if (posBestPar != reducedChain[j].end() - 1)
        varRight /= double(reducedChain[j].end() - posBestPar - 1);

      errorLeft[j] = -sqrt(varLeft);
      errorRight[j] = sqrt(varRight);
    }
  } // End if there is converged chain

  // If the converged chain is empty
  else {
    g_log.warning() << "There is no converged chain."
                       " Thus the parameters' errors are not"
                       " computed.\n";
    for (size_t k = 0; k < m_nParams; ++k) {
      bestParameters[k] = *(m_chain[k].end() - 1);
    }
  }
}

/** Initialze member variables related to fitting parameters
 *
 */
void FABADAMinimizer::initChainsAndParameters() {
  // The "real" parametersare got (not the active ones)
  m_nParams = m_fitFunction->nParams();
  if (m_nParams == 0) {
    throw std::invalid_argument("Function has 0 fitting parameters.");
  }
  // The initial parameters are saved
  if (m_parameters.size() != m_nParams) {
    m_parameters.resize(m_nParams);
  }

  size_t n = getProperty("ChainLength");
  m_chainIterations = size_t(ceil(double(n) / double(m_nParams)));

  // Save parameter constraints
  for (size_t i = 0; i < m_nParams; ++i) {

    double param = m_fitFunction->getParameter(i);
    m_parameters.set(i, param);

    API::IConstraint *iConstraint = m_fitFunction->getConstraint(i);
    if (iConstraint) {
      Constraints::BoundaryConstraint *bcon =
          dynamic_cast<Constraints::BoundaryConstraint *>(iConstraint);
      if (bcon) {
        if (bcon->hasLower()) {
          if (param < bcon->lower())
            m_parameters.set(i, bcon->lower());
        }
        if (bcon->hasUpper()) {
          if (param > bcon->upper())
            m_parameters.set(i, bcon->upper());
        }
      }
    }

    // Initialize chains
    m_chain.push_back(std::vector<double>(1, param));
    // Initilize jump parameters
    m_jump.push_back(param != 0.0 ? std::abs(param / 10) : 0.01);
  }
  m_chi2 = m_leastSquares->val();
  m_chain.push_back(std::vector<double>(1, m_chi2));
  m_parChanged = std::vector<bool>(m_nParams, false);
  m_changes = std::vector<int>(m_nParams, 0);
  m_changesOld = m_changes;
  m_numInactiveRegenerations = std::vector<size_t>(m_nParams, 0);
  m_parConverged = std::vector<bool>(m_nParams, false);
  m_criteria =
      std::vector<double>(m_nParams, getProperty("ConvergenceCriteria"));
}

/** Initialize member variables used for simulated annealing
 *
 */
void FABADAMinimizer::initSimulatedAnnealing() {

  // Obs: Simulated Annealing with maximum temperature = 1.0, 1step,
  // could be used to increase the "burn-in" period before beginning to
  // check for convergence (Not the ideal way -> Think something)
  if (getProperty("SimAnnealingApplied")) {

    m_temperature = getProperty("MaximumTemperature");
    if (m_temperature == 0.0) {
      g_log.warning() << "MaximumTemperature not a valid temperature"
                         " (T = 0). Default (T = 10.0) taken.\n";
      m_temperature = 10.0;
    }
    if (m_temperature < 0) {
      g_log.warning() << "MaximumTemperature not a temperature"
                         " (< 0), absolute value taken\n";
      m_temperature = -m_temperature;
    }
    m_overexploration = getProperty("Overexploration");
    if (!m_overexploration && m_temperature < 1) {
      m_temperature = 1 / m_temperature;
      g_log.warning() << "MaximumTemperature reduces proper"
                         " exploration (0 < T < 1), product inverse taken ("
                      << m_temperature << ")\n";
    }
    if (m_overexploration && m_temperature > 1) {
      m_overexploration = false;
      g_log.warning()
          << "Overexploration wrong temperature. Not"
             " overexploring. Applying usual Simulated Annealing.\n";
    }
    // Obs: The result is truncated to not have more iterations than
    // the chosen by the user and for all temperatures have the same
    // number of iterations
    m_leftRefrPoints = getProperty("NumRefrigerationSteps");
    if (m_leftRefrPoints <= 0) {
      g_log.warning()
          << "Wrong value for the number of refrigeration"
             " points (<= 0). Therefore, default value (5 points) taken.\n";
      m_leftRefrPoints = 5;
    }

    m_simAnnealingItStep = getProperty("SimAnnealingIterations");
    m_simAnnealingItStep /= m_leftRefrPoints;

    m_tempStep = pow(m_temperature, 1.0 / double(m_leftRefrPoints));

    // m_simAnnealingItStep stores the number of iterations per step
    // 50 for pseudo-continuous temperature decrease
    // without hindering the fitting algorithm itself
    if (m_simAnnealingItStep < 50 && !m_overexploration) {
      g_log.warning()
          << "SimAnnealingIterations/NumRefrigerationSteps too small"
             " (< 50 it). Simulated Annealing not applied\n";
      m_leftRefrPoints = 0;
      m_temperature = 1.0;
    }

    // During Overexploration, the temperature will not be changed
    if (m_overexploration)
      m_leftRefrPoints = 0;
  } else {
    m_temperature = 1.0;
    m_leftRefrPoints = 0;
  }
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
