//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BackToBackExponential.h"
#include <sstream>
#include <numeric>
#include <math.h>

#include <gsl/gsl_statistics.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_sf_erf.h>

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(BackToBackExponential)

using namespace Kernel;
using API::WorkspaceProperty;
using API::MatrixWorkspace_const_sptr;
using API::MatrixWorkspace;


// Get a reference to the logger
Logger& BackToBackExponential::g_log = Logger::get("BackToBackExponential");


/// Structure to contain least squares data
struct FitData {
  /// number of points to be fitted (size of X, Y and sigma arrays)
  size_t n;
  /// number of fit parameters
  size_t p;
  /// the data to be fitted (abscissae)
  double * X;
  /// the data to be fitted (ordinates)
  double * Y;
  /// the weighting data
  double * sigma;
};


/// Initialisation method
void BackToBackExponential::init()
{
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input));

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumIndex",0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("StartX",0.0);
  declareProperty("EndX",1.0);
  declareProperty("I",0.0, Direction::InOut);
  declareProperty("a",0.0, Direction::InOut);
  declareProperty("b",0.0, Direction::InOut);
  declareProperty("x0",0.0, Direction::InOut);
  declareProperty("s",1.0, Direction::InOut);
  declareProperty("bk",0.0, Direction::InOut);
  declareProperty("MaxIterations",500, mustBePositive->clone());
  declareProperty("Output Status","", Direction::Output);
  declareProperty("Output Chi^2/DoF",0.0, Direction::Output);

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void BackToBackExponential::exec()
{
  // Try and retrieve the optional properties
  int histNumber = getProperty("SpectrumIndex");
//  m_minX = getProperty("StartX");
//  m_maxX = getProperty("EndX");
  const int maxInterations = getProperty("MaxIterations");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  // number of histogram is equal to the number of spectra
  const int numberOfSpectra = localworkspace->getNumberHistograms();

  if ( histNumber >= numberOfSpectra )
  {
    g_log.warning("Invalid spectrum index given, using first spectrum");
    histNumber = 0;
  }

  // Retrieve the spectrum into a vector
  const std::vector<double>& XValues = localworkspace->dataX(histNumber);
  const std::vector<double>& YValues = localworkspace->dataY(histNumber);
  const std::vector<double>& YErrors = localworkspace->dataE(histNumber);

//  const int numberOfXBins = YValues.size(); // not cannot ask for size of XValues here
//                                            // since for histogram it is one bigger than number of data
//  const int sizeX = XValues.size();
//
//  // check if histogram data in which case the midt points of X values will be used further below
//  bool isHistogram = false;
//  if (sizeX == numberOfXBins + 1 )
//    isHistogram = true;

  // Now get the range properties
  Property* start = getProperty("StartX");
  double startX;
  // If startX or endX has not been set, make it the fram limit
  if ( ! start->isDefault() ) startX = getProperty("StartX");
  else startX = XValues.front();
  Property* end = getProperty("EndX");
  double endX;
  if ( ! end->isDefault() ) endX = getProperty("EndX");
  else endX = XValues.back();

  // Check the validity of startX
  if ( startX < XValues.front() )
  {
    g_log.warning("StartX out of range! Set to start of frame.");
    startX = XValues.front();
  }
  // Now get the corresponding bin boundary that comes before (or coincides with) this value
  for (m_minX = 0; XValues[m_minX+1] < startX; ++m_minX) {}

  // Check the validity of endX and get the bin boundary that come after (or coincides with) it
  if ( endX >= XValues.back() || endX < startX )
  {
    g_log.warning("EndX out of range! Set to end of frame");
    endX = XValues.back();
    m_maxX = YValues.size()-1;
  }
  else
  {
    for (m_maxX = m_minX; XValues[m_maxX] < endX; ++m_maxX) {}
  }


  // create and populate GSL data container

  FitData l_data;

  l_data.n = m_maxX - m_minX + 1; // m_minX and m_maxX are array markers. I.e. e.g. 0 & 19.
                                  // The data includes both of these array elements hence the reason for the +1
  l_data.p = 6; // number of gaussian parameters to fit
  l_data.X = new double[l_data.n];
  l_data.Y = new double[l_data.n];
  l_data.sigma = new double[l_data.n];

  // check if histogram data in which case the mid points of X values will be used further below
  const bool isHistogram = localworkspace->isHistogramData();

  for (unsigned int i = 0; i < l_data.n; ++i)
  {
    if (isHistogram)
      l_data.X[i] = 0.5*(XValues[m_minX+i]+XValues[m_minX+i+1]); // take midt point if histogram data
    else
      l_data.X[i] = XValues[m_minX+i];

    l_data.Y[i] = YValues[m_minX+i];
    l_data.sigma[i] = YErrors[m_minX+i];
  }

  // set-up initial guess for fit parameters

  gsl_vector *initFuncArg;
  initFuncArg = gsl_vector_alloc(l_data.p);

	gsl_vector_set(initFuncArg, 0, getProperty("I"));
	gsl_vector_set(initFuncArg, 1, getProperty("a"));
	gsl_vector_set(initFuncArg, 2, getProperty("b"));
	gsl_vector_set(initFuncArg, 3, getProperty("x0"));
	gsl_vector_set(initFuncArg, 4, getProperty("s"));
	gsl_vector_set(initFuncArg, 5, getProperty("bk"));


  // set-up GSL least squares container

  gsl_multifit_function_fdf f;
  f.f = &bTbExpo_f;
  f.df = &bTbExpo_df;
  f.fdf = &bTbExpo_fdf;
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

  //gsl_multifit_covar(s->J, 0.0, covar);

  // Output summary to log file

  double chi = gsl_blas_dnrm2(s->f);
  double dof = l_data.n - l_data.p;

  std::string fisse = gsl_strerror(status);

  g_log.information() << "Attempt to fit: I*(exp(a/2*(a*s^2+2*(x-c)))*erfc((a*s^2+(x-c))/sqrt(2*s^2))+exp(b/2*(b*s^2-2*(x-c)))*erfc((b*s^2-(x-c))/sqrt(s*s^2)))+bk" << "\n" <<
    "Iteration = " << iter << "\n" <<
    "Status = " << gsl_strerror(status) << "\n" <<
    "Chi^2/DoF = " << chi*chi / dof << "\n" <<
    "I = " << gsl_vector_get(s->x,0) << "; a = " << gsl_vector_get(s->x,1) <<
    "; b = " << gsl_vector_get(s->x,2) << "; x0 = " << gsl_vector_get(s->x,3) <<
    "; s = " << gsl_vector_get(s->x,4) << "; bk = " << gsl_vector_get(s->x,5) << "\n";


  // also output summary to properties...

  setProperty("Output Status", fisse);
  setProperty("Output Chi^2/DoF", chi*chi / dof);
  setProperty("I", gsl_vector_get(s->x,0));
  setProperty("a", gsl_vector_get(s->x,1));
  setProperty("b", gsl_vector_get(s->x,2));
  setProperty("x0", gsl_vector_get(s->x,3));
  setProperty("s", gsl_vector_get(s->x,4));
  setProperty("bk", gsl_vector_get(s->x,5));

  // clean up dynamically allocated gsl stuff

  delete [] l_data.X;
  delete [] l_data.Y;
  delete [] l_data.sigma;

  gsl_vector_free(initFuncArg);
  gsl_multifit_fdfsolver_free(s);


  return;
}


