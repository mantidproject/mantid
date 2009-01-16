//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Gaussian.h"
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

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Gaussian)

using namespace Kernel;
using API::WorkspaceProperty;
using API::Axis;
using API::MatrixWorkspace_const_sptr;
using API::MatrixWorkspace;
using API::Algorithm;

// Get a reference to the logger
Logger& Gaussian::g_log = Logger::get("Gaussian");


/// Structure to contain least squares data
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
};

/** Gaussian function in GSL format
* @param x Input function arguments
* @param params Input data
* @param f Output function value
* @return A GSL status information
*/
static int gauss_f (const gsl_vector * x, void *params, gsl_vector * f) {
    size_t n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *Y = ((struct FitData *)params)->Y;
    double *sigmaData = ((struct FitData *)params)->sigmaData;
    double bg0 = gsl_vector_get (x, 0);
    double height = gsl_vector_get (x, 1);
    double peakCentre = gsl_vector_get (x, 2);
    double weight = gsl_vector_get (x, 3);
    size_t i;
    for (i = 0; i < n; i++) {
        double diff=X[i]-peakCentre;
        double Yi = height*exp(-0.5*diff*diff*weight)+bg0;
        gsl_vector_set (f, i, (Yi - Y[i])/sigmaData[i]);
    }
    return GSL_SUCCESS;
}

/** Calculates Gaussian derivatives in GSL format
* @param x Input function arguments
* @param params Input data
* @param J Output derivatives
* @return A GSL status information
*/
static int gauss_df (const gsl_vector * x, void *params,
              gsl_matrix * J)
{
    size_t n = ((struct FitData *)params)->n;
    double *X = ((struct FitData *)params)->X;
    double *sigmaData = ((struct FitData *)params)->sigmaData;
    double height = gsl_vector_get (x, 1);
    double peakCentre = gsl_vector_get (x, 2);
    double weight = gsl_vector_get (x, 3);
    size_t i;
    for (i = 0; i < n; i++) {
        double s = sigmaData[i];
        double diff = X[i]-peakCentre;
        double e = exp(-0.5*diff*diff*weight)/s;
        gsl_matrix_set (J, i, 0, 1/s);
        gsl_matrix_set (J, i, 1, e);
        gsl_matrix_set (J, i, 2, diff*height*e*weight);
        gsl_matrix_set (J, i, 3, -0.5*diff*diff*height*e);
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
static int gauss_fdf (const gsl_vector * x, void *params,
               gsl_vector * f, gsl_matrix * J) {
    gauss_f (x, params, f);
    gauss_df (x, params, J);
    return GSL_SUCCESS;
}

/// Initialisation method
void Gaussian::init()
{
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input));

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumIndex",0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("StartX",0.0);
  declareProperty("EndX",0.0);
  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  declareProperty("bg0",0.0, Direction::InOut);
  declareProperty("height",0.0, positiveDouble, "", Direction::InOut);
  declareProperty("peakCentre",0.0, Direction::InOut);
  declareProperty("sigma",1.0, positiveDouble->clone(), "", Direction::InOut);
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
void Gaussian::exec()
{
  // Try and retrieve the optional properties
  int histNumber = getProperty("SpectrumIndex");
  const int maxInterations = getProperty("MaxIterations");

  const double peak_val = getProperty("peakCentre");
  const double sigma = getProperty("sigma");

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
  const std::vector<double>& XValues = localworkspace->dataX(histNumber);
  const std::vector<double>& YValues = localworkspace->dataY(histNumber);
  const std::vector<double>& YErrors = localworkspace->dataE(histNumber);

  // Now get the range properties
  Property* start = getProperty("StartX");
  double startX;
  // If startX or endX has not been set, make it 6*sigma away from the centre point initial guess
  if ( ! start->isDefault() ) startX = getProperty("StartX");
  else startX = peak_val-(6*sigma);
  Property* end = getProperty("EndX");
  double endX;
  if ( ! end->isDefault() ) endX = getProperty("EndX");
  else endX = peak_val+(6*sigma);

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
  l_data.p = 4; // number of gaussian parameters to fit
  l_data.X = new double[l_data.n];
  l_data.Y = new double[l_data.n];
  l_data.sigmaData = new double[l_data.n];

  // Check sufficient bins are going into the fit (will crash otherwise)
  if ( l_data.n < l_data.p )
  {
    g_log.error() << "Insufficient bins (" << l_data.n << ") going into this fit" << std::endl;
    throw std::runtime_error("Insufficient bins selected (must be at least 4)");
  }

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


  // set-up initial guess for fit parameters

  gsl_vector *initFuncArg;
  initFuncArg = gsl_vector_alloc(l_data.p);

  gsl_vector_set(initFuncArg, 0, getProperty("bg0"));
	gsl_vector_set(initFuncArg, 1, getProperty("height"));
	gsl_vector_set(initFuncArg, 2, peak_val);
	gsl_vector_set(initFuncArg, 3, 1/(sigma*sigma));

  //guessInitialValues(l_data, initFuncArg);

  // set-up GSL least squares container

  gsl_multifit_function_fdf f;
  f.f = &gauss_f;
  f.df = &gauss_df;
  f.fdf = &gauss_fdf;
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

  // Output summary to log file

  double chi = gsl_blas_dnrm2(s->f);

  double dof = l_data.n - l_data.p;


  std::string reportOfFit = gsl_strerror(status);

  // GSL may return a successful fit with the weight = 1/sigma^2,
  // which is clearly not acceptable, hence handle this case separately

  if (status == GSL_SUCCESS && gsl_vector_get(s->x,3) <= 0.0)
  {
    reportOfFit = "failure: weight=1/sigma^2 fitted to a value <= 0.0";
  }


  g_log.information() << "Attempt to fit: bg0+height*exp(-0.5*((x-peakCentre)/sigma)^2)\n" <<
    "Iteration = " << iter << "\n" <<
    "Status = " << reportOfFit << "\n" <<
    "Chi^2/DoF = " << chi*chi / dof << "\n" <<
    "bg0 = " << std::setprecision(10) << gsl_vector_get(s->x,0) <<
    "; height = " << std::setprecision(10) << gsl_vector_get(s->x,1) <<
    "; peakCentre = " << std::setprecision(10) << gsl_vector_get(s->x,2) <<
    "; sigma = " << std::setprecision(10) << sqrt(1/gsl_vector_get(s->x,3)) << "\n";


  // also output summary to properties...

  setProperty("Output Status", reportOfFit);
  setProperty("Output Chi^2/DoF", chi*chi / dof);
  setProperty("bg0", gsl_vector_get(s->x,0));
  setProperty("height", gsl_vector_get(s->x,1));
  setProperty("peakCentre", gsl_vector_get(s->x,2));
  setProperty("sigma", sqrt(1/gsl_vector_get(s->x,3)));



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
