//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SumSpectra.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SumSpectra)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_const_sptr;


/** Initialisation method.
 *
 */
void SumSpectra::init()
{
  declareProperty(
    new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input,
                                       new CommonBinsValidator<Workspace2D>),
                                       "The workspace containing the spectra to be summed" );
  declareProperty(
    new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "The name of the workspace to be created as the output of the algorithm" );

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartSpectrum",0, mustBePositive,
    "The first spectra index to be included in the summing (default 0)" );
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndSpectrum",0, mustBePositive->clone(),
    "The last spectra index to be included in the summing (default\n"
    "highest index)" );
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
  Workspace2D_const_sptr localworkspace = getProperty("InputWorkspace");

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

  Progress progress(this,0,1,m_MinSpec,m_MaxSpec,1);
  
  // Copy over the bin boundaries
  outputWorkspace->dataX(0) = localworkspace->readX(0);
	// Get references to the output workspaces's data vectors
  MantidVec& YSum = outputWorkspace->dataY(0);
  MantidVec& YError = outputWorkspace->dataE(0);
  // Get a reference to the spectra-detector map
  SpectraDetectorMap& specMap = outputWorkspace->mutableSpectraMap();
  const Axis* const spectraAxis = localworkspace->getAxis(1);

  // Loop over spectra
  for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  {
    // Retrieve the spectrum into a vector
    const std::vector<double>& YValues = localworkspace->readY(i);
    const std::vector<double>& YErrors = localworkspace->readE(i);

		for (int k = 0; k < YLength; ++k)
    {
      YSum[k] += YValues[k];
      YError[k] += YErrors[k]*YErrors[k];
    }
   
    // Map all the detectors onto the spectrum of the output (which is 0)
    specMap.addSpectrumEntries(0,specMap.getDetectors(spectraAxis->spectraNo(i)));

    progress.report();
  }
	//take the square root of all the accumulated squared errors - Assumes Gaussian errors
  std::transform(YError.begin(), YError.end(), YError.begin(), dblSqrt);
	
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
