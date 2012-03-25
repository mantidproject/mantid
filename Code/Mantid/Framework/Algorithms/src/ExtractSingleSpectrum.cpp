/*WIKI* 


Extracts a single spectrum from a [[Workspace2D]] and stores it in a new workspace.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ExtractSingleSpectrum.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractSingleSpectrum)

/// Sets documentation strings for this algorithm
void ExtractSingleSpectrum::initDocs()
{
  this->setWikiSummary("Extracts the specified spectrum from a workspace and places it in a new single-spectrum workspace. ");
  this->setOptionalMessage("Extracts the specified spectrum from a workspace and places it in a new single-spectrum workspace.");
}


using namespace Kernel;
using namespace API;

void ExtractSingleSpectrum::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0);  
  declareProperty("WorkspaceIndex",-1,mustBePositive);
}

void ExtractSingleSpectrum::exec()
{
  // Get hold of the input workspace
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  const int indexToExtract = getProperty("WorkspaceIndex");
  const size_t numHist = inputWorkspace->getNumberHistograms();
  if( static_cast<size_t>(indexToExtract) >= numHist )
  {
    throw Exception::IndexError(indexToExtract,inputWorkspace->getNumberHistograms(),this->name());
  }

  // Let crop do the rest
  IAlgorithm_sptr cropper = this->createSubAlgorithm("CropWorkspace", 0.0, 1.0);
  cropper->setProperty("InputWorkspace", inputWorkspace);
  cropper->setProperty("StartWorkspaceIndex", indexToExtract);
  cropper->setProperty("EndWorkspaceIndex", indexToExtract);
  cropper->executeAsSubAlg();

  setProperty<MatrixWorkspace_sptr>("OutputWorkspace", cropper->getProperty("OutputWorkspace"));
}

} // namespace Algorithms
} // namespace Mantid

