//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit1D.h"
#include <sstream>
#include <numeric>
#include <math.h>
#include <iomanip>
#include "MantidKernel/Exception.h"

#include <gsl/gsl_statistics.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_blas.h>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using API::WorkspaceProperty;
using API::Axis;
using API::MatrixWorkspace_const_sptr;
using API::MatrixWorkspace;
using API::Algorithm;

// Get a reference to the logger
Logger& Fit1D::g_log = Logger::get("Fit1D");


/// Structure to contain least squares data and used by GSL
struct FitData {
  /// number of points to be fitted (size of X, Y and sigmaData arrays)
  size_t n;
  /// number of fit parameters
  size_t p;
  /// the data to be fitted (abscissae)
  double * X;
  /// the data to be fitted (ordinates)
  const double * Y;
  /// the standard deviations of the Y data points
  const double * sigmaData;
  /// pointer to instance of Fit1D
  Fit1D* fit1D;
  /// Needed to use the simplex algorithm within the gsl least-squared framework.
  /// Will store output function values from gsl_f
  double * forSimplexLSwrap;
};

/** Fit1D GSL function wrapper
* @param x Input function arguments
* @param params Input data
* @param f Output function values = (y_cal-y_cal)/sigma for each data point
* @return A GSL status information
*/
static int gsl_f(const gsl_vector * x, void *params, gsl_vector * f) {

    ((struct FitData *)params)->fit1D->function (x->data, f->data,
                   ((struct FitData *)params)->X,
                   ((struct FitData *)params)->Y,
                   ((struct FitData *)params)->sigmaData,
                   ((struct FitData *)params)->n);


    return GSL_SUCCESS;
}

/** Fit1D GSL derivative function wrapper
* @param x Input function arguments
* @param params Input data
* @param J Output derivatives
* @return A GSL status information
*/
static int gsl_df(const gsl_vector * x, void *params, gsl_matrix * J) {

    ((struct FitData *)params)->fit1D->functionDeriv (x->data, J->data,
                   ((struct FitData *)params)->X,
                   ((struct FitData *)params)->Y,
                   ((struct FitData *)params)->sigmaData,
                   ((struct FitData *)params)->n);


    return GSL_SUCCESS;
}

/** Fit1D derivatives and function GSL wrapper
* @param x Input function arguments
* @param params Input data
* @param f Output function values = (y_cal-y_cal)/sigma for each data point
* @param J Output derivatives
* @return A GSL status information
*/
static int gsl_fdf(const gsl_vector * x, void *params,
               gsl_vector * f, gsl_matrix * J) {
    gsl_f(x, params, f);
    gsl_df(x, params, J);
    return GSL_SUCCESS;
}

/** Calculating the cost function assuming it has the least-squared
    format. Initially implemented to use the gsl simplex routine within
    the least-squared scheme.
* @param x Input function arguments
* @param params Input data
* @return Value of least squared cost function
*/
static double gsl_costFunction(const gsl_vector * x, void *params)
{
  double * l_forSimplexLSwrap = ((struct FitData *)params)->forSimplexLSwrap;

  ((struct FitData *)params)->fit1D->function (x->data,
                   l_forSimplexLSwrap,
                   ((struct FitData *)params)->X,
                   ((struct FitData *)params)->Y,
                   ((struct FitData *)params)->sigmaData,
                   ((struct FitData *)params)->n);

    double retVal = 0.0;

    for (unsigned int i = 0; i < ((struct FitData *)params)->n; i++)
      retVal += l_forSimplexLSwrap[i]*l_forSimplexLSwrap[i];


    return retVal;
}


/** Base class implementation of derivative function throws error. This is to check if such a function is provided
    by derivative class
* @param in Input fitting parameter values
* @param out Derivatives
* @param xValues X values for data points
* @param yValues Y values for data points
* @param yErrors Errors (standard deviations) on yValues
* @param nData Number of data points
 */
