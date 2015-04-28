#include "MantidAlgorithms/RemoveMaskedSpectra.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/MaskWorkspace.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RemoveMaskedSpectra)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
RemoveMaskedSpectra::RemoveMaskedSpectra() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
RemoveMaskedSpectra::~RemoveMaskedSpectra() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string RemoveMaskedSpectra::name() const {
  return "RemoveMaskedSpectra";
}

/// Algorithm's version for identification. @see Algorithm::version
int RemoveMaskedSpectra::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string RemoveMaskedSpectra::category() const {
  return "Transforms\\Splitting";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string RemoveMaskedSpectra::summary() const {
  return "Extracts unmasked spectra from a workspace and places them in a new "
         "workspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void RemoveMaskedSpectra::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(new WorkspaceProperty<>("MaskedWorkspace", "",
                                          Direction::Input,
                                          PropertyMode::Optional),
                  "If given but not as a MaskWorkspace, the masking from "
                  "this workspace will be used. If given as a "
                  "MaskWorkspace, the masking is read from its Y values.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void RemoveMaskedSpectra::exec() {
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr maskedWorkspace = getProperty("MaskedWorkspace");

  if (!maskedWorkspace){
    maskedWorkspace = inputWorkspace;
  } else if (inputWorkspace->getNumberHistograms() != maskedWorkspace->getNumberHistograms()) {
    throw std::runtime_error("Masked workspace has a different number of spectra.");
  }

  // Find indices of the unmasked spectra.
  std::vector<specid_t> indices;
  makeIndexList(indices, maskedWorkspace.get());

  auto extract = createChildAlgorithm("ExtractSpectra", 0, 1);
  extract->initialize();
  extract->setRethrows(true);

  extract->setProperty("InputWorkspace", inputWorkspace);
  extract->setProperty("SpectrumList", indices);

  extract->execute();

  MatrixWorkspace_sptr outputWorkspace = extract->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outputWorkspace);
}

//----------------------------------------------------------------------------------------------
/// Fill in a vector with spectra indices to be extracted.
/// @param indices :: A reference to a vector to fill with the indices.
/// @param maskedWorkspace :: A workspace with masking information.
void RemoveMaskedSpectra::makeIndexList(
    std::vector<specid_t> &indices, const API::MatrixWorkspace *maskedWorkspace) {
  auto mask = dynamic_cast<const DataObjects::MaskWorkspace *>(maskedWorkspace);
  if (mask) {
    for (size_t i = 0; i < mask->getNumberHistograms(); ++i) {
      if (mask->readY(i)[0] == 0.0) {
        indices.push_back(static_cast<specid_t>(i));
      }
    }
  } else {
    for (size_t i = 0; i < maskedWorkspace->getNumberHistograms(); ++i) {
      Geometry::IDetector_const_sptr det;
      try {
        det = maskedWorkspace->getDetector(i);
      } catch (Exception::NotFoundError &) {
        continue;
      }
      if (!det->isMasked()) {
        indices.push_back(static_cast<specid_t>(i));
      }
    }
  }
}

} // namespace Algorithms
} // namespace Mantid