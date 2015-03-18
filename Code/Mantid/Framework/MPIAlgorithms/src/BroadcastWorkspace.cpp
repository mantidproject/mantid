//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMPIAlgorithms/BroadcastWorkspace.h"
#include <boost/mpi.hpp>
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/BoundedValidator.h"

namespace mpi = boost::mpi;

namespace Mantid {
namespace MPIAlgorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(BroadcastWorkspace)

void BroadcastWorkspace::init() {
  // Input is optional - only the 'BroadcasterRank' process should provide an
  // input workspace
  declareProperty(new WorkspaceProperty<>(
      "InputWorkspace", "", Direction::Input, PropertyMode::Optional));
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));

  declareProperty("BroadcasterRank", 0,
                  boost::make_shared<BoundedValidator<int>>(
                      0, mpi::communicator().size() - 1));
}

void BroadcastWorkspace::exec() {
  // Every process in an MPI job must hit this next line or everything hangs!
  mpi::communicator world; // The communicator containing all processes

  // Get the rank of the process that's doing the broadcasting
  const int root = getProperty("BroadcasterRank");

  MatrixWorkspace_const_sptr inputWorkspace;
  std::size_t numSpec, numBins;
  bool hist;
  std::string xUnit, yUnit, yUnitLabel;
  bool distribution;

  if (world.rank() == root) {
    inputWorkspace = getProperty("InputWorkspace");
    if (!inputWorkspace) {
      g_log.fatal("InputWorkspace '" + getPropertyValue("InputWorkspace") +
                  "' not found in root process");
      // We need to stop all the other processes. Not very graceful, but there's
      // not much point in trying to be cleverer
      mpi::environment::abort(-1);
    }
    numSpec = inputWorkspace->getNumberHistograms();
    numBins = inputWorkspace->blocksize();
    hist = inputWorkspace->isHistogramData();

    xUnit = inputWorkspace->getAxis(0)->unit()->unitID();
    yUnit = inputWorkspace->YUnit();
    yUnitLabel = inputWorkspace->YUnitLabel();
    distribution = inputWorkspace->isDistribution();
  }

  // Broadcast the size of the workspace
  broadcast(world, numSpec, root);
  broadcast(world, numBins, root);
  broadcast(world, hist, root);

  // Create an output workspace in each process. Assume Workspace2D for now
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", numSpec, numBins + hist, numBins);
  setProperty("OutputWorkspace", outputWorkspace);

  // Broadcast the units
  broadcast(world, xUnit, root);
  broadcast(world, yUnit, root);
  broadcast(world, yUnit, root);
  broadcast(world, distribution, root);
  outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(xUnit);
  outputWorkspace->setYUnit(yUnit);
  outputWorkspace->setYUnitLabel(yUnitLabel);
  outputWorkspace->isDistribution(distribution);

  // TODO: broadcast any other pertinent details. Want to keep this to a minimum
  // though.

  for (std::size_t i = 0; i < numSpec; ++i) {
    if (world.rank() == root) {
      // For local output, just copy over
      outputWorkspace->dataX(i) = inputWorkspace->readX(i);
      outputWorkspace->dataY(i) = inputWorkspace->readY(i);
      outputWorkspace->dataE(i) = inputWorkspace->readE(i);

      // Send out the current spectrum
      broadcast(world, const_cast<MantidVec &>(inputWorkspace->readX(i)), root);
      broadcast(world, const_cast<MantidVec &>(inputWorkspace->readY(i)), root);
      broadcast(world, const_cast<MantidVec &>(inputWorkspace->readE(i)), root);
    } else {
      // Receive the broadcast spectrum from the broadcasting process
      broadcast(world, outputWorkspace->dataX(i), root);
      broadcast(world, outputWorkspace->dataY(i), root);
      broadcast(world, outputWorkspace->dataE(i), root);
    }
  }
}

} // namespace MPIAlgorithms
} // namespace Mantid