void Fit1D::functionDeriv(const double* in, double* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData)
{
  throw Exception::NotImplementedError("No derivative function provided");
}

/** Overload this function if the actual fitted parameters are different from 
    those the user specifies. Input is an Array of initial values of the fit 
    parameters as listed in declareParameters(). By default no changes is made
    to these. If this method is overloaded, the method modifyFinalFittedParameters()
    must also be overloaded.
* @param fittedParameter Values of fitting parameters in the order listed in declareParameters()
 */
void Fit1D::modifyInitialFittedParameters(std::vector<double>& fittedParameter) 
{}

/** If modifyInitialFittedParameters is overloaded this method must also be overloaded
    to reverse the effect of modifyInitialFittedParameters before outputting the results back to the user.
* @param fittedParameter Values of fitting parameters in the order listed in declareParameters()
 */
void Fit1D::modifyFinalFittedParameters(std::vector<double>& fittedParameter)
{}



/** Initialisation method
 */
void Fit1D::init()
{
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input), "Name of the input Workspace");

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumIndex",0, mustBePositive,
    "The spectrum to fit, uses the workspace numbering of the spectra (default 0)");
  declareProperty("StartX", EMPTY_DBL(),
    "A value of x in, or on the low x boundary of, the first bin to include in\n"
    "the fit (default lowest value of x)" );
  declareProperty("EndX", EMPTY_DBL(),
    "A value in, or on the high x boundary of, the last bin the fitting range\n"
    "(default the highest value of x)" );

  // declare parameters specific to a given fitting function
  declareParameters();

  // load the name of these specific parameter into a vector for later use
  const std::vector< Property* > props = getProperties();
  for ( size_t i = 4; i < props.size(); i++ )
  {
    m_parameterNames.push_back(props[i]->name());
  }

  declareProperty("MaxIterations", 500, mustBePositive->clone(),
    "Stop after this number of iterations if a good fit is not found" );
  declareProperty("Output Status","", Direction::Output);
  declareProperty("Output Chi^2/DoF",0.0, Direction::Output);

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();
}


