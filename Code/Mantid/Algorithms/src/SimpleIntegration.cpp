//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidDataObjects/Workspace2D.h"
#include <sstream>
#include <numeric>
#include <math.h>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SimpleIntegration)

using namespace Kernel;
using API::WorkspaceProperty;
using DataObjects::Workspace2D_sptr;
using DataObjects::Workspace2D;

// Get a reference to the logger
Logger& SimpleIntegration::g_log = Logger::get("SimpleIntegration");

/** Initialisation method.
 *
 */
void SimpleIntegration::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace","",Direction::Output));

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartSpectrum",0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndSpectrum",0, mustBePositive->clone());
  declareProperty("StartBin",0, mustBePositive->clone());
  declareProperty("EndBin",0, mustBePositive->clone());
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SimpleIntegration::exec()
{
  // Try and retrieve the optional properties
  m_MinBin = getProperty("StartBin");
  m_MaxBin = getProperty("EndBin");
  m_MinSpec = getProperty("StartSpectrum");
  m_MaxSpec = getProperty("EndSpectrum");

  // Get the input workspace
  Workspace2D_sptr localworkspace = getProperty("InputWorkspace");

  const int numberOfSpectra = localworkspace->getNumberHistograms();
  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if ( (0 > m_MinSpec) || (m_MinSpec > numberOfSpectra))
  {
    g_log.information("StartSpectrum out of range! Set to 0.");
    m_MinSpec = 0;
  }
  if ( !m_MaxSpec ) m_MaxSpec = numberOfSpectra-1;
  if ( m_MaxSpec > numberOfSpectra-1 || m_MaxSpec < m_MinSpec )
  {
    g_log.information("EndSpectrum out of range! Set to max detector number");
    m_MaxSpec = numberOfSpectra;
  }

  // Create the 1D workspace for the output
  Workspace2D_sptr outputWorkspace = boost::dynamic_pointer_cast<Workspace2D>(API::WorkspaceFactory::Instance().create(localworkspace,m_MaxSpec-m_MinSpec+1,1,1));
  // The first axis should be unit-less
  outputWorkspace->getAxis(0)->unit() = boost::shared_ptr<Unit>();

  // Create vectors to hold result
  const std::vector<double> XValue(1,0.0);
  std::vector<double> YSum(1);
  std::vector<double> YError(1);
  // Loop over spectra
  for (int i = m_MinSpec, j = 0; i <= m_MaxSpec; ++i,++j)
  {
    // Retrieve the spectrum into a vector
    const std::vector<double>& YValues = localworkspace->dataY(i);
    const std::vector<double>& YErrors = localworkspace->dataE(i);

    // First time through the loop, do some checking on StartBin & EndBin
    if ( i == m_MinSpec )
    {
      const int numberOfXBins = YValues.size();
      if ( (0 > m_MinBin) || (m_MinBin > numberOfXBins))
      {
        g_log.information("StartBin out of range! Set to 0");
        m_MinBin = 0;
      }
      if ( !m_MaxBin )
      {
        m_MaxBin = numberOfXBins;
      }
      else
      {
        ++m_MaxBin;
      }
      if ( m_MaxBin > numberOfXBins || m_MaxBin < m_MinBin)
      {
        g_log.information("EndBin out of range! Set to max number");
        m_MaxBin = numberOfXBins;
      }
    }

    // Sum up the required elements of the vector
    YSum[0] = std::accumulate(YValues.begin()+m_MinBin,YValues.begin()+m_MaxBin,0.0);
    // Error propagation - sqrt(sum of squared elements)
    YError[0] = sqrt(std::inner_product(YErrors.begin()+m_MinBin,YErrors.begin()+m_MaxBin,
                                        YErrors.begin()+m_MinBin,0.0));

    outputWorkspace->dataX(j) = XValue;
    outputWorkspace->dataY(j) = YSum;
    outputWorkspace->dataE(j) = YError;
    outputWorkspace->getAxis(1)->spectraNo(j) = localworkspace->getAxis(1)->spectraNo(i);
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputWorkspace);

  return;
}

} // namespace Algorithm
} // namespace Mantid
