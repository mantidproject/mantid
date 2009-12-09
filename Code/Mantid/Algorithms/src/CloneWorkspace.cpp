//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CloneWorkspace)

using namespace Kernel;
using namespace API;

void CloneWorkspace::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
}

void CloneWorkspace::exec()
{
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  
  // Create the output workspace. This will copy many aspects fron the input one.
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);
  
  // ...but not the data, so do that here.
  
  // Preserve X data sharing, if present.
  DataObjects::Workspace2D_sptr output2D = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(outputWorkspace);
  const bool commonBoundaries = WorkspaceHelpers::commonBoundaries(inputWorkspace);
  
  DataObjects::Histogram1D::RCtype newX;
  if ( output2D && commonBoundaries )
  {
    newX.access() = inputWorkspace->readX(0);
  }
  
  const int numHists = inputWorkspace->getNumberHistograms();
  Progress prog(this,0.0,1.0,numHists);
  
  for (int i = 0; i < numHists; ++i)
  {
    // Preserve/restore sharing if X vectors are the same
    if ( output2D && commonBoundaries )
    {
      output2D->setX(i,newX);
    }
    else
    {
      outputWorkspace->dataX(i) = inputWorkspace->readX(i);
    }
    
    outputWorkspace->dataY(i) = inputWorkspace->readY(i);
    outputWorkspace->dataE(i) = inputWorkspace->readE(i);
    
    prog.report();
  }
  
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid

