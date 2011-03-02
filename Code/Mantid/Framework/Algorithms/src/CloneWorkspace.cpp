//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CloneWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CloneWorkspace)

/// Sets documentation strings for this algorithm
void CloneWorkspace::initDocs()
{
  this->setWikiSummary("Copies an existing workspace into a new one. ");
  this->setOptionalMessage("Copies an existing workspace into a new one.");
}


using namespace Kernel;
using namespace API;
using namespace DataObjects;

void CloneWorkspace::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
}

void CloneWorkspace::exec()
{
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  EventWorkspace_const_sptr inputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(inputWorkspace);
  
  if (inputEvent)
  {
    // Handle an EventWorkspace as the input.

    //Make a brand new EventWorkspace
    EventWorkspace_sptr outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", inputEvent->getNumberHistograms(), 2, 1));

    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputEvent, outputWS, false);

    //You need to copy over the data as well.
    outputWS->copyDataFrom( (*inputEvent) );

    //Cast to the matrixOutputWS and save it
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS));
  }
  else
  {
    // Create the output workspace. This will copy many aspects fron the input one.
    MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);

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
    setProperty("OutputWorkspace", outputWorkspace);
  }
  
}

} // namespace Algorithms
} // namespace Mantid

