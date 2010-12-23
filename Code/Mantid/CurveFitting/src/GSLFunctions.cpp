//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/GSLFunctions.h"
#include "MantidCurveFitting/ICostFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction.h"


namespace Mantid
{
namespace CurveFitting
{

  using API::Jacobian;

  /** Fit GSL function wrapper
  * @param x Input function parameters
  * @param params Input data
  * @param f Output function values = (y_cal-y_data)/sigma for each data point
  * @return A GSL status information
  */
  int gsl_f(const gsl_vector * x, void *params, gsl_vector * f) {

    struct GSL_FitData *p = (struct GSL_FitData *)params;

    if (x->data) p->function->updateActive(x->data);
    p->function->function (f->data);

    // function() return calculated data values. Need to convert this values into
    // calculated-observed devided by error values used by GSL

    for (size_t i = 0; i<p->n; i++)
      f->data[i] = 
      ( f->data[i] - p->Y[i] ) * p->sqrtWeightData[i];

    return GSL_SUCCESS;
  }

  /** Fit GSL derivative function wrapper
  * @param x Input function arguments
  * @param params Input data
  * @param J Output derivatives
  * @return A GSL status information
  */
  int gsl_df(const gsl_vector * x, void *params, gsl_matrix * J) {

    struct GSL_FitData *p = (struct GSL_FitData *)params;

    p->J.setJ(J);

    if (x->data) p->function->updateActive(x->data);
    p->function->functionDeriv (&p->J);

    // functionDeriv() return derivatives of calculated data values. Need to convert this values into
    // derivatives of calculated-observed devided by error values used by GSL

    for (size_t iY = 0; iY < p->n; iY++) 
      for (size_t iP = 0; iP < p->p; iP++) 
        J->data[iY*p->p + iP] *= p->sqrtWeightData[iY];

    return GSL_SUCCESS;
  }

  /** Fit derivatives and function GSL wrapper
  * @param x Input function arguments
  * @param params Input data
  * @param f Output function values = (y_cal-y_cal)/sigma for each data point
  * @param J Output derivatives
  * @return A GSL status information
  */
  int gsl_fdf(const gsl_vector * x, void *params,
    gsl_vector * f, gsl_matrix * J) {
      gsl_f(x, params, f);
      gsl_df(x, params, J);
      return GSL_SUCCESS;
  }



  /** Calculating least-squared cost function from fitting function
  *
  * @param x Input function arguments
  * @param params Input data
  * @return Value of least squared cost function
  */
  double gsl_costFunction(const gsl_vector * x, void *params)
  {

    struct GSL_FitData *p = (struct GSL_FitData *)params;
    double * l_holdCalculatedData = p->holdCalculatedData;

    // calculate yCal and store in l_holdCalculatedData
    if (x->data) p->function->updateActive(x->data);
    p->function->function (l_holdCalculatedData);

    return p->costFunc->val(p->Y, p->sqrtWeightData, l_holdCalculatedData, p->n);
  }

  /** Calculating derivatives of least-squared cost function
  *
  * @param x Input function arguments
  * @param params Input data
  * @param df Derivatives cost function
  */
  void gsl_costFunction_df(const gsl_vector * x, void *params, gsl_vector *df)
  {

    struct GSL_FitData *p = (struct GSL_FitData *)params;
    double * l_holdCalculatedData = p->holdCalculatedData;

    if (x->data) p->function->updateActive(x->data);
    p->function->function (l_holdCalculatedData);
    p->J.setJ(p->holdCalculatedJacobian);
    p->function->functionDeriv (&p->J);

    p->costFunc->deriv(p->Y, p->sqrtWeightData, l_holdCalculatedData, 
                     p->holdCalculatedJacobian->data, df->data, p->p, p->n);
  }

  /** Return both derivatives and function value of least-squared cost function. This function is
  *   required by the GSL none least squares multidimensional fitting framework
  *
  * @param x Input function arguments
  * @param params Input data
  * @param f cost function value
  * @param df Derivatives of cost function
  */
  void gsl_costFunction_fdf(const gsl_vector * x, void *params, double *f, gsl_vector *df)
  {
    *f = gsl_costFunction(x, params);
    gsl_costFunction_df(x, params, df); 
  }

  /**
   * Constructor. Creates declared -> active index map
   * @param f Pointer to the Fit algorithm
   */
  GSL_FitData::GSL_FitData(API::IFitFunction* fun):function(fun)
  {
    int j = 0;
    for(int i=0;i<fun->nParams();++i)
    {
      if (fun->isActive(i))
      {
        J.m_index.push_back(j);
        j++;
      }
      else
        J.m_index.push_back(-1);
    }
  }

} // namespace CurveFitting
} // namespace Mantid
