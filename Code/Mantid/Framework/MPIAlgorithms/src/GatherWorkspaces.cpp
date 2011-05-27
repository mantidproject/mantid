//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMPIAlgorithms/GatherWorkspaces.h"
#include <boost/mpi.hpp>
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"

namespace mpi = boost::mpi;

namespace Mantid
{
namespace MPIAlgorithms
{

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GatherWorkspaces)

void GatherWorkspaces::init()
{
  // Input workspace is optional, except for the root process
  const bool optional(mpi::communicator().rank());
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,optional));
  // Output is optional - only the root process will output a workspace
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output,true));
}

void GatherWorkspaces::exec()
{
  // Every process in an MPI job must hit this next line or everything hangs!
  mpi::communicator world; // The communicator containing all processes

  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");

  // Create a new communicator that includes only those processes that have an input workspace
  const int haveWorkspace(inputWorkspace ? 1 : 0);
  mpi::communicator included = world.split(haveWorkspace);

  // If the present process doesn't have an input workspace then its work is done
  if ( !haveWorkspace ) return;

  // Get the number of bins in each histogram and check they're all the same
  const std::size_t numBins = inputWorkspace->blocksize();
  std::vector<std::size_t> all_numBins;
  all_gather(included, numBins, all_numBins);
  if ( std::count(all_numBins.begin(),all_numBins.end(),numBins) != all_numBins.size() )
  {
    // All the processes will error out if all the workspaces don't have the same number of bins
    throw Exception::MisMatch<std::size_t>(numBins, 0, "All input workspaces must have the same number of bins");
  }

  // Get the total number of spectra in the combined inputs
  std::size_t totalSpec;
  reduce(included, inputWorkspace->getNumberHistograms(), totalSpec, std::plus<std::size_t>(), 0);

  // The root process needs to create a workspace of the appropriate size
  MatrixWorkspace_sptr outputWorkspace;
  if ( world.rank() == 0 )
  {
    g_log.debug() << "Total number of spectra is " << totalSpec << "\n";
    // Create the workspace for the output
    outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,totalSpec,inputWorkspace->readX(0).size(),numBins);
    setProperty("OutputWorkspace",outputWorkspace);

    // Keep it lean-and-mean and don't bother with the spectra map, etc.
    // This line is needed to stop a crash on a subsequent SaveNexus
    outputWorkspace->replaceSpectraMap(new SpectraDetectorMap);
  }

  // Let's assume 1 spectrum in each workspace for the first try....
  // TODO: Generalise

  if ( world.rank() == 0 )
  {
    // Copy over data from own input workspace
    outputWorkspace->dataX(0) = inputWorkspace->readX(0);
    outputWorkspace->dataY(0) = inputWorkspace->readY(0);
    outputWorkspace->dataE(0) = inputWorkspace->readE(0);

    const int numReqs(3*(included.size()-1));
    mpi::request reqs[numReqs];
    int j(0);

    // Receive data from all the other processes
    // This works because the process ranks are ordered the same in 'included' as
    // they are in 'world', but in general this is not guaranteed. TODO: robustify
    for ( int i = 1; i < included.size(); ++i )
    {
      reqs[j++] = included.irecv(i,0,outputWorkspace->dataX(i));
      reqs[j++] = included.irecv(i,1,outputWorkspace->dataY(i));
      reqs[j++] = included.irecv(i,2,outputWorkspace->dataE(i));
    }

    // Make sure everything's been received before exiting the algorithm
    mpi::wait_all(reqs,reqs+numReqs);
  }
  else
  {
    mpi::request reqs[3];

    // Send the spectrum to the root process
    reqs[0] = included.isend(0,0,inputWorkspace->readX(0));
    reqs[1] = included.isend(0,1,inputWorkspace->readY(0));
    reqs[2] = included.isend(0,2,inputWorkspace->readE(0));

    // Make sure the sends have completed before exiting the algorithm
    mpi::wait_all(reqs,reqs+3);
  }

}

} // namespace MPIAlgorithms
} // namespace Mantid
