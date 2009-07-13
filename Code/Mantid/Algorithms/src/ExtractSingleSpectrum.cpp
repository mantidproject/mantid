//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ExtractSingleSpectrum.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractSingleSpectrum)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_const_sptr;

void ExtractSingleSpectrum::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);  
  declareProperty("SpectrumIndex",-1,mustBePositive);
}

void ExtractSingleSpectrum::exec()
{
  // Get hold of the input workspace
  Workspace2D_const_sptr inputWorkspace = getProperty("InputWorkspace");
  // Get the desired spectrum number and check it's in range
  const int desiredSpectrum = getProperty("SpectrumIndex");
  if ( desiredSpectrum >= inputWorkspace->getNumberHistograms() )
  {
    g_log.error("SpectrumIndex is greater than the number of spectra in this workspace.");
    throw Exception::IndexError(desiredSpectrum,inputWorkspace->getNumberHistograms(),this->name());
  }
  //m_progress = new Progress(this,0.0,1.0,lhs->getNumberHistograms());

  Progress p(this,0.0,1.0,1);
  // Now create a single spectrum workspace for the output
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,1,inputWorkspace->readX(0).size(),inputWorkspace->blocksize());
  
  // Copy in the data and spectrum number of the appropriate spectrum
  outputWorkspace->dataX(0) = inputWorkspace->readX(desiredSpectrum);
  outputWorkspace->dataY(0) = inputWorkspace->readY(desiredSpectrum);
  outputWorkspace->dataE(0) = inputWorkspace->readE(desiredSpectrum);
  outputWorkspace->getAxis(1)->spectraNo(0) = inputWorkspace->getAxis(1)->spectraNo(desiredSpectrum);
  
  setProperty("OutputWorkspace",outputWorkspace);
  p.report();
}

} // namespace Algorithms
} // namespace Mantid

