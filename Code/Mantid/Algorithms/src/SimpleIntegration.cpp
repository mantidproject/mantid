//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SimpleIntegration)

using namespace Kernel;
using namespace API;

// Get a reference to the logger
Logger& SimpleIntegration::g_log = Logger::get("SimpleIntegration");

/** Initialisation method.
 *
 */
void SimpleIntegration::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new HistogramValidator<>));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty("Range_lower",0.0);
  declareProperty("Range_upper",0.0);
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
void SimpleIntegration::exec()
{
  // Try and retrieve the optional properties
  m_MinRange = getProperty("Range_lower");
  m_MaxRange = getProperty("Range_upper");
  m_MinSpec = getProperty("StartSpectrum");
  m_MaxSpec = getProperty("EndSpectrum");

  // Get the input workspace
  Workspace_const_sptr localworkspace = getProperty("InputWorkspace");

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
    g_log.warning("EndSpectrum out of range! Set to max detector number");
    m_MaxSpec = numberOfSpectra;
  }
  if ( m_MinRange > m_MaxRange )
  {
    g_log.warning("Range_upper is less than Range_lower. Will integrate up to frame maximum.");
    m_MaxRange = 0.0;
  }

  // Create the 1D workspace for the output
  Workspace_sptr outputWorkspace = API::WorkspaceFactory::Instance().create(localworkspace,m_MaxSpec-m_MinSpec+1,2,1);

  // Create vectors to hold result
  std::vector<double> XValue(2);
  // Loop over spectra
  for (int i = m_MinSpec, j = 0; i <= m_MaxSpec; ++i,++j)
  {
    // Retrieve the spectrum into a vector
    const std::vector<double>& XValues = localworkspace->readX(i);
    const std::vector<double>& YValues = localworkspace->readY(i);
    const std::vector<double>& YErrors = localworkspace->readE(i);

    double maxX = m_MaxRange;
    if (!m_MaxRange) maxX = XValues.back();

    int startBin = -1;
    int endBin = YLength;
    double YSum = 0.0;
    double YError = 0.0;
    for (int k = 0; k < YLength; ++k)
    {
      if (XValues[k] >= m_MinRange && XValues[k+1] <= maxX )
      {
        if (startBin == -1) startBin = k;
        endBin = k+1;
        YSum += YValues[k];
        YError += YErrors[k]*YErrors[k];
      }
    }

    XValue[0] = XValues[startBin];
    XValue[1] = XValues[endBin];
    outputWorkspace->dataX(j) = XValue;
    outputWorkspace->dataY(j)[0] = YSum;
    outputWorkspace->dataE(j)[0] = sqrt(YError);
    if (localworkspace->axes() > 1)
    {
      outputWorkspace->getAxis(1)->spectraNo(j) = localworkspace->getAxis(1)->spectraNo(i);
    }
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputWorkspace);

  return;
}

} // namespace Algorithms
} // namespace Mantid
