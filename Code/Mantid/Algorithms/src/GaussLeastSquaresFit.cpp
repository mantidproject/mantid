//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GaussLeastSquaresFit.h"
#include "MantidDataObjects/Workspace2D.h"
//#include "MantidAPI/SpectraDetectorMap.h"
#include <sstream>
#include <numeric>
#include <math.h>

#include <gsl/gsl_statistics.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_blas.h>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(GaussLeastSquaresFit)

using namespace Kernel;
using API::WorkspaceProperty;
using API::Axis;
using DataObjects::Workspace2D_sptr;
using DataObjects::Workspace2D;

// Get a reference to the logger
Logger& GaussLeastSquaresFit::g_log = Logger::get("GaussLeastSquaresFit");



//using GSL::FitData;

/// Initialisation method
void GaussLeastSquaresFit::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input));
  //declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace","",Direction::Output));
  
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumNumber",0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("StartX",0, mustBePositive->clone());
  declareProperty("EndX",0, mustBePositive->clone());  
  declareProperty("MaxIterations",500, mustBePositive->clone()); 
}

/** Executes the algorithm
 * 
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void GaussLeastSquaresFit::exec()
{
  // Try and retrieve the optional properties
  m_spectrumNumber = getProperty("SpectrumNumber");
  m_minX = getProperty("StartX");
  m_maxX = getProperty("EndX");
  int maxInterations = getProperty("MaxIterations");


  // Get the input workspace
  Workspace2D_sptr localworkspace = getProperty("InputWorkspace");
  
  // number of histogram is equal to the number of spectra
  const int numberOfSpectra = localworkspace->getNumberHistograms(); 

  // Get the histogram number corresponding to the user specified spectrum number

  Axis *spectraAxis = localworkspace->getAxis(1); // Get axis that holds the spectrum numbers

  int histNumber = -1; // set to -1 here to test after the loop below whether it has be set
  for (int i = 0; i < numberOfSpectra; ++i)
  {
    if ( spectraAxis->spectraNo(i) == m_spectrumNumber )
      histNumber = i;
  }  

  if ( histNumber == -1 )
    histNumber = 0;

 
  // Retrieve the spectrum into a vector
  const std::vector<double>& XValues = localworkspace->dataX(histNumber);
  const std::vector<double>& YValues = localworkspace->dataY(histNumber);
  const std::vector<double>& YErrors = localworkspace->dataE(histNumber);

  const int numberOfXBins = XValues.size();
  if ( (m_minX < 0) || (m_minX >= numberOfXBins))
  {
    g_log.information("StartX out of range! Set to 0");
    m_minX = 0;
  }

  if ( m_maxX == 0 ) // if zero assumed that no value has been specified......
  {
    m_maxX = numberOfXBins - 1;  // -1 since we are counting from 0
  }

  if ( m_maxX >= numberOfXBins || m_maxX < m_minX)
  {
     g_log.information("EndX out of range! Set to max number");
     m_maxX = numberOfXBins - 1; // -1 since we are counting from 0
  }
    
  // create and populate GSL data container

  FitData l_data;

  l_data.n = m_maxX - m_minX; 
  l_data.p = 4; // number of gaussian parameters to fit 
  l_data.X = new double[l_data.n];
  l_data.Y = new double[l_data.n];
  l_data.sigma = new double[l_data.n];

  for (unsigned int i = 0; i < l_data.n; i++)
  {
    l_data.X[i] = XValues[m_minX+i];
    l_data.Y[i] = YValues[m_minX+i];
    l_data.sigma[i] = YErrors[m_minX+i];
  }

  // set-up initial guess for fit parameters

  gsl_vector *initFuncArg; 
  initFuncArg = gsl_vector_alloc(l_data.p);

  guessInitialValues(l_data, initFuncArg);

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

  size_t iter = 0;
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

  g_log.information() << "Attempt to fit: y0+A*sqrt(2/PI)/w*exp(-2*((x-xc)/w)^2)\n" <<
    "Iteration = " << iter << "\n" <<
    "Status = " << gsl_strerror(status) << "\n" <<
    "Chi^2/DoF = " << chi*chi / dof << "\n" <<
    "y0 = " << gsl_vector_get(s->x,0) << "; A = " << gsl_vector_get(s->x,1) <<
    "; w = " << gsl_vector_get(s->x,2) << "; xc = " << gsl_vector_get(s->x,3) << "\n";


  // clean up dynamically allocated gsl stuff

  delete [] l_data.X;
  delete [] l_data.Y;
  delete [] l_data.sigma;

  gsl_vector_free(initFuncArg);
  gsl_multifit_fdfsolver_free(s);

  
  return;  
}

/** Method which guesses initial parameter values
* @param data The data to be fitted against  
* @param param_init Output parameter values
*/
void GaussLeastSquaresFit::guessInitialValues(const FitData& data, gsl_vector* param_init)
{
	size_t imin, imax;
	gsl_stats_minmax_index(&imin, &imax, data.Y, 1, data.n);

	double min_out = data.Y[imin];
	double max_out = data.Y[imax];

  size_t imax_temp;
  {
    double* temp = new double[data.n];
 
  	for (int i = 0; i < data.n; i++)
	  	temp[i] = fabs(data.Y[i]);

	  imax_temp = gsl_stats_max_index(temp, 1, data.n);  
    delete [] temp;
  }

	double offset, area;
	if (imax_temp == imax)
		offset = min_out;
	else //reversed bell
		offset = max_out;

	double xc = data.X[imax_temp];
	double width = 2.0 * gsl_stats_sd(data.X, 1, data.n);

  double pi_div_2 = 1.57079632679489661923;
  area = sqrt(pi_div_2)*width*fabs(max_out - min_out);

	gsl_vector_set(param_init, 0, area);
	gsl_vector_set(param_init, 1, xc);
	gsl_vector_set(param_init, 2, width);
	gsl_vector_set(param_init, 3, offset);
}



} // namespace Algorithm
} // namespace Mantid
