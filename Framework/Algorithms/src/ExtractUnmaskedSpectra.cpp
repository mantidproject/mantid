// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ExtractUnmaskedSpectra.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractUnmaskedSpectra)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ExtractUnmaskedSpectra::name() const {
  return "ExtractUnmaskedSpectra";
}

/// Algorithm's version for identification. @see Algorithm::version
int ExtractUnmaskedSpectra::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ExtractUnmaskedSpectra::category() const {
  return "Transforms\\Splitting";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ExtractUnmaskedSpectra::summary() const {
  return "Extracts unmasked spectra from a workspace and places them in a new "
         "workspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ExtractUnmaskedSpectra::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input workspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(
          "MaskWorkspace", "", Direction::Input, API::PropertyMode::Optional),
      "An optional mask workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ExtractUnmaskedSpectra::exec() {
  // Get the input workspace
  API::MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  // Get the masked workspace (optional).
  API::MatrixWorkspace_sptr maskedWorkspace = getProperty("MaskWorkspace");

  // Define the mask
  API::MatrixWorkspace_sptr mask;
  if (maskedWorkspace) {
    if (boost::dynamic_pointer_cast<API::IMaskWorkspace>(maskedWorkspace)) {
      mask = maskedWorkspace;
    } else {
      auto extractMask = createChildAlgorithm("ExtractMask");
      extractMask->setProperty("InputWorkspace", maskedWorkspace);
      extractMask->executeAsChildAlg();
      mask = extractMask->getProperty("OutputWorkspace");
    }
  } else {
    auto extractMask = createChildAlgorithm("ExtractMask");
    extractMask->setProperty("InputWorkspace", inputWorkspace);
    extractMask->executeAsChildAlg();
    mask = extractMask->getProperty("OutputWorkspace");
  }

  std::vector<size_t> indicesToExtract;
  auto nSpectra = inputWorkspace->getNumberHistograms();
  indicesToExtract.reserve(nSpectra);

  // Find the unmasked spectra
  for (size_t index = 0; index < nSpectra; ++index) {
    if (mask->readY(index)[0] < 1.0) {
      indicesToExtract.push_back(index);
    }
  }

  // Extract the unmasked spectra.
  auto extractSpectra = createChildAlgorithm("ExtractSpectra");
  extractSpectra->setProperty("InputWorkspace", inputWorkspace);
  extractSpectra->setProperty("WorkspaceIndexList", indicesToExtract);
  extractSpectra->executeAsChildAlg();

  // Store the output
  API::MatrixWorkspace_sptr outputWorkspace =
      extractSpectra->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
