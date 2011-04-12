//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Linear.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fit.h>

namespace Mantid
{
namespace CurveFitting
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Linear)

/// Sets documentation strings for this algorithm
void Linear::initDocs()
{
  this->setWikiSummary("Performs linear least-squares regression on a spectrum (or portion of one). ");
  this->setOptionalMessage("Performs linear least-squares regression on a spectrum (or portion of one).");
}


using namespace Mantid::Kernel;
using namespace Mantid::API;


Linear::Linear() : API::Algorithm(), m_minX(0), m_maxX(0), m_progress(NULL)
{}

Linear::~Linear() {if(m_progress) delete m_progress;m_progress=NULL;}

void Linear::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
    "Workspace with the spectrum to fit");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "Name of the workspace that will contain the result");

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex",0, mustBePositive,
    "Index number of the Workspace to fit");
  declareProperty("StartX", EMPTY_DBL(),
    "An X value in the first bin to include in the fit (default\n"
    "lowest value of X)");
  declareProperty("EndX", EMPTY_DBL(),
    "An X value in the last bin to be included in the range\n"
    "(default the high X value");
  declareProperty("FitStatus", "", new NullValidator<std::string>,
    "Empty if the fit succeeded, otherwise contains the gsl error\n"
    "message", Direction::Output);
  declareProperty("FitIntercept", 0.0,
    "The intercept with the ordinate of the fitted line. c0 in the\n"
    "equation below", Direction::Output);
  declareProperty("FitSlope",0.0,
    "The slope of the fitted line. c1 in the equation below",
    Direction::Output);
  declareProperty("Chi2",0.0,
    "The goodness of the fit", Direction::Output);

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();
}

