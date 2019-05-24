// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Scale.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Scale)

using namespace Kernel;
using namespace API;

namespace {
namespace PropertyNames {
const std::string INPUT_WORKSPACE("InputWorkspace");
const std::string OUTPUT_WORKSPACE("OutputWorkspace");
const std::string FACTOR("Factor");
const std::string OPERATION("Operation");
} // namespace PropertyNames
namespace Operation {
const std::string MULT("Multiply");
const std::string ADD("Add");
} // namespace Operation
} // anonymous namespace

void Scale::init() {
  declareProperty(make_unique<WorkspaceProperty<>>(
      PropertyNames::INPUT_WORKSPACE, "", Direction::Input));
  declareProperty(make_unique<WorkspaceProperty<>>(
      PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output));

  declareProperty(PropertyNames::FACTOR, 1.0,
                  "The value by which to scale the input workspace");
  std::vector<std::string> operations{Operation::MULT, Operation::ADD};
  declareProperty(PropertyNames::OPERATION, Operation::MULT,
                  boost::make_shared<StringListValidator>(operations),
                  "Whether to multiply by, or add factor");
}

std::map<std::string, std::string> Scale::validateInputs() {
  std::map<std::string, std::string> result;

  // don't allow adding with EventWorkspace
  if (getPropertyValue(PropertyNames::OPERATION) == Operation::ADD) {
    MatrixWorkspace_const_sptr inputWS =
        getProperty(PropertyNames::INPUT_WORKSPACE);
    const auto eventWS =
        boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(inputWS);
    if (bool(eventWS)) {
      result[PropertyNames::INPUT_WORKSPACE] = "Cannot Add to EventWorkspace";
    }
  }

  return result;
}

void Scale::exec() {
  MatrixWorkspace_sptr inputWS = getProperty(PropertyNames::INPUT_WORKSPACE);
  MatrixWorkspace_sptr outputWS = getProperty(PropertyNames::OUTPUT_WORKSPACE);
  const auto inPlace = bool(outputWS == inputWS);
  if (!inPlace)
    outputWS = inputWS->clone();

  const double factor = getProperty(PropertyNames::FACTOR);
  const std::string operation = getPropertyValue(PropertyNames::OPERATION);

  // We require a copy of the workspace if there is Dx
  MatrixWorkspace_sptr bufferWS;
  const auto hasDx = inputWS->hasDx(0);
  if (hasDx) {
    if (inPlace)
      bufferWS = inputWS->clone();
    else
      bufferWS = inputWS;
  }

  Progress progress(this, 0.0, 1.0, 2);

  if (operation == Operation::MULT) {
    progress.report("Multiplying factor...");

    outputWS *= factor;
  } else if (operation == Operation::ADD) {
    progress.report("Adding factor...");

    outputWS += factor;
  } else { // should be impossible to get here
    std::stringstream msg;
    msg << "Do not know how to \"" << operation << "\"";
    throw std::runtime_error(msg.str());
  }

  progress.report();

  // If there are any Dx values in the input workspace, then
  // copy them across. We check only the first spectrum.
  if (hasDx) {
    const auto numHist = bufferWS->getNumberHistograms();
    for (size_t index = 0; index < numHist; ++index) {
      outputWS->setSharedDx(index, bufferWS->sharedDx(index));
    }
  }

  setProperty(PropertyNames::OUTPUT_WORKSPACE, outputWS);
}

} // namespace Algorithms
} // namespace Mantid
