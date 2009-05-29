//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit1D.h"
#include <sstream>
#include <numeric>
#include <math.h>
#include <iomanip>

#include <gsl/gsl_statistics.h>
#include <gsl/gsl_multifit_nlin.h>
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
  double * Y;
  /// the standard deviations of the Y data points
  double * sigmaData;
  /// pointer to instance of Fit1D
  Fit1D* fit1D;
};

/** Fit1D GSL function wrapper
* @param x Input function arguments
* @param params Input data
* @param f Output function value
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
* @param f Output function value
* @param J Output derivatives
* @return A GSL status information
*/
static int gsl_fdf(const gsl_vector * x, void *params,
               gsl_vector * f, gsl_matrix * J) {
    gsl_f(x, params, f);
    gsl_df(x, params, J);
    return GSL_SUCCESS;
}


/** Initialisation method
 */
void Fit1D::init()
{
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input), "Name of the input Workspace");

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumIndex",0, mustBePositive,
    "The spectrum to fit, using the workspace numbering of the spectra (default 0)");
  declareProperty("StartX", EMPTY_DBL(), "X value to start fitting from");
  declareProperty("EndX", EMPTY_DBL(), "last X value to include in fitting range");

  // declare parameters specific to a given fitting function
  declareParameters();

  // load the name of these specific parameter into a vector for later use
  const std::vector< Property* > props = getProperties();
  for ( size_t i = 4; i < props.size(); i++ )
  {
    m_parameterNames.push_back(props[i]->name());
  }

  declareProperty("MaxIterations",500, mustBePositive->clone(),
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

  // Set the fitting range
  double startX = getProperty("StartX");
  double endX = getProperty("EndX"); 

  if ( isEmpty( startX ) )
  {//the value hasn't been set by the user, use defaults
    startX = XValues.front();
    modifyStartOfRange(startX); // does nothing by default but derived class may provide a more intelligent value
  }
  else//the user has entered a value, let's use it
    startX = getProperty("StartX");  

  if ( isEmpty( endX ) ) 
  {//load defaults values
    endX = XValues.back();
    modifyEndOfRange(endX); // does nothing by default but derived class may previde a more intelligent value
  }
  else//use wht the use entered
    endX = getProperty("EndX");

  
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



  // create and populate GSL data container

  FitData l_data;

  l_data.n = m_maxX - m_minX; // m_minX and m_maxX are array markers. I.e. e.g. 0 & 19.
                              // The data includes both of these array elements
  l_data.p = m_parameterNames.size(); 
  l_data.X = new double[l_data.n];
  l_data.Y = new double[l_data.n];
  l_data.sigmaData = new double[l_data.n];
  l_data.fit1D = this;

  // check if histogram data in which case the mid points of X values will be used further below
  const bool isHistogram = localworkspace->isHistogramData();

  for (unsigned int i = 0; i < l_data.n; ++i)
  {
    if (isHistogram)
      l_data.X[i] = 0.5*(XValues[m_minX+i]+XValues[m_minX+i+1]); // take mid-point if histogram data
    else
      l_data.X[i] = XValues[m_minX+i];

    l_data.Y[i] = YValues[m_minX+i];
    l_data.sigmaData[i] = YErrors[m_minX+i];
  }


  for (int i = 0; i < l_data.p; i++)
  {
    m_fittedParameter.push_back(getProperty(m_parameterNames[i]));
  }
  modifyInitialFittedParameters(m_fittedParameter); // do nothing except if overwritten by derived class


  // set-up initial guess for fit parameters

  gsl_vector *initFuncArg;
  initFuncArg = gsl_vector_alloc(l_data.p);

  for (int i = 0; i < l_data.p; i++)
  {
    gsl_vector_set(initFuncArg, i, m_fittedParameter[i]);
  }


  // set-up GSL least squares container

  gsl_multifit_function_fdf f;
  f.f = &gsl_f;
  f.df = &gsl_df;
  f.fdf = &gsl_fdf;
  f.n = l_data.n;
  f.p = l_data.p;
  f.params = &l_data;

  // set-up remaining GSL machinery

  const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;
  gsl_multifit_fdfsolver *s = gsl_multifit_fdfsolver_alloc(T, l_data.n, l_data.p);
  gsl_multifit_fdfsolver_set(s, &f, initFuncArg);


  // finally do the fitting

  int iter = 0;
  int status;
  do
  {
    iter++;
    status = gsl_multifit_fdfsolver_iterate(s);

    if (status)  // break if error
      break;

    status = gsl_multifit_test_delta(s->dx, s->x, 1e-4, 1e-4);
  }
  while (status == GSL_CONTINUE && iter < maxInterations);

  gsl_matrix *covar = gsl_matrix_alloc(l_data.p, l_data.p);
  gsl_multifit_covar(s->J, 0.0, covar);


  // put final converged fitting values back into m_fittedParameter

  for (int i = 0; i < l_data.p; i++)
  {
    m_fittedParameter[i] = gsl_vector_get(s->x,i);
  }
  modifyFinalFittedParameters(m_fittedParameter); // do nothing except if overwritten by derived class


  // Output summary to log file

  double chi = gsl_blas_dnrm2(s->f);
  double dof = l_data.n - l_data.p;

  std::string reportOfFit = gsl_strerror(status);

  g_log.information() << "Attempt to fit: bg0+height*exp(-0.5*((x-peakCentre)/sigma)^2)\n" <<
    "Iteration = " << iter << "\n" <<
    "Status = " << reportOfFit << "\n" <<
    "Chi^2/DoF = " << chi*chi / dof << "\n";
  for (int i = 0; i < l_data.p; i++)
    g_log.information() << m_parameterNames[i] << " = " << m_fittedParameter[i] << "  ";
  

  // also output summary to properties

  setProperty("Output Status", reportOfFit);
  setProperty("Output Chi^2/DoF", chi*chi / dof);
  for (int i = 0; i < l_data.p; i++)
    setProperty(m_parameterNames[i], m_fittedParameter[i]);


  // clean up dynamically allocated gsl stuff

  delete [] l_data.X;
  delete [] l_data.Y;
  delete [] l_data.sigmaData;

  gsl_vector_free(initFuncArg);
  gsl_multifit_fdfsolver_free(s);


  return;
}






} // namespace Algorithm
} // namespace Mantid
