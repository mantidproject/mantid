//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/MarkDeadDetectors.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MarkDeadDetectors)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;

// Initialise the logger
Kernel::Logger& MarkDeadDetectors::g_log = Kernel::Logger::get("MarkDeadDetectors");

/// (Empty) Constructor
MarkDeadDetectors::MarkDeadDetectors() {}

/// Destructor
MarkDeadDetectors::~MarkDeadDetectors() {}

void MarkDeadDetectors::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("Workspace","", Direction::InOut));
  declareProperty(new ArrayProperty<int>("WorkspaceIndexList"));
  
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndexMin",0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("WorkspaceIndexMax",0, mustBePositive->clone());
}

void MarkDeadDetectors::exec()
{
  // Get the input workspace
  m_localWorkspace = getProperty("Workspace");
  // Get the size of the vectors
  const int vectorSize = m_localWorkspace->blocksize();

  Property *DeadDetectorList = getProperty("WorkspaceIndexList");
  // This is an optional property - only do something if it's actually been set.
  if ( ! DeadDetectorList->isDefault() )
  {
    std::vector<int> detList = getProperty("WorkspaceIndexList");
    std::vector<int>::const_iterator it;
    for (it = detList.begin(); it != detList.end(); ++it) 
    {
      clearSpectrum(*it,vectorSize);
    }
  }
  
  Property *minDet = getProperty("WorkspaceIndexMin");
  Property *maxDet = getProperty("WorkspaceIndexMax");
  // These are optional properties - only do something if they've actually been set.
  if ( !minDet->isDefault() && !maxDet->isDefault() )
  {
    int min = getProperty("WorkspaceIndexMin");
    int max = getProperty("WorkspaceIndexMax");
    // Check validity of min/max properties
    if ( min > max )
    {
      g_log.error() << "WorkspaceIndexMin (" << min << ") cannot be greater than WorkspaceIndexMax (" << max << ")";
      throw std::invalid_argument("WorkspaceIndexMin cannot be greater than WorkspaceIndexMax");
    }
    // If max goes beyond end of workspace, print warning and set to highest index
    if (max >= m_localWorkspace->getHistogramNumber() )
    {
      g_log.warning("WorkspaceIndexMax is greater than workspace size - will go to end of workspace");
      max = m_localWorkspace->getHistogramNumber()-1;
    }
    
    for (int i = min; i <= max; ++i) 
    {
      clearSpectrum(i,vectorSize);
    }
  }
  
}

void MarkDeadDetectors::clearSpectrum(const int& index, const int& vectorSize)
{
  // Mark associated detector as dead
  m_localWorkspace->getInstrument()->getDetector(m_localWorkspace->spectraNo(index))->markDead();

  // Zero the workspace spectra (data and errors, not X values)
  m_localWorkspace->dataY(index).assign(vectorSize,0.0);
  m_localWorkspace->dataE(index).assign(vectorSize,0.0);
  m_localWorkspace->dataE2(index).assign(vectorSize,0.0);
}

} // namespace DataHandling
} // namespace Mantid
