/*WIKI* 

Uses the binary operation algorithms [[Multiply]] or [[Plus]] to scale the input workspace by the amount requested. This algorithm is provided as a simple, but less powerful, alternative to the python [[Workspace_Algebra|workspace algebra]] functionality.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Scale.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Scale)

/// Sets documentation strings for this algorithm
void Scale::initDocs()
{
  this->setWikiSummary("Scales an input workspace by the given factor, which can be either multiplicative or additive. ");
  this->setOptionalMessage("Scales an input workspace by the given factor, which can be either multiplicative or additive.");
}


using namespace Kernel;
using namespace API;

void Scale::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  
  declareProperty("Factor",1.0,"The factor by which to scale the input workspace");
  std::vector<std::string> op(2);
  op[0] = "Multiply";
  op[1] = "Add";
  declareProperty("Operation","Multiply",boost::make_shared<StringListValidator>(op),"The scaling operation: multiply (default) or add");
}

void Scale::exec()
{
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  
  const double factor = getProperty("Factor");
  const std::string op = getPropertyValue("Operation");
  
  if (op == "Multiply")
  {
    if (outputWS == inputWS) inputWS *= factor;
    else outputWS = inputWS * factor;
  }
  else
  {
    if (outputWS == inputWS) inputWS += factor;
    else outputWS = inputWS + factor;
  }
  
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid

