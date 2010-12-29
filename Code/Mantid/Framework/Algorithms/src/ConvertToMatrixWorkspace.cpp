//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToMatrixWorkspace.h"

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

} // namespace Algorithms
} // namespace Mantid

