//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ExtractSingleSpectrum.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractSingleSpectrum)

using namespace Kernel;
using namespace API;

void ExtractSingleSpectrum::init()
{
  //this->setWikiSummary("Extracts the specified spectrum from a workspace and places it in a new single-spectrum workspace.");
  //this->setOptionalMessage("Extracts the specified spectrum from a workspace and places it in a new single-spectrum workspace.");

  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);  
  declareProperty("WorkspaceIndex",-1,mustBePositive);
}

void ExtractSingleSpectrum::exec()
{
  // Get hold of the input workspace
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  // Get the desired spectrum number and check it's in range
  const int desiredSpectrum = getProperty("WorkspaceIndex");
  if ( desiredSpectrum >= inputWorkspace->getNumberHistograms() )
  {
    g_log.error("WorkspaceIndex is greater than the number of entries in this workspace.");
    throw Exception::IndexError(desiredSpectrum,inputWorkspace->getNumberHistograms(),this->name());
  }

  // Now create a single spectrum workspace for the output
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,1,inputWorkspace->readX(0).size(),inputWorkspace->blocksize());
  
  progress(0.5);
  // Copy in the data and spectrum number of the appropriate spectrum
  outputWorkspace->dataX(0) = inputWorkspace->readX(desiredSpectrum);
  outputWorkspace->dataY(0) = inputWorkspace->readY(desiredSpectrum);
  outputWorkspace->dataE(0) = inputWorkspace->readE(desiredSpectrum);
  // If Axis 1 on the original is a spectra axis copy over the correct spectrum number
  const Axis * axisOne = inputWorkspace->getAxis(1);
  if( axisOne->isSpectra() )
  {
    outputWorkspace->getAxis(1)->spectraNo(0) = inputWorkspace->getAxis(1)->spectraNo(desiredSpectrum);
  }
  setProperty("OutputWorkspace",outputWorkspace);
  progress(1.0);
}

} // namespace Algorithms
} // namespace Mantid

