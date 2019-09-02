// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertToDistribution.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidKernel/CompositeValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToDistribution)

using namespace API;

void ConvertToDistribution::init() {
  auto wsValidator = boost::make_shared<Kernel::CompositeValidator>();
  wsValidator->add<HistogramValidator>();
  wsValidator->add<RawCountValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "Workspace", "", Kernel::Direction::InOut, wsValidator),
                  "The name of the workspace to convert.");
}

void ConvertToDistribution::exec() {
  MatrixWorkspace_sptr workspace = getProperty("Workspace");

  WorkspaceHelpers::makeDistribution(workspace);
}

std::map<std::string, std::string> ConvertToDistribution::validateInputs() {
  std::map<std::string, std::string> errors;

  MatrixWorkspace_sptr workspace = getProperty("Workspace");
  if (workspace && workspace->id() == "EventWorkspace") {
    errors["Workspace"] = "Event workspaces cannot be directly converted to "
                          "distributions.";
  }

  return errors;
}

} // namespace Algorithms
} // namespace Mantid
