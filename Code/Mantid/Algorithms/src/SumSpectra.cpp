//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SumSpectra.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SumSpectra)

using namespace Kernel;
using namespace API;

// Get a reference to the logger
Logger& SumSpectra::g_log = Logger::get("SumSpectra");

/** Initialisation method.
 *
 */
void SumSpectra::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new CommonBinsValidator<>));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartSpectrum",0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndSpectrum",0, mustBePositive->clone());
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SumSpectra::exec()
{
  // Try and retrieve the optional properties
  m_MinSpec = getProperty("StartSpectrum");
  m_MaxSpec = getProperty("EndSpectrum");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  const int numberOfSpectra = localworkspace->getNumberHistograms();
  const int YLength = localworkspace->blocksize();
  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if ( m_MinSpec > numberOfSpectra )
  {
    g_log.warning("StartSpectrum out of range! Set to 0.");
    m_MinSpec = 0;
  }
  if ( !m_MaxSpec ) m_MaxSpec = numberOfSpectra-1;
  if ( m_MaxSpec > numberOfSpectra-1 || m_MaxSpec < m_MinSpec )
  {
    g_log.warning("EndSpectrum out of range! Set to max spectrum number");
    m_MaxSpec = numberOfSpectra;
  }

  // Create the 1D workspace for the output
  MatrixWorkspace_sptr outputWorkspace = API::WorkspaceFactory::Instance().create(localworkspace,
			1,localworkspace->readX(0).size(),YLength);

  int progress_step = (m_MaxSpec-m_MinSpec+1) / 100;
  if (progress_step == 0) progress_step = 1;
  
	// Create vectors to hold result
  std::vector<double> XResult = localworkspace->readX(0);
  std::vector<double> YSum(localworkspace->readY(0).size(),0);
  std::vector<double> YError(localworkspace->readY(0).size(),0);
  // Loop over spectra
  for (int i = m_MinSpec, j = 0; i <= m_MaxSpec; ++i,++j)
  {
    // Retrieve the spectrum into a vector
    const std::vector<double>& XValues = localworkspace->readX(i);
    const std::vector<double>& YValues = localworkspace->readY(i);
    const std::vector<double>& YErrors = localworkspace->readE(i);

		for (int k = 0; k < YLength; ++k)
    {
      YSum[k] += YValues[k];
      YError[k] += YErrors[k]*YErrors[k];
    }
   
    if (j % progress_step == 0)
    {
        interruption_point();
        progress( double(j)/(m_MaxSpec-m_MinSpec+1) );
    }

  }
  outputWorkspace->dataX(0) = XResult;
  outputWorkspace->dataY(0) = YSum;
	//take the square root of all the accumulated squared errors - Assumes Gaussian errors
  std::transform(YError.begin(), YError.end(), YError.begin(),dblSqrt);
  outputWorkspace->dataE(0) = YError;
	
  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputWorkspace);

  return;
}

double SumSpectra::dblSqrt(double in)
{
  return sqrt(in);
}

} // namespace Algorithms
} // namespace Mantid
