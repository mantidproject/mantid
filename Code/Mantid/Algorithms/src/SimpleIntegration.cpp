//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "SimpleIntegration.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"

#include <sstream>
#include <numeric>
#include <math.h>

// Register the class into the algorithm factory
//DECLARE_NAMESPACED_ALGORITHM(Mantid::Algorithms, SimpleIntegration)

namespace Mantid
{
namespace Algorithms
{

using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace1D;
using Mantid::DataObjects::Workspace2D;

// Get a reference to the logger
Logger& SimpleIntegration::g_log = Logger::get("SimpleIntegration");

/** Initialisation method. Does nothing at present.
 * 
 *  @return A StatusCode object indicating whether the operation was successful
 */
StatusCode SimpleIntegration::init()
{
  declareProperty("StartX",0);
  declareProperty("EndX",0);
  declareProperty("StartY",0);
  declareProperty("EndY",0);
  
  return StatusCode::SUCCESS;
}

/** Executes the algorithm
 * 
 *  @return A StatusCode object indicating whether the operation was successful
 */
StatusCode SimpleIntegration::exec()
{
  // Try and retrieve the optional properties
  // Since a property can only be a string at present, need to convert to int
  // No type checking - this code MUST be temporary
  /// @todo Remove the need for this conversion
  try {
    std::string strInput = getPropertyValue("StartX");
    std::istringstream iss (strInput,std::istringstream::in);
    iss >> m_MinX;
  } catch (Exception::NotFoundError e) {
    m_MinX = 0;
  }

  try {
    std::string strInput = getPropertyValue("EndX");
    std::istringstream iss (strInput,std::istringstream::in);
    iss >> m_MaxX;
  } catch (Exception::NotFoundError e) {
    m_MaxX = 0;
  }

  try {
    std::string strInput = getPropertyValue("StartY");
    std::istringstream iss (strInput,std::istringstream::in);
    iss >> m_MinY;
  } catch (Exception::NotFoundError e) {
    m_MinY = 0;
  }
  
  try {
    std::string strInput = getPropertyValue("EndY");
    std::istringstream iss (strInput,std::istringstream::in);
    iss >> m_MaxY;
  } catch (Exception::NotFoundError e) {
    m_MaxY = 0;
  }
    
  const Workspace2D *localworkspace = dynamic_cast<Workspace2D*>(m_inputWorkspace);
  const int numberOfYBins = localworkspace->getHistogramNumber();
  // Check 'StartX' is in range 0-numberOfSpectra
  if ( (0 > m_MinY) || (m_MinY > numberOfYBins))
  {
    g_log.information("StartY out of range! Set to 0.");
    m_MinY = 0;
  }

  
  if ( !m_MaxY ) m_MaxY = numberOfYBins;
  if ( m_MaxY > numberOfYBins || m_MaxY < m_MinY ) 
  {
    g_log.information("EndY out of range! Set to max detector number");
    m_MaxY = numberOfYBins;
  }
  
  // Create vectors to hold result
  std::vector<double> detectorNumber;
  std::vector<double> sums;
  std::vector<double> errors;
  // Loop over spectra
  for (int i = m_MinY; i < m_MaxY; ++i) {
    
    // Retrieve the spectrum into a vector
    const std::vector<double>& YValues = localworkspace->getY(i);
    const std::vector<double>& YErrors = localworkspace->getE(i);

    // First time through the loop, do some checking on StartY & EndY
    if ( i == m_MinY )
    {
      const int numberOfXBins = YValues.size();
      if ( (0 > m_MinX) || (m_MinX > numberOfXBins))
      {
        g_log.information("StartX out of range! Set to 0");
        m_MinX = 0;
      }
      if ( !m_MaxX ) m_MaxX = numberOfXBins;
      if ( m_MaxX > numberOfXBins || m_MaxX < m_MinX)
      {
        g_log.information("EndX out of range! Set to max number");
        m_MaxX = numberOfXBins;
      }
    }
    
    // Sum up the required elements of the vector
    double YSum = std::accumulate(YValues.begin()+m_MinX,YValues.begin()+m_MaxX,0.0);
    // Error propagation - sqrt(sum of squared elements)
    double YError = sqrt(std::inner_product(YErrors.begin()+m_MinX,YErrors.begin()+m_MaxX,
                                        YErrors.begin()+m_MinX,0.0));
    
    // Add the results to the vectors for the new workspace
    // Warning: Counting detector number from 0
    detectorNumber.push_back(i);
    sums.push_back(YSum);    
    errors.push_back(YError);

  }
  
  // Create the 1D workspace for the output
  WorkspaceFactory *factory = WorkspaceFactory::Instance();
  m_outputWorkspace = factory->create("Workspace1D");
  Workspace1D *localWorkspace = dynamic_cast<Workspace1D*>(m_outputWorkspace);

  // Populate the 1D workspace
  localWorkspace->setX(detectorNumber);
  localWorkspace->setData(sums, errors);



  return StatusCode::SUCCESS;
}

/** Finalisation method. Does nothing at present.
 *
 *  @return A StatusCode object indicating whether the operation was successful
 */
StatusCode SimpleIntegration::final()
{
  return StatusCode::SUCCESS;
}

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SimpleIntegration)

} // namespace Algorithm
} // namespace Mantid
