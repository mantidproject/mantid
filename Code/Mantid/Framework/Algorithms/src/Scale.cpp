//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Scale.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Scale)

using namespace Kernel;
using namespace API;

void Scale::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input));
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));

  declareProperty("Factor", 1.0,
                  "The value by which to scale the input workspace");
  std::vector<std::string> op(2);
  op[0] = "Multiply";
  op[1] = "Add";
  declareProperty("Operation", "Multiply",
                  boost::make_shared<StringListValidator>(op),
                  "Whether to multiply by, or add factor");
}

void Scale::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  const double factor = getProperty("Factor");
  const std::string op = getPropertyValue("Operation");

  if (op == "Multiply") {
    if (outputWS == inputWS)
      inputWS *= factor;
    else
      outputWS = inputWS * factor;
  } else {
    if (outputWS == inputWS)
      inputWS += factor;
    else
      outputWS = inputWS + factor;
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
