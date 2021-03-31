// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidMDAlgorithms/DisplayNormalizationSetter.h"

namespace Mantid {
namespace MDAlgorithms {

/**
 * Sets the display normalization
 * @param mdWorkspace: the MDWorkspace which needs its normalizations flags set
 * @param underlyingWorkspace: the underlying workspace, ie EventWorkspace or
 * Workspace2D
 * @param isQ: if the transform is Q or not
 * @param mode: the energy transfer mode
 */
void DisplayNormalizationSetter::operator()(const Mantid::API::IMDWorkspace_sptr &mdWorkspace,
                                            const Mantid::API::MatrixWorkspace_sptr &underlyingWorkspace, bool isQ,
                                            const Mantid::Kernel::DeltaEMode::Type &mode) {
  if (std::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(mdWorkspace)) {
    setNormalizationMDEvent(mdWorkspace, underlyingWorkspace, isQ, mode);
  } else {
    throw std::runtime_error("Setting the display normaliztion is currently "
                             "only implemented for MDEvent Workspaces");
  }
}

/**
 * Sets the display normalization for MDEventWorkspaces
 * @param mdWorkspace: the MDWorkspace which needs its normalizations flags set
 * @param underlyingWorkspace: the underlying workspace, ie EventWorkspace or
 * Workspace2D
 * @param isQ: if the transform is Q or not
 * @param mode: the energy transfer mode
 */
void DisplayNormalizationSetter::setNormalizationMDEvent(const Mantid::API::IMDWorkspace_sptr &mdWorkspace,
                                                         const Mantid::API::MatrixWorkspace_sptr &underlyingWorkspace,
                                                         bool isQ, const Mantid::Kernel::DeltaEMode::Type &mode) {

  auto isEventWorkspace =
      static_cast<bool>(std::dynamic_pointer_cast<Mantid::DataObjects::EventWorkspace>(underlyingWorkspace));
  Mantid::API::MDNormalization displayNormalization(Mantid::API::MDNormalization::VolumeNormalization);
  Mantid::API::MDNormalization displayNormalizationHisto(Mantid::API::MDNormalization::VolumeNormalization);

  // If is not Q
  if (!isQ) {
    displayNormalizationHisto = Mantid::API::MDNormalization::VolumeNormalization;
    // 2. If Energy mode is elastic --> Volume Normalization
  } else if (mode == Mantid::Kernel::DeltaEMode::Elastic) {
    displayNormalizationHisto = Mantid::API::MDNormalization::VolumeNormalization;
    // 3. If Energy is inelastic and underlying workspace is event workspace -->
    // use no normalization
  } else if (isEventWorkspace) {
    displayNormalizationHisto = Mantid::API::MDNormalization::NoNormalization;
    // 4. If Eneregy is inelastic and underlying workspace is other workspace
    // --> use num event normalization
  } else {
    displayNormalizationHisto = Mantid::API::MDNormalization::NumEventsNormalization;
  }

  applyNormalizationMDEvent(std::move(mdWorkspace), displayNormalization, displayNormalizationHisto);
}

/**
 * Apply the normalization to an MDEvent Workspace
 * @param mdWorkspace: the workspace which gets its normalization tags set
 * @param displayNormalization: the display normalization for the MDEvent
 * workspace
 * @param displayNormalizationHisto: the display normalization for derived
 * MDHisto workspaces
 */
void DisplayNormalizationSetter::applyNormalizationMDEvent(const Mantid::API::IMDWorkspace_sptr &mdWorkspace,
                                                           Mantid::API::MDNormalization displayNormalization,
                                                           Mantid::API::MDNormalization displayNormalizationHisto) {
  auto ws = std::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(mdWorkspace);
  ws->setDisplayNormalization(displayNormalization);
  ws->setDisplayNormalizationHisto(displayNormalizationHisto);
}
} // namespace MDAlgorithms
} // namespace Mantid
