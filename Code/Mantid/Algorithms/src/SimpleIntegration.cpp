//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SimpleIntegration.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"

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
using DataObjects::Workspace1D_sptr;
using DataObjects::Workspace1D;
using DataObjects::Workspace2D_sptr;
using DataObjects::Workspace2D;

// Get a reference to the logger
Logger& SimpleIntegration::g_log = Logger::get("SimpleIntegration");

/** Initialisation method. Does nothing at present.
 * 
 */
void SimpleIntegration::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<Workspace1D>("OutputWorkspace","",Direction::Output));
  
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartX",0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndX",0, mustBePositive->clone());
  declareProperty("StartY",0, mustBePositive->clone());
  declareProperty("EndY",0, mustBePositive->clone());
}

/** Executes the algorithm
 * 
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SimpleIntegration::exec()
{
  // Try and retrieve the optional properties
  m_MinX = getProperty("StartX");
  m_MaxX = getProperty("EndX");
  m_MinY = getProperty("StartY");
  m_MaxY = getProperty("EndY");

  // Get the input workspace
  Workspace2D_sptr localworkspace = getProperty("InputWorkspace");
  
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
    const std::vector<double>& YValues = localworkspace->dataY(i);
    const std::vector<double>& YErrors = localworkspace->dataE(i);

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
  Workspace1D_sptr outputWorkspace = boost::dynamic_pointer_cast<Workspace1D>(API::WorkspaceFactory::Instance().create("Workspace1D"));

  // Populate the 1D workspace
  outputWorkspace->setX(detectorNumber);
  outputWorkspace->setData(sums, errors);
  
  // Assign it to the output workspace property
  setProperty("OutputWorkspace",outputWorkspace);
  
  return;  
}

} // namespace Algorithm
} // namespace Mantid
