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
  if(mpi::communicator().rank())
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,PropertyMode::Optional));
  else
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,PropertyMode::Mandatory));
  // Output is optional - only the root process will output a workspace
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output,PropertyMode::Optional));
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
  if ( !haveWorkspace )
  {
    g_log.information("No input workspace on this process, so nothing to do.");
    return;
  }

  // Get the number of bins in each workspace and check they're all the same
  const std::size_t numBins = inputWorkspace->blocksize();
  std::vector<std::size_t> all_numBins;
  all_gather(included, numBins, all_numBins);
  if ( std::count(all_numBins.begin(),all_numBins.end(),numBins) != (int)all_numBins.size() )
  {
    // All the processes will error out if all the workspaces don't have the same number of bins
    throw Exception::MisMatch<std::size_t>(numBins, 0, "All input workspaces must have the same number of bins");
  }
  // Also check that all workspaces are either histogram or not
  // N.B. boost mpi doesn't seem to like me using booleans in the all_gather
  const int hist = inputWorkspace->isHistogramData();
  std::vector<int> all_hist;
  all_gather(included, hist, all_hist);
  if ( std::count(all_hist.begin(),all_hist.end(),hist) != (int)all_hist.size() )
  {
    // All the processes will error out if we don't have either all histogram or all point-data workspaces
    throw Exception::MisMatch<int>(hist, 0, "The input workspaces must be all histogram or all point data");
  }

  // Get the total number of spectra in the combined inputs
  std::size_t totalSpec = inputWorkspace->getNumberHistograms();

  // The root process needs to create a workspace of the appropriate size
  MatrixWorkspace_sptr outputWorkspace;
  if ( world.rank() == 0 )
  {
    g_log.debug() << "Total number of spectra is " << totalSpec << "\n";
    // Create the workspace for the output
    outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,totalSpec,numBins+hist,numBins);
    setProperty("OutputWorkspace",outputWorkspace);
  }

  for (size_t wi = 0; wi < totalSpec; wi++)
  {
    if ( world.rank() == 0 )
    {
      outputWorkspace->dataX(wi) = inputWorkspace->readX(wi);
      reduce(included, inputWorkspace->readY(wi), outputWorkspace->dataY(wi), vplus(), 0);
      reduce(included, inputWorkspace->readE(wi), outputWorkspace->dataE(wi), eplus(), 0);
      const ISpectrum * inSpec = inputWorkspace->getSpectrum(wi);
      ISpectrum * outSpec = outputWorkspace->getSpectrum(wi);
      outSpec->clearDetectorIDs();
      outSpec->addDetectorIDs( inSpec->getDetectorIDs() );
    }
    else
    {
      reduce(included, inputWorkspace->readY(wi), vplus(), 0);
      reduce(included, inputWorkspace->readE(wi), eplus(), 0);
    }
  }

}

} // namespace MPIAlgorithms
} // namespace Mantid