void Linear::exec()
{
  // Get the input workspace
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  // Get the spectrum to fit
  const int histNumber = getProperty("WorkspaceIndex");
  // Check validity
  if ( histNumber >= inputWorkspace->getNumberHistograms() )
  {
    g_log.error() << "WorkspaceIndex set to an invalid value of " << histNumber << std::endl;
    throw Exception::IndexError(histNumber,inputWorkspace->getNumberHistograms(),"Linear WorkspaceIndex property");
  }

  // Get references to the data in the chosen spectrum
  const MantidVec& X = inputWorkspace->dataX(histNumber);
  const MantidVec& Y = inputWorkspace->dataY(histNumber);
  const MantidVec& E = inputWorkspace->dataE(histNumber);
  // Check if this spectrum has errors
  double errorsCount = 0.0;

  // Retrieve the Start/EndX properties, if set
  this->setRange(X,Y);
  
  const bool isHistogram = inputWorkspace->isHistogramData();
  // If the spectrum to be fitted has masked bins, we want to exclude them (even if only partially masked)
  const MatrixWorkspace::MaskList * const maskedBins = 
    ( inputWorkspace->hasMaskedBins(histNumber) ? &(inputWorkspace->maskedBins(histNumber)) : NULL );
  // Put indices of masked bins into a set for easy searching later
  std::set<int> maskedIndices;
  if (maskedBins)
  {
    MatrixWorkspace::MaskList::const_iterator it;
    for (it = maskedBins->begin(); it != maskedBins->end(); ++it)
      maskedIndices.insert(it->first);
  }
  
  progress(0);

  // Declare temporary vectors and reserve enough space if they're going to be used
  std::vector<double> XCen, unmaskedY, weights;
  int numPoints = m_maxX - m_minX;
  if (isHistogram) XCen.reserve(numPoints);
  if (maskedBins) unmaskedY.reserve(numPoints);
  weights.reserve(numPoints);

  for (int i = 0; i < numPoints; ++i)
  {
    // If the current bin is masked, skip it
    if ( maskedBins && maskedIndices.count(m_minX+i) ) continue;
    // Need to adjust X to centre of bin, if a histogram
    if (isHistogram) XCen.push_back( 0.5*(X[m_minX+i]+X[m_minX+i+1]) );
    // If there are masked bins present, we need to copy the unmasked Y values
    if (maskedBins) unmaskedY.push_back(Y[m_minX+i]);
    // GSL wants the errors as weights, i.e. 1/sigma^2
    // We need to be careful if E is zero because that would naively lead to an infinite weight on the point.
    // Solution taken here is to zero weight if error is zero, which typically means Y is zero
    //   (so it is effectively excluded from the fit).
    const double& currentE = E[m_minX+i];
    weights.push_back( currentE ? 1.0/(currentE*currentE) : 0.0 );
    // However, if the spectrum given has all errors of zero, then we should use the gsl function that
    //   doesn't take account of the errors.
    if ( currentE ) ++errorsCount;
  }
  progress(0.3);
  
  // If masked bins present, need to recalculate numPoints here
  if (maskedBins) numPoints = unmaskedY.size();
  // If no points left for any reason, bail out
  if (numPoints == 0)
  {
    g_log.error("No points in this range to fit");
    throw std::runtime_error("No points in this range to fit");
  }

  // Set up pointer variables to pass to gsl, pointing them to the right place
  const double * const xVals = ( isHistogram ? &XCen[0] : &X[m_minX] );
  const double * const yVals = ( maskedBins ? &unmaskedY[0] : &Y[m_minX] );

  // Call the gsl fitting function
  // The stride value of 1 reflects that fact that we want every element of our input vectors
  const int stride = 1;
  double *c0(new double),*c1(new double),*cov00(new double),*cov01(new double),*cov11(new double),*chisq(new double);
  int status;
  // Unless our spectrum has error values for vast majority of points, 
  //   call the gsl function that doesn't use errors
  if ( errorsCount/numPoints < 0.9 )
  {
    g_log.debug("Calling gsl_fit_linear (doesn't use errors in fit)");
    status = gsl_fit_linear(xVals,stride,yVals,stride,numPoints,c0,c1,cov00,cov01,cov11,chisq);
  }
  // Otherwise, call the one that does account for errors on the data points
  else
  {
    g_log.debug("Calling gsl_fit_wlinear (uses errors in fit)");
    status = gsl_fit_wlinear(xVals,stride,&weights[0],stride,yVals,stride,numPoints,c0,c1,cov00,cov01,cov11,chisq);
  }
  progress(0.8);

  // Check that the fit succeeded
  std::string fitStatus = gsl_strerror(status);
  // For some reason, a fit where c0,c1 & chisq are all infinity doesn't report as a 
  //   failure, so check explicitly.
  if ( !gsl_finite(*chisq) || !gsl_finite(*c0) || !gsl_finite(*c1) )
    fitStatus = "Fit gives infinities";
  if (fitStatus != "success") g_log.error() << "The fit failed: " << fitStatus << "\n";
  else
    g_log.information() << "The fit succeeded, giving y = " << *c0 << " + " << *c1 << "*x, with a Chi^2 of " << *chisq << "\n";
  
  // Set the fit result output properties
  setProperty("FitStatus",fitStatus);
  setProperty("FitIntercept",*c0);
  setProperty("FitSlope",*c1);
  setProperty("Chi2",*chisq);
  
  // Create and fill a workspace2D with the same bins as the fitted spectrum and the value of the fit for the centre of each bin
  const int YSize = Y.size();
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,1,X.size(),YSize);
  
  // Copy over the X bins
  outputWorkspace->dataX(0).assign(X.begin(),X.end());
  // Now loop over the spectrum and use gsl function to calculate the Y & E values for the function
  for (int i = 0; i < YSize; ++i)
  {
    const double x = ( isHistogram ? 0.5*(X[i]+X[i+1]) : X[i] );
    const int err = gsl_fit_linear_est(x,*c0,*c1,*cov00,*cov01,*cov11,&(outputWorkspace->dataY(0)[i]),&(outputWorkspace->dataE(0)[i]));
    if (err) g_log.warning() << "Problem in filling the output workspace: " << gsl_strerror(err) << std::endl;
  }
  setProperty("OutputWorkspace",outputWorkspace);
  progress(1);
  // Clean up
  delete c0;
  delete c1;
  delete cov00;
  delete cov01;
  delete cov11;
  delete chisq;
}

/// Retrieve and check the Start/EndX parameters, if set
void Linear::setRange(const MantidVec& X, const MantidVec& Y)
{
  //read in the values that the user selected
  double startX = getProperty("StartX");
  double endX = getProperty("EndX");
  //If the user didn't a start default to the start of the data
  if ( isEmpty(startX) ) startX = X.front();
  //the default for the end is the end of the data
  if ( isEmpty(endX) ) endX = X.back();

  // Check the validity of startX
  if ( startX < X.front() )
  {
    g_log.warning("StartX out of range! Set to start of frame.");
    startX = X.front();
  }
  // Now get the corresponding bin boundary that comes before (or coincides with) this value
  for (m_minX = 0; X[m_minX+1] < startX; ++m_minX) {}

  // Check the validity of endX and get the bin boundary that come after (or coincides with) it
  if ( endX >= X.back() || endX < startX )
  {
    if ( endX != X.back() )
    {
      g_log.warning("EndX out of range! Set to end of frame");
      endX = X.back();
    }
    m_maxX = Y.size();
  }
  else
  {
    for (m_maxX = m_minX; X[m_maxX] < endX; ++m_maxX) {}
  }  
}

} // namespace Algorithm
} // namespace Mantid
