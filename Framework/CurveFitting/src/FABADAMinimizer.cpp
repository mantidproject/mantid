#include "MantidCurveFitting/FABADAMinimizer.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/BoundaryConstraint.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

#include "MantidKernel/Logger.h"

#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/version.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#include <iostream>
#include <ctime>

namespace Mantid {
namespace CurveFitting {

namespace {
// static logger object
Kernel::Logger g_log("FABADAMinimizer");
// absolute maximum number of iterations when fit must converge
const size_t convergenceMaxIterations = 50000;
// number of iterations when convergence isn't expected
const size_t lowerIterationLimit = 350;
// very large number
const double largeNumber = 1e100;
// jump checking rate
const size_t jumpCheckingRate = 200;
// low jump limit
const double lowJumpLimit = 1e-25;
}

DECLARE_FUNCMINIMIZER(FABADAMinimizer, FABADA)

//----------------------------------------------------------------------------------------------
/// Constructor
FABADAMinimizer::FABADAMinimizer()
    : m_counter(0), m_numberIterations(0), m_changes(), m_jump(),
      m_parameters(), m_chain(), m_chi2(0.), m_converged(false),
      m_conv_point(0), m_par_converged(), m_lower(), m_upper(), m_bound(),
      m_criteria(), m_max_iter(0) {
  declareProperty("ChainLength", static_cast<size_t>(10000),
                  "Length of the converged chain.");
  declareProperty("StepsBetweenValues", static_cast<size_t>(10),
                  "Steps realized between keeping each result.");
  declareProperty(
      "ConvergenceCriteria", 0.01,
      "Variance in Cost Function for considering convergence reached.");
  declareProperty("JumpAcceptanceRate", 0.6666666,
                  "Desired jumping acceptance rate");
  declareProperty(
      new API::WorkspaceProperty<>("PDF", "PDF", Kernel::Direction::Output),
      "The name to give the output workspace");
  declareProperty(
      new API::WorkspaceProperty<>("Chains", "", Kernel::Direction::Output),
      "The name to give the output workspace");
  declareProperty(new API::WorkspaceProperty<>("ConvergedChain", "",
                                               Kernel::Direction::Output,
                                               API::PropertyMode::Optional),
                  "The name to give the output workspace");
  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(
                      "CostFunctionTable", "", Kernel::Direction::Output),
                  "The name to give the output workspace");
  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(
                      "Parameters", "", Kernel::Direction::Output),
                  "The name to give the output workspace");
}

//----------------------------------------------------------------------------------------------

/// Destructor
FABADAMinimizer::~FABADAMinimizer() {}

