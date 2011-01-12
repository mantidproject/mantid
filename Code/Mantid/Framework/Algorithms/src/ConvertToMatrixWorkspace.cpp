//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToMatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToMatrixWorkspace)

using namespace Kernel;
using namespace API;

void ConvertToMatrixWorkspace::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
}

void ConvertToMatrixWorkspace::exec()
{
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  // Let's see if we have to do anything first. Basically we want to avoid the data copy if we can
  DataObjects::EventWorkspace_const_sptr eventW = 
    boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(inputWorkspace);
  MatrixWorkspace_sptr outputWorkspace;
  if( eventW )
  {
    g_log.information() << "Converting EventWorkspace to Workspace2D.\n";
    // Create the output workspace. This will copy many aspects fron the input one.
    outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);

    // ...but not the data, so do that here.
    
    const int numHists = inputWorkspace->getNumberHistograms();
    Progress prog(this,0.0,1.0,numHists);
    
    PARALLEL_FOR2(inputWorkspace,outputWorkspace)
    for (int i = 0; i < numHists; ++i)
    {
      PARALLEL_START_INTERUPT_REGION
	  
      outputWorkspace->setX(i,inputWorkspace->refX(i));
      outputWorkspace->dataY(i) = inputWorkspace->readY(i);
      outputWorkspace->dataE(i) = inputWorkspace->readE(i);
      
      prog.report();

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }
  else
  {
    g_log.information() << "Input workspace does not need converting. Pointing OutputWorkspace property to input.\n";
    outputWorkspace = boost::const_pointer_cast<MatrixWorkspace>(inputWorkspace);
  }

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid

