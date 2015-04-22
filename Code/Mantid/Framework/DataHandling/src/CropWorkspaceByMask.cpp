#include "MantidDataHandling/CropWorkspaceByMask.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CropWorkspaceByMask)

//----------------------------------------------------------------------------------------------
/// Constructor
CropWorkspaceByMask::CropWorkspaceByMask() {}

//----------------------------------------------------------------------------------------------
/// Destructor
CropWorkspaceByMask::~CropWorkspaceByMask() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CropWorkspaceByMask::name() const {
  return "CropWorkspaceByMask";
}

/// Algorithm's version for identification. @see Algorithm::version
int CropWorkspaceByMask::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string CropWorkspaceByMask::category() const {
  return "Transforms\\Splitting";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CropWorkspaceByMask::summary() const {
  return "Extracts unmasked spectra from a workspace and places them in a new "
         "workspace.";
}

//----------------------------------------------------------------------------------------------
/// Initialize the algorithm's properties.
void CropWorkspaceByMask::init() {
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
/// Execute the algorithm.
void CropWorkspaceByMask::exec() {
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr maskedWorkspace = getProperty("MaskedWorkspace");

  if ( !maskedWorkspace ) maskedWorkspace = inputWorkspace;

  std::vector<size_t> indices;
  makeIndexList(indices, maskedWorkspace.get());

  // Number of spectra in the cropped workspace.
  size_t nSpectra = indices.size();
  // Number of bins/data points in the cropped workspace.
  size_t nBins = inputWorkspace->blocksize();
  size_t histogram = inputWorkspace->isHistogramData()? 1 : 0;

  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(
      inputWorkspace, nSpectra, nBins, nBins - histogram);

}

//----------------------------------------------------------------------------------------------
/// Fill in a vector with spectra indices to be extracted.
/// @param indices :: A reference to a vector to fill with the indices.
/// @param maskedWorkspace :: A workspace with masking information.
void CropWorkspaceByMask::makeIndexList(
    std::vector<size_t> &indices, const API::MatrixWorkspace *maskedWorkspace) {
  auto mask = dynamic_cast<const DataObjects::MaskWorkspace *>(maskedWorkspace);
  if (mask) {
    for (size_t i = 0; i < mask->getNumberHistograms(); ++i) {
      if (mask->readY(i)[0] == 0.0) {
        indices.push_back(i);
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

      if (det->isMasked()) {
        indices.push_back(i);
      }
    }
  }
}

} // namespace DataHandling
} // namespace Mantid