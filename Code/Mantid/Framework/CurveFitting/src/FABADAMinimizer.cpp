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
// histogram length for the PDF output workspace
const size_t pdf_length = 50;
// number of iterations when convergence isn't expected
const size_t lowerIterationLimit = 350;
}

DECLARE_FUNCMINIMIZER(FABADAMinimizer, FABADA)

//----------------------------------------------------------------------------------------------
/// Constructor
FABADAMinimizer::FABADAMinimizer() : API::IFuncMinimizer(), m_conv_point(0) {
  declareProperty("ChainLength", static_cast<size_t>(10000),
                  "Length of the converged chain.");
  declareProperty(
      "ConvergenceCriteria", 0.0001,
      "Variance in Chi square for considering convergence reached.");
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspacePDF", "pdf",
                                               Kernel::Direction::Output),
                  "The name to give the output workspace");
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspaceChain", "chain",
                                               Kernel::Direction::Output),
                  "The name to give the output workspace");
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspaceConverged", "",
                                               Kernel::Direction::Output,
                                               API::PropertyMode::Optional),
                  "The name to give the output workspace");
  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(
                      "ChiSquareTable", "chi2", Kernel::Direction::Output),
                  "The name to give the output workspace");
  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(
                      "PdfError", "pdfE", Kernel::Direction::Output),
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
          m_lower.push_back(-10e100);
        }
        if (bcon->hasUpper()) {
          m_upper.push_back(bcon->upper());
        } else {
          m_upper.push_back(10e100);
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
    }
    std::vector<double> v;
    v.push_back(p);
    m_chain.push_back(v);
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
}

