//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMPIAlgorithms/BroadcastWorkspace.h"
#include <boost/mpi.hpp>

namespace mpi = boost::mpi;

namespace Mantid
{
namespace MPIAlgorithms
{

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(BroadcastWorkspace)

void BroadcastWorkspace::init()
{
  // Input is optional - only the 'BroadcasterRank' process should provide an input workspace
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,true));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty("BroadcasterRank",0,new BoundedValidator<int>(0,mpi::communicator().size()-1));
}

void BroadcastWorkspace::exec()
{
  // Every process in an MPI job must hit this next line or everything hangs!
  mpi::communicator world; // The communicator containing all processes

  // Get the rank of the process that's doing the broadcasting
  const int root = getProperty("BroadcasterRank");

  MatrixWorkspace_const_sptr inputWorkspace;
  std::size_t numSpec, numBins;
  bool hist;

  if ( world.rank() == root )
  {
    inputWorkspace = getProperty("InputWorkspace");
    if ( !inputWorkspace )
    {
      throw Exception::NotFoundError("InputWorkspace not found in root process",getPropertyValue("InputWorkspace"));
      // TODO: Stop all the other processes
    }
    numSpec = inputWorkspace->getNumberHistograms();
    numBins = inputWorkspace->blocksize();
    hist = inputWorkspace->isHistogramData();
  }

  // Broadcast the size of the workspace
  broadcast(world,numSpec,root);
  broadcast(world,numBins,root);
  broadcast(world,hist,root);

  // Create an output workspace in each process. Assume Workspace2D for now
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create("Workspace2D",numSpec,numBins+hist,numBins);
  // Hard-code the unit for now as it's needed in my example to allow a divide.
  // TODO: Broadcast this and any other pertinent details
  outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");
  setProperty("OutputWorkspace",outputWorkspace);

  for ( std::size_t i = 0; i < numSpec; ++i )
  {
    if ( world.rank() == root )
    {
      // For local output, just copy over
      outputWorkspace->dataX(i) = inputWorkspace->readX(i);
      outputWorkspace->dataY(i) = inputWorkspace->readY(i);
      outputWorkspace->dataE(i) = inputWorkspace->readE(i);

      // Send out the current spectrum
      broadcast(world,const_cast<MantidVec&>(inputWorkspace->readX(i)),root);
      broadcast(world,const_cast<MantidVec&>(inputWorkspace->readY(i)),root);
      broadcast(world,const_cast<MantidVec&>(inputWorkspace->readE(i)),root);
    }
    else
    {
      // Receive the broadcast spectrum from the broadcasting process
      broadcast(world,outputWorkspace->dataX(i),root);
      broadcast(world,outputWorkspace->dataY(i),root);
      broadcast(world,outputWorkspace->dataE(i),root);
    }
  }

}

} // namespace MPIAlgorithms
} // namespace Mantid
