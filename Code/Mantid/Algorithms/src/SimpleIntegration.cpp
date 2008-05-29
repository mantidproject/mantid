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
  if ( !m_MaxY ) m_MaxY = numberOfYBins-1;
  if ( m_MaxY > numberOfYBins-1 || m_MaxY < m_MinY ) 
  {
    g_log.information("EndY out of range! Set to max detector number");
    m_MaxY = numberOfYBins;
  }

  // Create the 1D workspace for the output
  Workspace2D_sptr outputWorkspace = boost::dynamic_pointer_cast<Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D",m_MaxY-m_MinY+1,1,1));
  
  // Create vectors to hold result
  const std::vector<double> XValue(1,0.0);
  std::vector<double> YSum(1);
  std::vector<double> YError(1);
  // Loop over spectra
  for (int i = m_MinY, j = 0; i <= m_MaxY; ++i,++j) 
  {  
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
      if ( !m_MaxX ) 
      {
        m_MaxX = numberOfXBins;  
      }
      else
      {
        ++m_MaxX;
      }
      if ( m_MaxX > numberOfXBins || m_MaxX < m_MinX)
      {
        g_log.information("EndX out of range! Set to max number");
        m_MaxX = numberOfXBins;
      }
    }
    
    // Sum up the required elements of the vector
    YSum[0] = std::accumulate(YValues.begin()+m_MinX,YValues.begin()+m_MaxX,0.0);
    // Error propagation - sqrt(sum of squared elements)
    YError[0] = sqrt(std::inner_product(YErrors.begin()+m_MinX,YErrors.begin()+m_MaxX,
                                        YErrors.begin()+m_MinX,0.0));
    
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