/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void Fit1D::exec()
{
  // check if derivative defined in derived class

  bool isDerivDefined;
  double *inTest; 
  inTest = new double[m_parameterNames.size()];  // need to allocate this to avoid memory problem when calling functionDeriv below
  double outTest, xValuesTest = 0, yValuesTest = 1, yErrorsTest = 1;
  try
  {
    // note nData set to zero (last argument) hence this should avoid further memory problems
    functionDeriv(inTest, &outTest, &xValuesTest, &yValuesTest, &yErrorsTest, 0); 
  }
  catch (Exception::NotImplementedError e)
  {
    isDerivDefined = false;
  }
  delete [] inTest;

  // Try to retrieve optional properties
  int histNumber = getProperty("SpectrumIndex");
  const int maxInterations = getProperty("MaxIterations");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  // number of histogram is equal to the number of spectra
  const int numberOfSpectra = localworkspace->getNumberHistograms();
  // Check that the index given is valid
  if ( histNumber >= numberOfSpectra )
  {
    g_log.warning("Invalid spectrum index given, using first spectrum");
    histNumber = 0;
  }

  // Retrieve the spectrum into a vector
  const std::vector<double>& XValues = localworkspace->readX(histNumber);
  const std::vector<double>& YValues = localworkspace->readY(histNumber);
  const std::vector<double>& YErrors = localworkspace->readE(histNumber);

  //Read in the fitting range data that we were sent
  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  //check if the values had been set, otherwise use defaults
  if ( isEmpty( startX ) )
  {
    startX = XValues.front();
    modifyStartOfRange(startX); // does nothing by default but derived class may provide a more intelligent value
  }
  if ( isEmpty( endX ) )
  {
    endX = XValues.back();
    modifyEndOfRange(endX); // does nothing by default but derived class may previde a more intelligent value
  }

  int m_minX; // The X bin to start the fitting from
  int m_maxX; // The X bin to finish the fitting at

  // Check the validity of startX
  if ( startX < XValues.front() )
  {
    g_log.warning("StartX out of range! Set to start of frame.");
    startX = XValues.front();
  }
  // Get the corresponding bin boundary that comes before (or coincides with) this value
  for (m_minX = 0; XValues[m_minX+1] < startX; ++m_minX) {}

  // Check the validity of endX and get the bin boundary that come after (or coincides with) it
  if ( endX >= XValues.back() || endX < startX )
  {
    g_log.warning("EndX out of range! Set to end of frame");
    endX = XValues.back();
    m_maxX = YValues.size();
  }
  else
  {
    for (m_maxX = m_minX; XValues[m_maxX] < endX; ++m_maxX) {}
  }


  // create and populate GSL data container warn user if l_data.n < l_data.p 
  // since as a rule of thumb this is required as a minimum to obtained 'accurate'
  // fitting parameter values.

  FitData l_data;

  l_data.n = m_maxX - m_minX; // m_minX and m_maxX are array markers. I.e. e.g. 0 & 19.
  l_data.p = m_parameterNames.size();
  if (l_data.n < l_data.p)
    g_log.warning("Number of data points less than number of parameters to be fitted.");
  l_data.X = new double[l_data.n];
  l_data.fit1D = this;
  l_data.forSimplexLSwrap = new double[l_data.n];

  // check if histogram data in which case use mid points of histogram bins

  const bool isHistogram = localworkspace->isHistogramData();
  for (unsigned int i = 0; i < l_data.n; ++i)
  {
    if (isHistogram)
      l_data.X[i] = 0.5*(XValues[m_minX+i]+XValues[m_minX+i+1]); // take mid-point if histogram bin
    else
      l_data.X[i] = XValues[m_minX+i];
  }

  l_data.Y = &YValues[m_minX];
  l_data.sigmaData = &YErrors[m_minX];


  // create array of fitted parameter. Take these to those input by the user. However, for doing the
  // underlying fitting it might be more efficient to actually perform the fitting on some of other
  // form of the fitted parameters. For instance, take the Gaussian sigma parameter. In practice it
  // in fact more efficient to perform the fitting not on sigma but 1/sigma^2. The methods 
  // modifyInitialFittedParameters() and modifyFinalFittedParameters() are used to allow for this;
  // by default these function do nothing.

  for (size_t i = 0; i < l_data.p; i++)
  {
    m_fittedParameter.push_back(getProperty(m_parameterNames[i]));
  }
  modifyInitialFittedParameters(m_fittedParameter); // does nothing except if overwritten by derived class


  // set-up initial guess for fit parameters

  gsl_vector *initFuncArg;
  initFuncArg = gsl_vector_alloc(l_data.p);

  for (size_t i = 0; i < l_data.p; i++)
  {
    gsl_vector_set(initFuncArg, i, m_fittedParameter[i]);
  }


  // set-up GSL container to be used with GSL simplex algorithm

  gsl_multimin_function gslSimplexContainer;
  gslSimplexContainer.n = l_data.p;  // n here refers to number of parameters
  gslSimplexContainer.f = &gsl_costFunction;
  gslSimplexContainer.params = &l_data;


  // set-up GSL least squares container

  gsl_multifit_function_fdf f;
  f.f = &gsl_f;
  f.df = &gsl_df;
  f.fdf = &gsl_fdf;
  f.n = l_data.n;
  f.p = l_data.p;
  f.params = &l_data;

  // set-up remaining GSL machinery for least squared

  const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;
  gsl_multifit_fdfsolver *s;
  if (isDerivDefined)
  {
    s = gsl_multifit_fdfsolver_alloc(T, l_data.n, l_data.p);
    gsl_multifit_fdfsolver_set(s, &f, initFuncArg);
  }

  // set-up remaining GSL machinery to use simplex algorithm

  const gsl_multimin_fminimizer_type *simplexType = gsl_multimin_fminimizer_nmsimplex;
  gsl_multimin_fminimizer *simplexMinimizer;
  gsl_vector *simplexStepSize;
  if (!isDerivDefined)
  {
    simplexMinimizer = gsl_multimin_fminimizer_alloc(simplexType, l_data.p);
    simplexStepSize = gsl_vector_alloc(l_data.p);
    gsl_vector_set_all (simplexStepSize, 1.0);  // is this always a sensible starting step size?
    gsl_multimin_fminimizer_set(simplexMinimizer, &gslSimplexContainer, initFuncArg, simplexStepSize);
  }

  // finally do the fitting

  int iter = 0;
  int status;
  double size; // for simplex algorithm
  double finalCostFuncVal;
  double dof = l_data.n - l_data.p;  // dof stands for degrees of freedom

  // Standard least-squares used if derivative function defined otherwise simplex

  if (isDerivDefined)
  {
    do
    {
      iter++;
      status = gsl_multifit_fdfsolver_iterate(s);

      if (status)  // break if error
        break;

      status = gsl_multifit_test_delta(s->dx, s->x, 1e-4, 1e-4);
    }
    while (status == GSL_CONTINUE && iter < maxInterations);

    double chi = gsl_blas_dnrm2(s->f);
    finalCostFuncVal = chi*chi / dof;

    // put final converged fitting values back into m_fittedParameter
    for (unsigned int i = 0; i < l_data.p; i++)
      m_fittedParameter[i] = gsl_vector_get(s->x,i);
  }
  else
  {
    do
    {
      iter++;
      status = gsl_multimin_fminimizer_iterate(simplexMinimizer);

      if (status)  // break if error
        break;

      size = gsl_multimin_fminimizer_size(simplexMinimizer);
      status = gsl_multimin_test_size(size, 1e-2);
    }
    while (status == GSL_CONTINUE && iter < maxInterations);

    finalCostFuncVal = simplexMinimizer->fval / dof;

    // put final converged fitting values back into m_fittedParameter
    for (unsigned int i = 0; i < l_data.p; i++)
      m_fittedParameter[i] = gsl_vector_get(simplexMinimizer->x,i);
  }

  modifyFinalFittedParameters(m_fittedParameter);   // do nothing except if overwritten by derived class


  // Output summary to log file

  std::string reportOfFit = gsl_strerror(status);

  g_log.information() << "Iteration = " << iter << "\n" <<
    "Status = " << reportOfFit << "\n" <<
    "Chi^2/DoF = " << finalCostFuncVal << "\n";
  for (size_t i = 0; i < l_data.p; i++)
    g_log.information() << m_parameterNames[i] << " = " << m_fittedParameter[i] << "  ";


  // also output summary to properties

  setProperty("Output Status", reportOfFit);
  setProperty("Output Chi^2/DoF", finalCostFuncVal);
  for (size_t i = 0; i < l_data.p; i++)
    setProperty(m_parameterNames[i], m_fittedParameter[i]);


  // clean up dynamically allocated gsl stuff

  delete [] l_data.X;
  //delete [] l_data.Y;
  //delete [] l_data.sigmaData;
  delete [] l_data.forSimplexLSwrap;

  gsl_vector_free(initFuncArg);

  if (isDerivDefined)
    gsl_multifit_fdfsolver_free(s);
  else
  {
    gsl_vector_free(simplexStepSize);
    gsl_multimin_fminimizer_free(simplexMinimizer);
  }

  return;
}

double Fit1D::getFittedParam(const unsigned int i) const
{
	if (i>=m_fittedParameter.size())
		throw std::range_error("Fitted parameter out of range");
	return m_fittedParameter[i];
}


} // namespace Algorithm
} // namespace Mantid
