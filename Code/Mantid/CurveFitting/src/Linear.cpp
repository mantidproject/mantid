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

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Get a reference to the logger. It is used to print out information, warning and error messages
Logger& Linear::g_log = Logger::get("Linear");

Linear::Linear() : API::Algorithm(), m_minX(0), m_maxX(0)
{}

void Linear::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumIndex",0, mustBePositive);
  declareProperty("StartX",0.0);
  declareProperty("EndX",0.0);
  
  declareProperty("FitStatus","", Direction::Output);
  declareProperty("FitIntercept",0.0,Direction::Output);
  declareProperty("FitSlope",0.0,Direction::Output);
  declareProperty("Chi^2",0.0, Direction::Output);

  // Disable default gsl error handler (which is to call abort!)
  gsl_set_error_handler_off();
}

void Linear::exec()
{
  // Get the input workspace
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  // Get the spectrum to fit
  const int histNumber = getProperty("SpectrumIndex");
  // Check validity
  if ( histNumber >= inputWorkspace->getNumberHistograms() )
  {
    g_log.error() << "SpectrumIndex set to an invalid value of " << histNumber << std::endl;
    throw Exception::IndexError(histNumber,inputWorkspace->getNumberHistograms(),"Linear SpectrumIndex property");
  }

  // Get references to the data in the chosen spectrum
  const std::vector<double>& X = inputWorkspace->dataX(histNumber);
  const std::vector<double>& Y = inputWorkspace->dataY(histNumber);
  const std::vector<double>& E = inputWorkspace->dataE(histNumber);
  // Check if this spectrum has errors
  bool noErrors = true;

  // Retrieve the Start/EndX properties, if set
  this->setRange(X,Y);
  
  const bool isHistogram = inputWorkspace->isHistogramData();
  const int numPoints = m_maxX - m_minX;

  // Create the X & E vectors required for the gsl call
  std::vector<double> XCen(numPoints), weights(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    // Need to adjust X to centre of bin, if a histogram
    XCen[i] = ( isHistogram ? 0.5*(X[m_minX+i]+X[m_minX+i+1]) : X[m_minX] );
    // GSL wants the errors as weights, i.e. 1/sigma^2
    // We need to be careful if E is zero because that would naively lead to an infinite weight on the point.
    // Solution taken here is to zero weight if error is zero, which typically means Y is zero
    //   (so it is effectively excluded from the fit).
    const double& currentE = E[m_minX+i];
    weights[i] = (currentE ? 1.0/(currentE*currentE) : 0.0);
    // However, if the spectrum given has all errors of zero, then we should use the gsl function that
    //   doesn't take account of the errors.
    if ( noErrors && currentE ) noErrors = false;
  }
  
  // Call the gsl fitting function
  // The stride value of 1 reflects that fact that we want every element of out input vectors
  const int stride = 1;
  double *c0(new double),*c1(new double),*cov00(new double),*cov01(new double),*cov11(new double),*chisq(new double);
  int status;
  // If this spectrum had ALL zeros for the errors, call the gsl function that doesn't use errors
  if ( noErrors )
  {
    status = gsl_fit_linear(&XCen[0],stride,&Y[m_minX],stride,numPoints,c0,c1,cov00,cov01,cov11,chisq);
  }
  // Otherwise, call the one that does account for errors on the data points
  else
  {
    status = gsl_fit_wlinear(&XCen[0],stride,&weights[0],stride,&Y[m_minX],stride,numPoints,c0,c1,cov00,cov01,cov11,chisq);
  }
  
  // Check that the fit succeeded
  std::string fitStatus = gsl_strerror(status);
  if (status) g_log.error() << "The fit failed: " << fitStatus << std::endl;
  else
  {
    g_log.debug() << "The fit succeeded, giving y = " << *c0 << " + " << *c1 << "*x, with a Chi^2 of " << *chisq << std::endl;
  }
  
  // Set the fit result output properties
  setProperty("FitStatus",fitStatus);
  setProperty("FitIntercept",*c0);
  setProperty("FitSlope",*c1);
  setProperty("Chi^2",*chisq);
  
  // Create and fill a workspace1D with the same bins as the fitted spectrum and the value of the fit for the centre of each bin
  const int YSize = Y.size();
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create("Workspace1D",1,X.size(),YSize);
  // Copy over various data members from the input workspace
  outputWorkspace->setInstrument(inputWorkspace->getInstrument());
  outputWorkspace->setSample(inputWorkspace->getSample());
  outputWorkspace->setYUnit(inputWorkspace->YUnit());
  outputWorkspace->isDistribution(inputWorkspace->isDistribution());
  outputWorkspace->getAxis(0)->unit() = inputWorkspace->getAxis(0)->unit();
  outputWorkspace->getAxis(0)->title() = inputWorkspace->getAxis(0)->title();
  
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
  
  // Clean up
  delete c0;
  delete c1;
  delete cov00;
  delete cov01;
  delete cov11;
  delete chisq;
}

/// Retrieve and check the Start/EndX parameters, if set
void Linear::setRange(const std::vector<double>& X, const std::vector<double>& Y)
{
  Property* start = getProperty("StartX");
  double startX;
  // If startX or endX has not been set, make it 6*sigma away from the centre point initial guess
  if ( ! start->isDefault() ) startX = getProperty("StartX");
  else startX = X.front();
  Property* end = getProperty("EndX");
  double endX;
  if ( ! end->isDefault() ) endX = getProperty("EndX");
  else endX = X.back();

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
