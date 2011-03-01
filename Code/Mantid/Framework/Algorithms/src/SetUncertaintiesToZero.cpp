//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SetUncertaintiesToZero.h"
#include <vector>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetUncertaintiesToZero)

using namespace Kernel;
using namespace API;

/// (Empty) Constructor
SetUncertaintiesToZero::SetUncertaintiesToZero() : API::Algorithm()
{}

/// Virtual destructor
SetUncertaintiesToZero::~SetUncertaintiesToZero()
{}

/// Algorithm's name
const std::string SetUncertaintiesToZero::name() const
{ return "SetUncertaintiesToZero";}

/// Algorithm's version
int SetUncertaintiesToZero::version() const
{ return (1);}

/// Algorithm's category for identification
const std::string SetUncertaintiesToZero::category() const
{ return "General";}

void SetUncertaintiesToZero::init()
{
  //this->setWikiSummary("This algorithm creates a workspace which is the duplicate of the input, but where the error value for every bin has been set to zero.");
  //this->setOptionalMessage("This algorithm creates a workspace which is the duplicate of the input, but where the error value for every bin has been set to zero.");

  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",
                                                              Direction::Input));
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",
                                                              Direction::Output));
}

void SetUncertaintiesToZero::exec()
{
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");

  // Create the output workspace. This will copy many aspects from the input one.
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
    outputWorkspace->dataE(i) = std::vector<double>(inputWorkspace->readE(i).size(), 0.);

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid

