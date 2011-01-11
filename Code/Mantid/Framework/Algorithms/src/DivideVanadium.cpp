//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DivideVanadium.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DivideVanadium)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

void DivideVanadium::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("VanadiumWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "Name of the output workspace, can be the same as the input" );

}

void DivideVanadium::exec()
{
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  const int numHists = inputWorkspace->getNumberHistograms();
  const int numBins = inputWorkspace->blocksize();
  MatrixWorkspace_sptr outputWorkspace = getProperty("OutputWorkspace");
  // If input and output workspaces are not the same, create a new workspace for the output
  if (outputWorkspace != inputWorkspace ) outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);


  MatrixWorkspace_const_sptr vanadiumWorkspace = getProperty("VanadiumWorkspace");

    // ...but not the data, so do that here.

    const int vnumHists = vanadiumWorkspace->getNumberHistograms();
    Progress prog(this,0.0,1.0,numHists);

    PARALLEL_FOR2(inputWorkspace,outputWorkspace)
    for (int i = 0; i < numHists; ++i)
    {
      PARALLEL_START_INTERUPT_REGION

    for (int iv = 0; iv < vnumHists; ++iv)
    {
      if((inputWorkspace->getDetector(i)->getID()) == 
          (inputWorkspace->getDetector(iv)->getID()))
          for (int ib = 0; ib < numBins; ++ib){
          outputWorkspace->dataY(i)[ib] = 
          inputWorkspace->readY(i)[ib]/vanadiumWorkspace->readY(iv)[ib];
          }
    }

      prog.report();
  
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
    setProperty("InputWorkspace", outputWorkspace);
  
}

} // namespace Algorithms
} // namespace Mantid

