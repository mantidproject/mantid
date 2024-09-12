// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/MDFramesToSpecialCoordinateSystem.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"

namespace Mantid::DataObjects {

/**
 * Get the Special Coordinate System based on the MDFrame information.
 * @param workspace: the workspace which is being queried
 * @returns either a special coordinate or an empty optional
 */
std::optional<Mantid::Kernel::SpecialCoordinateSystem>
MDFramesToSpecialCoordinateSystem::operator()(const Mantid::API::IMDWorkspace *workspace) const {
  // Make sure that the workspaces are either an MDHisto or MDEvent workspaces
  if (!dynamic_cast<const Mantid::API::IMDEventWorkspace *>(workspace) &&
      !dynamic_cast<const Mantid::API::IMDHistoWorkspace *>(workspace)) {
    throw std::invalid_argument("Error in MDFrameFromWorkspace: Can only "
                                "extract MDFrame from MDEvent and MDHisto "
                                "workspaces");
  }

  // Requirements for the special coordinate are: If there are more than one
  // Q-compatible (QSample, QLab, HKL) dimension, then they have to be identical
  // This dimension will define the special coordinate system. Otherwise, we
  // don't have a special coordinate system

  std::optional<Mantid::Kernel::SpecialCoordinateSystem> qFrameType =
      Mantid::Kernel::SpecialCoordinateSystem::None; // Set to none just to have
                                                     // it initialized
  auto hasQFrame = false;
  auto isUnknown = false;
  for (size_t dimIndex = 0; dimIndex < workspace->getNumDims(); ++dimIndex) {
    auto dimension = workspace->getDimension(dimIndex);
    auto &frame = dimension->getMDFrame();
    // Check for QCompatibility. This has gotten a bit more complicated than
    // necessary since the std::optional
    // caused a GCC error, when it was not initialized. Using -Wuninitialized
    // didn't make the compiler happy.
    if (frame.getMDUnit().isQUnit()) {
      auto specialCoordinteSystem = frame.equivalientSpecialCoordinateSystem();
      if (hasQFrame) {
        checkQCompatibility(specialCoordinteSystem, qFrameType);
      }
      qFrameType = specialCoordinteSystem;
      hasQFrame = true;
    }

    isUnknown = isUnknownFrame(dimension);
  }

  std::optional<Mantid::Kernel::SpecialCoordinateSystem> output;
  if (hasQFrame) {
    output = qFrameType;
  } else {
    // If the frame is unknown then keep the optional empty
    if (!isUnknown) {
      output = Mantid::Kernel::SpecialCoordinateSystem::None;
    }
  }

  return output;
}

/**
 * Make sure that the QFrame types are the same.
 * @param specialCoordinateSystem: the q frame type to test.
 * @param qFrameType: the current q frame type
 */
void MDFramesToSpecialCoordinateSystem::checkQCompatibility(
    Mantid::Kernel::SpecialCoordinateSystem specialCoordinateSystem,
    std::optional<Mantid::Kernel::SpecialCoordinateSystem> qFrameType) const {
  if (qFrameType) {
    if (specialCoordinateSystem != qFrameType.value()) {
      throw std::invalid_argument("Error in MDFrameFromWorkspace: Coordinate "
                                  "system in the different dimensions don't "
                                  "match.");
    }
  }
}

/* Checks if an MDFrame is an UnknownFrame
 * @param dimension: a dimension
 * @returns true if the MDFrame is of UnknownFrame type.
 */
bool MDFramesToSpecialCoordinateSystem::isUnknownFrame(
    const Mantid::Geometry::IMDDimension_const_sptr &dimension) const {
  Mantid::Geometry::MDFrame_uptr replica(dimension->getMDFrame().clone());
  auto isUnknown = false;
  if (dynamic_cast<Mantid::Geometry::UnknownFrame *>(replica.get())) {
    isUnknown = true;
  }
  return isUnknown;
}
} // namespace Mantid::DataObjects