/// Initialize minimizer. Set initial values for all private members.
void FABADAMinimizer::initialize(API::ICostFunction_sptr function,
                                 size_t maxIterations) {

  m_leastSquares = boost::dynamic_pointer_cast<CostFuncLeastSquares>(function);
  if (!m_leastSquares) {
    throw std::invalid_argument(
        "FABADA works only with least squares. Different function was given.");
  }

  m_counter = 0;
  m_leastSquares->getParameters(m_parameters);
  API::IFunction_sptr fun = m_leastSquares->getFittingFunction();

  if (fun->nParams() == 0) {
    throw std::invalid_argument("Function has 0 fitting parameters.");
  }

  size_t n = getProperty("ChainLength");
  m_numberIterations = n / fun->nParams();

  if (m_numberIterations > maxIterations) {
    g_log.warning()
        << "MaxIterations property reduces the required number of iterations ("
        << m_numberIterations << ")." << std::endl;
    m_numberIterations = maxIterations;
  }

  for (size_t i = 0; i < m_leastSquares->nParams(); ++i) {
    double p = m_parameters.get(i);
    m_bound.push_back(false);
    API::IConstraint *iconstr = fun->getConstraint(i);
    if (iconstr) {
      BoundaryConstraint *bcon = dynamic_cast<BoundaryConstraint *>(iconstr);
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
    std::vector<double> v;
    v.push_back(p);
    m_chain.push_back(v);
    m_max_iter = maxIterations;
    m_changes.push_back(0);
    m_par_converged.push_back(false);
    m_criteria.push_back(getProperty("ConvergenceCriteria"));
    if (p != 0.0) {
      m_jump.push_back(std::abs(p / 10));
    } else {
      m_jump.push_back(0.01);
    }
  }
  m_chi2 = m_leastSquares->val();
  std::vector<double> v;
  v.push_back(m_chi2);
  m_chain.push_back(v);
  m_converged = false;
  m_max_iter = maxIterations;
}

/// Do one iteration. Returns true if iterations to be continued, false if they
/// must stop.
bool FABADAMinimizer::iterate(size_t) {

  if (!m_leastSquares) {
    throw std::runtime_error("Cost function isn't set up.");
  }

  size_t nParams = m_leastSquares->nParams();
  size_t m = nParams;

  // Just for the last iteration. For doing exactly the indicated number of
  // iterations.
  if (m_converged && m_counter == m_numberIterations) {
    size_t t = getProperty("ChainLength");
    m = t % nParams;
  }

  // Do one iteration of FABADA's algorithm for each parameter.
  for (size_t i = 0; i < m; i++) {
    GSLVector new_parameters = m_parameters;

    // Calculate the step, depending on convergence reached or not
    double step;
    if (m_converged || m_bound[i]) {
      boost::mt19937 mt;
      mt.seed(123 * (int(m_counter) +
                     45 * int(i))); // Numeros inventados para la seed
      boost::normal_distribution<double> distr(0.0, std::abs(m_jump[i]));
      boost::variate_generator<boost::mt19937,
                               boost::normal_distribution<double>> gen(mt,
                                                                       distr);
      step = gen();
    } else {
      step = m_jump[i];
    }

    // Calculate the new value of the parameter
    double new_value = m_parameters.get(i) + step;

    // Comproves if it is inside the boundary constrinctions. If not, changes
    // it.
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

    // Set the new value in order to calculate the new Chi square value
    if (boost::math::isnan(new_value)) {
      throw std::runtime_error("Parameter value is NaN.");
    }
    new_parameters.set(i, new_value);
    m_leastSquares->setParameter(i, new_value);
    double chi2_new = m_leastSquares->val();

    // If new Chi square value is lower, jumping directly to new parameter
    if (chi2_new < m_chi2) {
      for (size_t j = 0; j < nParams; j++) {
        m_chain[j].push_back(new_parameters.get(j));
      }
      m_chain[nParams].push_back(chi2_new);
      m_parameters = new_parameters;
      m_chi2 = chi2_new;
      m_changes[i] += 1;

    }

    // If new Chi square value is higher, it depends on the probability
    else {
      // Calculate probability of change
      double prob = exp((m_chi2 / 2.0) - (chi2_new / 2.0));

      // Decide if changing or not
      boost::mt19937 mt;
      mt.seed(int(time_t()) + 48 * (int(m_counter) + 76 * int(i)));
      boost::uniform_real<> distr(0.0, 1.0);
      double p = distr(mt);
      if (p <= prob) {
        for (size_t j = 0; j < nParams; j++) {
          m_chain[j].push_back(new_parameters.get(j));
        }
        m_chain[nParams].push_back(chi2_new);
        m_parameters = new_parameters;
        m_chi2 = chi2_new;
        m_changes[i] += 1;
      } else {
        for (size_t j = 0; j < nParams; j++) {
          m_chain[j].push_back(m_parameters.get(j));
        }
        m_chain[nParams].push_back(m_chi2);
        m_leastSquares->setParameter(i, new_value - m_jump[i]);
        m_jump[i] = -m_jump[i];
      }
    }

    const double jumpAR = getProperty("JumpAcceptanceRate");

    // Update the jump once each jumpCheckingRate iterations
    if (m_counter % jumpCheckingRate == 150) // JUMP CHECKING RATE IS 200, BUT
                                             // IS NOT CHECKED AT FIRST STEP, IT
                                             // IS AT 150
    {
      double jnew;
      if (m_changes[i] == 0.0) {
        jnew = m_jump[i] /
               10.0; // JUST FOR THE CASE THERE HAS NOT BEEN ANY CHANGE.
      } else {
        double f = m_changes[i] / double(m_counter);
        jnew = m_jump[i] * f / jumpAR;
      }

      m_jump[i] = jnew;

      // Check if the new jump is too small. It means that it has been a wrong
      // convergence.
      if (std::abs(m_jump[i]) < lowJumpLimit) {
        API::IFunction_sptr fun = m_leastSquares->getFittingFunction();
        g_log.warning()
            << "Wrong convergence for parameter " + fun->parameterName(i) +
                   ". Try to set a proper initial value for this parameter"
            << std::endl;
      }
    }

    // Check if the Chi square value has converged for parameter i.
    const size_t startingPoint =
        350; // The iteration since it starts to check if convergence is reached
    if (!m_par_converged[i] && m_counter > startingPoint) {
      if (chi2_new != m_chi2) {
        double chi2_quotient = std::abs(chi2_new - m_chi2) / m_chi2;
        if (chi2_quotient < m_criteria[i]) {
          m_par_converged[i] = true;
        }
      }
    }
  } // for i

  // Update the counter, after finishing the iteration for each parameter
  m_counter += 1;

  // Check if Chi square has converged for all the parameters.
  if (m_counter > lowerIterationLimit && !m_converged) {
    size_t t = 0;
    for (size_t i = 0; i < nParams; i++) {
      if (m_par_converged[i]) {
        t += 1;
      }
    }
    // If all parameters have converged:
    // It set up both the counter and the changes' vector to 0, in order to
    // consider only the data of the converged part of the chain, when updating
    // the jump.
    if (t == nParams) {
      m_converged = true;
      m_conv_point = m_counter * nParams + 1;
      m_counter = 0;
      for (size_t i = 0; i < nParams; ++i) {
        m_changes[i] = 0;
      }
    }
  }

  if (!m_converged) {
    // If there is not convergence continue the iterations.
    if (m_counter <= convergenceMaxIterations &&
        m_counter < m_numberIterations - 1) {
      return true;
    }
    // If there is not convergence, but it has been made
    // convergenceMaxIterations iterations, stop and throw the error.
    else {
      API::IFunction_sptr fun = m_leastSquares->getFittingFunction();
      std::string failed = "";
      for (size_t i = 0; i < nParams; ++i) {
        if (!m_par_converged[i]) {
          failed = failed + fun->parameterName(i) + ", ";
        }
      }
      failed.replace(failed.end() - 2, failed.end(), ".");
      throw std::runtime_error(
          "Convegence NOT reached after " +
          boost::lexical_cast<std::string>(m_max_iter) +
          " iterations.\n   Try to set better initial values for parameters: " +
          failed);
    }
  } else {
    // If convergence has been reached, continue untill complete the chain
    // length.
    if (m_counter <= m_numberIterations) {
      return true;
    }
    // If convergence has been reached, but the maximum of iterations have been
    // reached before finishing the chain, stop and throw the error.
    if (m_counter >= m_max_iter) {
      throw std::length_error("Convegence reached but Max Iterations parameter "
                              "insufficient for creating the whole chain.\n "
                              "Increase Max Iterations");
    }
    // nothing else to do, stop interations
    return false;
  }
  // can we even get here?
  return true;
}

double FABADAMinimizer::costFunctionVal() { return m_chi2; }

/// When the all the iterations have been done, calculate and show all the
/// results.
void FABADAMinimizer::finalize() {
  // Creating the reduced chain (considering only one each "Steps between
  // values" values)
  size_t cl = getProperty("ChainLength");
  size_t n_steps = getProperty("StepsBetweenValues");
  size_t conv_length = size_t(double(cl) / double(n_steps));
  std::vector<std::vector<double>> red_conv_chain;
  size_t nParams = m_leastSquares->nParams();
  for (size_t e = 0; e <= nParams; ++e) {
    std::vector<double> v;
    v.push_back(m_chain[e][m_conv_point]);
    red_conv_chain.push_back(v);
  }

  // Calculate the red_conv_chain for the cost fuction.
  auto first = m_chain[nParams].begin() + m_conv_point;
  auto last = m_chain[nParams].end();
  std::vector<double> conv_chain(first, last);
  for (size_t k = 1; k < conv_length; ++k) {
    red_conv_chain[nParams].push_back(conv_chain[n_steps * k]);
  }

  // Calculate the position of the minimum Chi square value
  auto pos_min = std::min_element(red_conv_chain[nParams].begin(),
                                  red_conv_chain[nParams].end());
  m_chi2 = *pos_min;

  std::vector<double> par_def(nParams);
  std::vector<double> error_left(nParams);
  std::vector<double> error_rigth(nParams);
  API::IFunction_sptr fun = m_leastSquares->getFittingFunction();

  // Do one iteration for each parameter.
  for (size_t j = 0; j < nParams; ++j) {
    // Calculate the parameter value and the errors
    auto first = m_chain[j].begin() + m_conv_point;
    auto last = m_chain[j].end();
    std::vector<double> conv_chain(first, last);
    auto &rc_chain_j = red_conv_chain[j];
    for (size_t k = 0; k < conv_length; ++k) {
      rc_chain_j.push_back(conv_chain[n_steps * k]);
    }
    par_def[j] = rc_chain_j[pos_min - red_conv_chain[nParams].begin()];
    std::sort(rc_chain_j.begin(), rc_chain_j.end());
    auto pos_par = std::find(rc_chain_j.begin(), rc_chain_j.end(), par_def[j]);
    auto pos_left = rc_chain_j.begin();
    auto pos_right = rc_chain_j.end() - 1;
    size_t sigma = static_cast<size_t>(0.34 * double(conv_length));
    if (pos_par == rc_chain_j.end()) {
      error_left[j] = *(pos_right - sigma);
      error_rigth[j] = *pos_right;
    } else {

      if (sigma < static_cast<size_t>(std::distance(pos_left, pos_par))) {
        pos_left = pos_par - sigma;
      }
      // make sure the iterator is valid in any case
      if (sigma < static_cast<size_t>(std::distance(pos_par, pos_right))) {
        pos_right = pos_par + sigma;
      }
      error_left[j] = *pos_left - *pos_par;
      error_rigth[j] = *pos_right - *pos_par;
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

    for (size_t j = 0; j < nParams; ++j) {
      API::TableRow row = wsPdfE->appendRow();
      row << fun->parameterName(j) << par_def[j] << error_left[j]
          << error_rigth[j];
    }
    // Set and name the Parameter Errors workspace.
    setProperty("Parameters", wsPdfE);
  }

  // Set the best parameter values
  for (size_t j = 0; j < nParams; ++j) {
    m_leastSquares->setParameter(j, par_def[j]);
  }

  double mostPchi2;

  // Create the workspace for the Probability Density Functions
  size_t pdf_length = 20; // histogram length for the PDF output workspace
  API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
      "Workspace2D", nParams + 1, pdf_length + 1, pdf_length);

  // Calculate the cost function Probability Density Function
  std::sort(red_conv_chain[nParams].begin(), red_conv_chain[nParams].end());
  std::vector<double> pdf_y(pdf_length, 0);
  double start = red_conv_chain[nParams][0];
  double bin =
      (red_conv_chain[nParams][conv_length - 1] - start) / double(pdf_length);
  size_t step = 0;
  MantidVec &X = ws->dataX(nParams);
  MantidVec &Y = ws->dataY(nParams);
  X[0] = start;
  for (size_t i = 1; i < pdf_length + 1; i++) {
    double bin_end = start + double(i) * bin;
    X[i] = bin_end;
    while (step < conv_length && red_conv_chain[nParams][step] <= bin_end) {
      pdf_y[i - 1] += 1;
      ++step;
    }
    Y[i - 1] = pdf_y[i - 1] / (double(conv_length) * bin);
  }

  std::vector<double>::iterator pos_MPchi2 =
      std::max_element(pdf_y.begin(), pdf_y.end());

  if (pos_MPchi2 - pdf_y.begin() == 0) {
    // mostPchi2 = X[pos_MPchi2-pdf_y.begin()];
    mostPchi2 = *pos_min;
  } else {
    mostPchi2 = X[pos_MPchi2 - pdf_y.begin()] + (bin / 2.0);
  }

  // Do one iteration for each parameter.
  for (size_t j = 0; j < nParams; ++j) {
    // Calculate the Probability Density Function
    std::vector<double> pdf_y(pdf_length, 0);
    double start = red_conv_chain[j][0];
    double bin =
        (red_conv_chain[j][conv_length - 1] - start) / double(pdf_length);
    size_t step = 0;
    MantidVec &X = ws->dataX(j);
    MantidVec &Y = ws->dataY(j);
    X[0] = start;
    for (size_t i = 1; i < pdf_length + 1; i++) {
      double bin_end = start + double(i) * bin;
      X[i] = bin_end;
      while (step < conv_length && red_conv_chain[j][step] <= bin_end) {
        pdf_y[i - 1] += 1;
        ++step;
      }
      Y[i - 1] = pdf_y[i - 1] / (double(conv_length) * bin);
    }

    // Calculate the most probable value, from the PDF.
    std::vector<double>::iterator pos_MP =
        std::max_element(pdf_y.begin(), pdf_y.end());
    double mostP = X[pos_MP - pdf_y.begin()] + (bin / 2.0);
    m_leastSquares->setParameter(j, mostP);
  }

  // Set and name the PDF workspace.
  setProperty("PDF", ws);

  const bool outputChains = !getPropertyValue("Chains").empty();

  if (outputChains) {

    // Create the workspace for the complete parameters' chain (the last
    // histogram is for the Chi square).
    size_t chain_length = m_chain[0].size();
    API::MatrixWorkspace_sptr wsC = API::WorkspaceFactory::Instance().create(
        "Workspace2D", nParams + 1, chain_length, chain_length);

    // Do one iteration for each parameter plus one for Chi square.
    for (size_t j = 0; j < nParams + 1; ++j) {
      MantidVec &X = wsC->dataX(j);
      MantidVec &Y = wsC->dataY(j);
      for (size_t k = 0; k < chain_length; ++k) {
        X[k] = double(k);
        Y[k] = m_chain[j][k];
      }
    }

    // Set and name the workspace for the complete chain
    setProperty("Chains", wsC);
  }

  // Read if necessary to show the workspace for the converged part of the
  // chain.
  const bool outputConvergedChains =
      !getPropertyValue("ConvergedChain").empty();

  if (outputConvergedChains) {
    // Create the workspace for the converged part of the chain.
    API::MatrixWorkspace_sptr wsConv = API::WorkspaceFactory::Instance().create(
        "Workspace2D", nParams + 1, conv_length, conv_length);

    // Do one iteration for each parameter plus one for Chi square.
    for (size_t j = 0; j < nParams + 1; ++j) {
      std::vector<double>::const_iterator first =
          m_chain[j].begin() + m_conv_point;
      std::vector<double>::const_iterator last = m_chain[j].end();
      std::vector<double> conv_chain(first, last);
      MantidVec &X = wsConv->dataX(j);
      MantidVec &Y = wsConv->dataY(j);
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
        m_chi2 / (double(data_number - nParams)); // For de minimum value.
    double mostPchi2_red = mostPchi2 / (double(data_number - nParams));

    // Add the information to the workspace and name it.
    API::TableRow row = wsChi2->appendRow();
    row << m_chi2 << mostPchi2 << Chi2min_red << mostPchi2_red;
    setProperty("CostFunctionTable", wsChi2);
  }

  // Set the best parameter values
  for (size_t j = 0; j < nParams; ++j) {
    m_leastSquares->setParameter(j, par_def[j]);
  }
}

} // namespace CurveFitting
} // namespace Mantid
