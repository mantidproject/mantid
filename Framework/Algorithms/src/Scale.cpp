//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Scale.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Scale)

using namespace Kernel;
using namespace API;

void Scale::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input));
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output));

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

  auto hasDx = inputWS->hasDx(0);

  Progress progress(this, 0.0, 1.0, 2);

  // We require a copy of the workspace if the
  auto inPlace = outputWS == inputWS;
  MatrixWorkspace_sptr bufferWS;

  if (op == "Multiply") {

    progress.report("Multiplying factor...");

    if (outputWS == inputWS) {
      if (hasDx) {
        bufferWS = inputWS->clone();
      }
      inputWS *= factor;
    } else {
      outputWS = inputWS * factor;
    }
  } else {

    progress.report("Adding factor...");

    if (outputWS == inputWS) {
      if (hasDx) {
        bufferWS = inputWS->clone();
      }
      inputWS += factor;
    } else {
      outputWS = inputWS + factor;
    }
  }

  progress.report();

  // If there are any Dx values in the input workspace, then
  // copy them across. We check only the first spectrum.
  if (hasDx) {
    if (!inPlace) {
      bufferWS = inputWS;
    }
    for (size_t index = 0; index < bufferWS->getNumberHistograms(); ++index) {
      outputWS->setSharedDx(index, bufferWS->sharedDx(index));
    }
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