/** Gaussian function in GSL format
* @param x Input function arguments
* @param params Input data
* @param f Output function value
* @return A GSL status information
*/
int bTbExpo_f (const gsl_vector * x, void *params, gsl_vector * f) {
    size_t n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigma = ((struct FitData *)params)->sigma;
    double I = gsl_vector_get (x, 0);
    double a = gsl_vector_get (x, 1);
    double b = gsl_vector_get (x, 2);
    double x0 = gsl_vector_get (x, 3);
    double s = gsl_vector_get (x, 4);
    double bk = gsl_vector_get (x, 5);

    size_t i;

    double s2 = s*s;
    for (i = 0; i < n; i++) {
      double diff=X[i]-x0;
      double Yi = I*(exp(a/2*(a*s2+2*diff))*gsl_sf_erfc((a*s2+diff)/sqrt(2*s2))
                    + exp(b/2*(b*s2-2*diff))*gsl_sf_erfc((b*s2-diff)/sqrt(2*s2)))+bk;
      gsl_vector_set (f, i, (Yi - Y[i])/sigma[i]);
    }
    return GSL_SUCCESS;
}

/** Calculates Gaussian derivatives in GSL format
* @param x Input function arguments
* @param params Input data
* @param J Output derivatives
* @return A GSL status information
*/
int bTbExpo_df (const gsl_vector * x, void *params,
              gsl_matrix * J)
{
    size_t n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *sigma = ((struct FitData *)params)->sigma;
    double I = gsl_vector_get (x, 0);
    double a = gsl_vector_get (x, 1);
    double b = gsl_vector_get (x, 2);
    double x0 = gsl_vector_get (x, 3);
    double s = gsl_vector_get (x, 4);
    size_t i;


    double s2 = s*s;
    for (i = 0; i < n; i++) {

        double diff = X[i]-x0;

        double e_a = exp(0.5*a*(a*s2+2*diff));
        double e_b = exp(0.5*b*(b*s2-2*diff));
        double erfc_a = gsl_sf_erfc((a*s2+diff)/sqrt(2*s2));
        double erfc_b = gsl_sf_erfc((b*s2-diff)/sqrt(2*s2));

        // apart from a prefactor terms arising from defivative or argument of erfc's divided by sqrt(2)
        double div_erfc_a = - exp( -(a*s2+diff)*(a*s2+diff)/(2*s2)+0.5*a*(a*s2+2.0*diff) ) * M_SQRT2/M_SQRTPI;
        double div_erfc_b = - exp( -(b*s2-diff)*(b*s2-diff)/(2*s2)+0.5*b*(b*s2-2.0*diff) ) * M_SQRT2/M_SQRTPI;


        gsl_matrix_set (J, i, 0, (e_a*erfc_a+e_b*erfc_b)/sigma[i]);    // deriv I
        gsl_matrix_set (J, i, 1,                                       // deriv a
          I*( s*div_erfc_a + e_a*(a*s2+diff)*erfc_a )/sigma[i]);
        gsl_matrix_set (J, i, 2,                                       // deriv b
          I*( s*div_erfc_b + e_b*(b*s2-diff)*erfc_b )/sigma[i]);
        gsl_matrix_set (J, i, 3,                                                    // deriv x0
          I*( (-div_erfc_a+div_erfc_b)/s + b*e_b*erfc_b - a*e_a*erfc_a )/sigma[i]);
        gsl_matrix_set (J, i, 4,                                                    // deriv s
          I*( div_erfc_b*(b+diff/s2)+div_erfc_a*(a-diff/s2)
              + b*b*e_b*s*erfc_b + a*a*e_a*s*erfc_a )/sigma[i]);
        gsl_matrix_set (J, i, 5, 1/sigma[i]);                                       // deriv bk
    }
    return GSL_SUCCESS;
}

/** Calculates Gaussian derivatives and function value in GSL format
* @param x Input function arguments
* @param params Input data
* @param f Output function value
* @param J Output derivatives
* @return A GSL status information
*/
int bTbExpo_fdf (const gsl_vector * x, void *params,
               gsl_vector * f, gsl_matrix * J) {
    bTbExpo_f (x, params, f);
    bTbExpo_df (x, params, J);
    return GSL_SUCCESS;
}



} // namespace Algorithm
} // namespace Mantid