/// Do one iteration. Returns true if iterations to be continued, false if they
/// must stop.
bool FABADAMinimizer::iterate(size_t) {

  if (!m_leastSquares) {
    throw std::runtime_error("Cost function isn't set up.");
  }

  size_t n = m_leastSquares->nParams();
  size_t m = n;

  // Just for the last iteration. For doing exactly the indicated number of
  // iterations.
  if (m_converged && m_counter == (m_numberIterations)) {
    size_t t = getProperty("ChainLength");
    m = t % n;
  }

  // Do one iteration of FABADA's algorithm for each parameter.
  for (size_t i = 0; i < m; i++) {
    GSLVector new_parameters = m_parameters;

    // Calculate the step, depending on convergence reached or not
    double step;
    if (m_converged) {
      boost::mt19937 mt;
      mt.seed(123 * (int(m_counter) + 45 * int(i)));

      boost::normal_distribution<double> distr(0.0, std::abs(m_jump[i]));
      boost::variate_generator<
          boost::mt19937, boost::normal_distribution<double>> gen(mt, distr);

      step = gen();

    } else {
      step = m_jump[i];
    }
    // Couts just for helping when developing when coding
    /// std::cout << std::endl << m_counter << "." << i <<
    /// std::endl<<std::endl<< m_parameters.get(i)<<std::endl; //DELETE AT THE
    /// END
    /// std::cout << "Real step:  " << step << "      Due to the m_jump:  " <<
    /// m_jump[i] << std::endl; //DELETE AT THE END

    // Calculate the new value of the parameter
    double new_value = m_parameters.get(i) + step;

    // Comproves if it is inside the boundary constrinctions. If not, changes
    // it.
    if (m_bound[i]) {
      if (new_value < m_lower[i]) {
        new_value = m_lower[i] + (m_lower[i] - new_value) / 2;
      }
      if (new_value > m_upper[i]) {
        new_value = m_upper[i] - (new_value - m_upper[i]) / 2;
      }
    }

    // Set the new value in order to calculate the new Chi square value
    if (boost::math::isnan(new_value)) {
      throw std::runtime_error("Parameter value is NaN.");
    }
    new_parameters.set(i, new_value);
    m_leastSquares->setParameter(i, new_value);
    double chi2_new = m_leastSquares->val();

    /// std::cout << "OLD Chi2: " << m_chi2 << "      NEW Chi2:  " << chi2_new
    /// << std::endl; // DELETE AT THE END

    // If new Chi square value is lower, jumping directly to new parameter
    if (chi2_new < m_chi2) {
      for (size_t j = 0; j < n; j++) {
        m_chain[j].push_back(new_parameters.get(j));
      }
      m_chain[n].push_back(chi2_new);
      m_parameters = new_parameters;
      m_chi2 = chi2_new;
      m_changes[i] += 1;
      /// std::cout << "Salta directamente!!" << std::endl;// DELETE AT THE END
    }

    // If new Chi square value is higher, it depends on the probability
    else {
      // Calculate probability of change
      double prob = exp((m_chi2 / 2.0) - (chi2_new / 2.0));
      /// std::cout  << "PROBABILIDAD cambio:   " << prob << std::endl;// DELETE
      /// AT THE END

      // Decide if changing or not
      boost::mt19937 mt;
      mt.seed(int(time_t()) + 48 * (int(m_counter) + 76 * int(i)));
      boost::uniform_real<> distr(0.0, 1.0);
      double p = distr(mt);
      /// std::cout << " Random number " << p << std::endl;// DELETE AT THE END
      if (p <= prob) {
        for (size_t j = 0; j < n; j++) {
          m_chain[j].push_back(new_parameters.get(j));
        }
        m_chain[n].push_back(chi2_new);
        m_parameters = new_parameters;
        m_chi2 = chi2_new;
        m_changes[i] += 1;
        /// std::cout << "SI hay cambio" << std::endl;// DELETE AT THE END
      } else {
        for (size_t j = 0; j < n; j++) {
          m_chain[j].push_back(m_parameters.get(j));
        }
        m_chain[n].push_back(m_chi2);
        m_leastSquares->setParameter(i, new_value - m_jump[i]);
        m_jump[i] = -m_jump[i];
        /// std::cout << "NO hay cambio" << std::endl;// DELETE AT THE END
      }
    }

    /// std::cout << std::endl << std::endl << std::endl;// DELETE AT THE END

    const size_t jumpCheckingRate = 200;
    const double lowJumpLimit = 1e-15;

    // Update the jump once each jumpCheckingRate iterations
    if (m_counter % jumpCheckingRate == 150) {
      double jnew;
      // All this is just a temporal test...
      std::vector<double>::const_iterator first = m_chain[n].end() - 41;
      std::vector<double>::const_iterator last = m_chain[n].end();
      std::vector<double> test(first, last);
      int c = 0;
      for (int j = 0; j < 39; ++j) {
        if (test[j] == test[j + 1]) {
          c += 1;
        }
      }
      if (c > 38) {
        jnew = m_jump[i] / 100;
      } // ...untill here.

      else {
        if (m_changes[i] == 0.0) {
          jnew = m_jump[i] / 10.0;
        } else {
          double f = m_changes[i] / double(m_counter);
          jnew = m_jump[i] * f / 0.6666666666;
          /// std::cout << f << "   m_counter  "<< m_counter << "   m_changes
          /// " << m_changes[i] << std::endl; // DELETE AT THE END
        }
      }
      m_jump[i] = jnew;

      // Check if the new jump is too small. It means that it has been a wrong
      // convergence.
      if (std::abs(m_jump[i]) < lowJumpLimit) {
        API::IFunction_sptr fun = m_leastSquares->getFittingFunction();
        throw std::runtime_error(
            "Wrong convergence for parameter " + fun->parameterName(i) +
            ". Try to set a proper initial value this parameter");
      }
    }

    // Check if the Chi square value has converged for parameter i.
    if (!m_par_converged[i] &&
        m_counter > 350) // It only check since the iteration number 350
    {
      if (chi2_new != m_chi2) {
        double chi2_quotient = std::abs(chi2_new - m_chi2) / m_chi2;
        if (chi2_quotient < m_criteria[i]) {
          m_par_converged[i] = true;
        }
      }
    }
  }

  m_counter +=
      1; // Update the counter, after finishing the iteration for each parameter

  // Check if Chi square has converged for all the parameters.
  if (m_counter > lowerIterationLimit && !m_converged) {
    size_t t = 0;
    for (size_t i = 0; i < n; i++) {
      if (m_par_converged[i]) {
        t += 1;
      }
    }
    // If all parameters have converged:
    // It set up both the counter and the changes' vector to 0, in order to
    // consider only the
    // data of the converged part of the chain, when updating the jump.
    if (t == n) {
      m_converged = true;
      m_conv_point = m_counter * n + 1;
      m_counter = 0;
      for (size_t i = 0; i < n; ++i) {
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
      for (size_t i = 0; i < n; ++i) {
        if (!m_par_converged[i]) {
          failed = failed + fun->parameterName(i) + ", ";
        }
      }
      failed.replace(failed.end() - 2, failed.end(), ".");
      throw std::runtime_error(
          "Convegence NOT reached after " +
          boost::lexical_cast<std::string>(m_counter) +
          " iterations.\n   Try to set better initial values for parameters: " +
          failed);
    }
  } else {
    // If convergence has been reached, continue untill complete the chain
    // length.
    if (m_counter <= m_numberIterations) {
      return true;
    }
    // When the all the iterations have been done, calculate and show all the
    // results.
    else {
      // Create the workspace for the Probability Density Functions
      API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create(
          "Workspace2D", n, pdf_length + 1, pdf_length);

      // Create the workspace for the parameters' value and errors.
      API::ITableWorkspace_sptr wsPdfE =
          API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      wsPdfE->addColumn("str", "Name");
      wsPdfE->addColumn("double", "Value");
      wsPdfE->addColumn("double", "Left's error");
      wsPdfE->addColumn("double", "Rigth's error");

      std::vector<double>::iterator pos_min = std::min_element(
          m_chain[n].begin() + m_conv_point,
          m_chain[n]
              .end()); // Calculate the position of the minimum Chi aquare value
      m_chi2 = *pos_min;
      // index of the minimum chi^2
      size_t minIndex =
          static_cast<size_t>(std::distance(m_chain[n].begin(), pos_min));

      std::vector<double> par_def(n);
      API::IFunction_sptr fun = m_leastSquares->getFittingFunction();

      // Do one iteration for each parameter.
      for (size_t j = 0; j < n; ++j) {
        // Calculate the parameter value and the errors
        par_def[j] = m_chain[j][minIndex];
        std::vector<double>::const_iterator first =
            m_chain[j].begin() + m_conv_point;
        std::vector<double>::const_iterator last = m_chain[j].end();
        std::vector<double> conv_chain(first, last);
        size_t conv_length = conv_chain.size();
        std::sort(conv_chain.begin(), conv_chain.end());
        std::vector<double>::const_iterator pos_par =
            std::find(conv_chain.begin(), conv_chain.end(), par_def[j]);
        int sigma = int(0.34 * double(conv_length));
        // make sure the iterator is valid in any case
        std::vector<double>::const_iterator pos_left = conv_chain.begin();
        if (sigma < static_cast<int>(std::distance(pos_left, pos_par))) {
          pos_left = pos_par - sigma;
        }
        // make sure the iterator is valid in any case
        std::vector<double>::const_iterator pos_right = conv_chain.end() - 1;
        if (sigma < static_cast<int>(std::distance(pos_par, pos_right))) {
          pos_right = pos_par + sigma;
        }
        API::TableRow row = wsPdfE->appendRow();
        row << fun->parameterName(j) << par_def[j] << *pos_left - *pos_par
            << *pos_right - *pos_par;

        // Calculate the Probability Density Function
        std::vector<double> pdf_y(pdf_length, 0);
        double start = conv_chain[0];
        double bin = (conv_chain[conv_length - 1] - start) / pdf_length;
        size_t step = 0;
        MantidVec &X = ws->dataX(j);
        MantidVec &Y = ws->dataY(j);
        X[0] = start;
        for (size_t i = 1; i < pdf_length + 1; i++) {
          double bin_end = start + static_cast<double>(i) * bin;
          X[i] = bin_end;
          while (step < conv_length && conv_chain[step] <= bin_end) {
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

      // Set and name the two workspaces already calculated.
      setProperty("OutputWorkspacePDF", ws);
      setProperty("PdfError", wsPdfE);

      // Create the workspace for the complete parameters' chain (the last
      // histogram is for the Chi square).
      size_t chain_length = m_chain[0].size();
      API::MatrixWorkspace_sptr wsC = API::WorkspaceFactory::Instance().create(
          "Workspace2D", n + 1, chain_length, chain_length);

      // Do one iteration for each parameter plus one for Chi square.
      for (size_t j = 0; j < n + 1; ++j) {
        MantidVec &X = wsC->dataX(j);
        MantidVec &Y = wsC->dataY(j);
        for (size_t k = 0; k < chain_length; ++k) {
          X[k] = double(k);
          Y[k] = m_chain[j][k];
        }
      }

      // Set and name the workspace for the complete chain
      setProperty("OutputWorkspaceChain", wsC);

      // Read if necessary to show the workspace for the converged part of the
      // chain.
      const bool con = !getPropertyValue("OutputWorkspaceConverged").empty();

      if (con) {
        // Create the workspace for the converged part of the chain.
        size_t conv_length = (m_counter - 1) * n + m;
        API::MatrixWorkspace_sptr wsConv =
            API::WorkspaceFactory::Instance().create("Workspace2D", n + 1,
                                                     conv_length, conv_length);

        // Do one iteration for each parameter plus one for Chi square.
        for (size_t j = 0; j < n + 1; ++j) {
          std::vector<double>::const_iterator first =
              m_chain[j].begin() + m_conv_point;
          std::vector<double>::const_iterator last = m_chain[j].end();
          std::vector<double> conv_chain(first, last);
          MantidVec &X = wsConv->dataX(j);
          MantidVec &Y = wsConv->dataY(j);
          for (size_t k = 0; k < conv_length; ++k) {
            X[k] = double(k);
            Y[k] = conv_chain[k];
          }
        }

        // Set and name the workspace for the converged part of the chain.
        setProperty("OutputWorkspaceConverged", wsConv);
      }

      // Create the workspace for the Chi square values.
      API::ITableWorkspace_sptr wsChi2 =
          API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      wsChi2->addColumn("double", "Chi2min");
      wsChi2->addColumn("double", "Chi2MP");
      wsChi2->addColumn("double", "Chi2min_red");
      wsChi2->addColumn("double", "Chi2MP_red");

      // Calculate de Chi square most probable.
      double Chi2MP = m_leastSquares->val();

      // Reset the best parameter values ---> Si al final no se muestra la tabla
      // que sale por defecto, esto se podra borrar...
      for (size_t j = 0; j < n; ++j) {
        m_leastSquares->setParameter(j, par_def[j]);
      }

      // Obtain the quantity of the initial data.
      API::FunctionDomain_sptr domain = m_leastSquares->getDomain();
      size_t data_number = domain->size();

      // Calculate the value for the reduced Chi square.
      double Chi2min_red =
          *pos_min / (double(data_number - n)); // For de minimum value.
      double Chi2MP_red =
          Chi2MP / (double(data_number - n)); // For the most probable.

      // Add the information to the workspace and name it.
      API::TableRow row = wsChi2->appendRow();
      row << *pos_min << Chi2MP << Chi2min_red << Chi2MP_red;
      setProperty("ChiSquareTable", wsChi2);

      return false;
    }
  }

  return true;
}
double FABADAMinimizer::costFunctionVal() { return m_chi2; }

} // namespace CurveFitting
} // namespace Mantid
