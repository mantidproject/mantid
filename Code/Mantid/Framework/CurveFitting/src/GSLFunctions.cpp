//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/GSLFunctions.h"
#include "MantidAPI/ICostFunction.h"
#include "MantidAPI/IConstraint.h"

namespace Mantid {
namespace CurveFitting {

using API::Jacobian;

/** Fit GSL function wrapper
* @param x :: Input function parameters
* @param params :: Input data
* @param f :: Output function values = (y_cal-y_data)/sigma for each data point
* @return A GSL status information
*/
int gsl_f(const gsl_vector *x, void *params, gsl_vector *f) {

  struct GSL_FitData *p = (struct GSL_FitData *)params;

  // update function parameters
  if (x->data) {
    size_t ia = 0;
    for (size_t i = 0; i < p->function->nParams(); ++i) {
      if (p->function->isActive(i)) {
        p->function->setActiveParameter(i, x->data[ia]);
        ++ia;
      }
    }
  }
  p->function->applyTies();

  auto values = boost::dynamic_pointer_cast<API::FunctionValues>(
      p->costFunction->getValues());
  if (!values) {
    throw std::invalid_argument("FunctionValues expected");
  }
  p->function->function(*p->costFunction->getDomain(), *values);

  // Add penalty
  double penalty = 0.;
  for (size_t i = 0; i < p->function->nParams(); ++i) {
    API::IConstraint *c = p->function->getConstraint(i);
    if (c) {
      penalty += c->checkDeriv();
    }
  }

  size_t n = values->size() - 1;
  // add penalty to first and last point and every 10th point in between
  if (penalty != 0.0) {
    values->addToCalculated(0, penalty);
    values->addToCalculated(n, penalty);

    for (size_t i = 9; i < n; i += 10) {
      values->addToCalculated(i, penalty);
    }
  }

  // function() return calculated data values. Need to convert this values into
  // calculated-observed devided by error values used by GSL

  for (size_t i = 0; i < p->n; i++) {
    f->data[i] = (values->getCalculated(i) - values->getFitData(i)) *
                 values->getFitWeight(i);
    // std::cerr << values.getCalculated(i) << ' ' << values.getFitData(i) << '
    // ' << values.getFitWeight(i) << std::endl;
  }

  return GSL_SUCCESS;
}

/** Fit GSL derivative function wrapper
* @param x :: Input function arguments
* @param params :: Input data
* @param J :: Output derivatives
* @return A GSL status information
*/
int gsl_df(const gsl_vector *x, void *params, gsl_matrix *J) {

  struct GSL_FitData *p = (struct GSL_FitData *)params;

  p->J.setJ(J);

  // update function parameters
  if (x->data) {
    size_t ia = 0;
    for (size_t i = 0; i < p->function->nParams(); ++i) {
      if (p->function->isActive(i)) {
        p->function->setActiveParameter(i, x->data[ia]);
        ++ia;
      }
    }
  }
  p->function->applyTies();

  // calculate the Jacobian
  p->function->functionDeriv(*p->costFunction->getDomain(), p->J);

  // p->function->addPenaltyDeriv(&p->J);
  // add penalty
  size_t n = p->costFunction->getDomain()->size() - 1;
  size_t ia = 0;
  for (size_t i = 0; i < p->function->nParams(); ++i) {
    if (!p->function->isActive(i))
      continue;
    API::IConstraint *c = p->function->getConstraint(i);
    if (c) {
      double penalty = c->checkDeriv2();
      if (penalty != 0.0) {
        double deriv = p->J.get(0, ia);
        p->J.set(0, ia, deriv + penalty);
        deriv = p->J.get(n, ia);
        p->J.set(n, ia, deriv + penalty);

        for (size_t j = 9; j < n; j += 10) {
          deriv = p->J.get(j, ia);
          p->J.set(j, ia, deriv + penalty);
        }
      }
    } // if (c)
    ++ia;
  }

  // functionDeriv() return derivatives of calculated data values. Need to
  // convert this values into
  // derivatives of calculated-observed devided by error values used by GSL
  auto values = boost::dynamic_pointer_cast<API::FunctionValues>(
      p->costFunction->getValues());
  if (!values) {
    throw std::invalid_argument("FunctionValues expected");
  }
  for (size_t iY = 0; iY < p->n; iY++)
    for (size_t iP = 0; iP < p->p; iP++) {
      J->data[iY * p->p + iP] *= values->getFitWeight(iY);
      // std::cerr << iY << ' ' << iP << ' ' << J->data[iY*p->p + iP] <<
      // std::endl;
    }

  return GSL_SUCCESS;
}

/** Fit derivatives and function GSL wrapper
* @param x :: Input function arguments
* @param params :: Input data
* @param f :: Output function values = (y_cal-y_cal)/sigma for each data point
* @param J :: Output derivatives
* @return A GSL status information
*/
int gsl_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J) {
  gsl_f(x, params, f);
  gsl_df(x, params, J);
  return GSL_SUCCESS;
}

/**
 * Constructor. Creates declared -> active index map
 * @param cf :: ICostFunction
 */
GSL_FitData::GSL_FitData(boost::shared_ptr<CostFuncLeastSquares> cf)
    : function(cf->getFittingFunction()), costFunction(cf) {
  gsl_set_error_handler_off();
  // number of active parameters
  p = 0;
  for (size_t i = 0; i < function->nParams(); ++i) {
    if (function->isActive(i))
      ++p;
  }

  // number of fitting data
  n = cf->getValues()->size();

  bool functionFixed = false;
  if (p == 0) {
    p = 1;
    functionFixed = true;
  }

  // holdCalculatedJacobian =  gsl_matrix_alloc (n, p);

  initFuncParams = gsl_vector_alloc(p);

  if (functionFixed) {
    gsl_vector_set(initFuncParams, 0, 0.0);
  } else {
    size_t ia = 0;
    for (size_t i = 0; i < function->nParams(); ++i) {
      if (function->isActive(i)) {
        gsl_vector_set(initFuncParams, ia, function->activeParameter(i));
        ++ia;
      }
    }
  }

  int j = 0;
  for (size_t i = 0; i < function->nParams(); ++i) {
    if (function->isActive(i)) {
      J.m_index.push_back(j);
      j++;
    } else
      J.m_index.push_back(-1);
  }
}

GSL_FitData::~GSL_FitData() {
  // gsl_matrix_free(holdCalculatedJacobian);
  gsl_vector_free(initFuncParams);
}

} // namespace CurveFitting
} // namespace Mantid
