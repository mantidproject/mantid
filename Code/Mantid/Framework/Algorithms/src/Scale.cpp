//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Scale.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Scale)

using namespace Kernel;
using namespace API;

void Scale::init()
{
  //this->setWikiSummary("Scales an input workspace by the given factor, which can be either multiplicative or additive.");
  //this->setOptionalMessage("Scales an input workspace by the given factor, which can be either multiplicative or additive.");

  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  
  declareProperty("Factor",1.0,"The factor by which to scale the input workspace");
  std::vector<std::string> op(2);
  op[0] = "Multiply";
  op[1] = "Add";
  declareProperty("Operation","Multiply",new ListValidator(op),"The scaling operation: multiply (default) or add");
}

void Scale::exec()
{
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS;
  
  const double factor = getProperty("Factor");
  const std::string op = getPropertyValue("Operation");
  
  if (op == "Multiply")
  {
    outputWS = inputWS * factor;
  }
  else
  {
    outputWS = inputWS + factor;
  }
  
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid

