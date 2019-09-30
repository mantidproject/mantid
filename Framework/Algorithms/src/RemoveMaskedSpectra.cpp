// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RemoveMaskedSpectra.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/MaskWorkspace.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RemoveMaskedSpectra)

/// Algorithms name for identification. @see Algorithm::name
const std::string RemoveMaskedSpectra::name() const {
  return "RemoveMaskedSpectra";
}

/// Algorithm's version for identification. @see Algorithm::version
int RemoveMaskedSpectra::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RemoveMaskedSpectra::category() const {
  return "Transforms\\Splitting";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string RemoveMaskedSpectra::summary() const {
  return "Extracts unmasked spectra from a workspace and places them in a new "
         "workspace.";
}

/** Initialize the algorithm's properties.
 */
void RemoveMaskedSpectra::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("MaskedWorkspace", "",
                                                        Direction::Input,
                                                        PropertyMode::Optional),
                  "If given but not as a MaskWorkspace, the masking from "
                  "this workspace will be used. If given as a "
                  "MaskWorkspace, the masking is read from its Y values.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "An output workspace.");
}

/** Execute the algorithm.
 */
void RemoveMaskedSpectra::exec() {
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr maskedWorkspace = getProperty("MaskedWorkspace");

  if (!maskedWorkspace) {
    maskedWorkspace = inputWorkspace;
  } else if (inputWorkspace->getNumberHistograms() !=
             maskedWorkspace->getNumberHistograms()) {
    throw std::runtime_error(
        "Masked workspace has a different number of spectra.");
  }

  // Find indices of the unmasked spectra.
  std::vector<size_t> indices;
  makeIndexList(indices, maskedWorkspace.get());

  auto extract = createChildAlgorithm("ExtractSpectra", 0, 1);
  extract->initialize();
  extract->setRethrows(true);

  extract->setProperty("InputWorkspace", inputWorkspace);
  extract->setProperty("WorkspaceIndexList", indices);

  extract->execute();

  MatrixWorkspace_sptr outputWorkspace =
      extract->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWorkspace);
}

/// Fill in a vector with spectra indices to be extracted.
/// @param indices :: A reference to a vector to fill with the indices.
/// @param maskedWorkspace :: A workspace with masking information.
void RemoveMaskedSpectra::makeIndexList(
    std::vector<size_t> &indices, const API::MatrixWorkspace *maskedWorkspace) {
  auto mask = dynamic_cast<const DataObjects::MaskWorkspace *>(maskedWorkspace);
  if (mask) {
    for (size_t i = 0; i < mask->getNumberHistograms(); ++i) {
      if (mask->y(i)[0] == 0.0) {
        indices.push_back(i);
      }
    }
  } else {
    const auto &spectrumInfo = maskedWorkspace->spectrumInfo();
    for (size_t i = 0; i < maskedWorkspace->getNumberHistograms(); ++i) {
      if (!spectrumInfo.hasDetectors(i))
        continue;
      if (!spectrumInfo.isMasked(i))
        indices.push_back(i);
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
