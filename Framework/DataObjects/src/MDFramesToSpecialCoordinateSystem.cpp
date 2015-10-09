#include "MantidDataObjects/MDFramesToSpecialCoordinateSystem.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"

namespace Mantid {
namespace DataObjects {

MDFramesToSpecialCoordinateSystem::MDFramesToSpecialCoordinateSystem() {}

MDFramesToSpecialCoordinateSystem::~MDFramesToSpecialCoordinateSystem() {}

/**
 * Return a q frame which is common to all dimensions
 * @param workspace: the workspace which is being queried
 * @returns either a special coordinate or an empty optional
 */
boost::optional<Mantid::Kernel::SpecialCoordinateSystem>
    MDFramesToSpecialCoordinateSystem::
    operator()(const Mantid::API::IMDWorkspace *workspace) const {
  // Make sure that the workspaces are either an MDHisto or MDEvent workspaces
  if (!dynamic_cast<const Mantid::API::IMDEventWorkspace *>(workspace) &&
      !dynamic_cast<const Mantid::API::IMDHistoWorkspace *>(workspace)) {
    throw std::invalid_argument("Error in MDFrameFromWorkspace: Can only "
                                "extract MDFrame from MDEvent and MDHisto "
                                "workspaces");
  }

  // Coordiante system
  // Extract expected special coordinate system
  auto coordinateSystem = extractCoordinateSystem(workspace->getDimension(0));
  auto isUnknown = false;
  for (size_t dimIndex = 0; dimIndex < workspace->getNumDims(); ++dimIndex) {
    auto current = extractCoordinateSystem(workspace->getDimension(dimIndex));
    if (coordinateSystem == current) {
      coordinateSystem = current;
    } else {
      throw std::invalid_argument("Error in MDFrameFromWorkspace: Coordinate "
                                  "system in the different dimensions don't "
                                  "match.");
    }

    // Check if the Frame is of UnknownFrame Type
    isUnknown = isUnknownFrame(workspace->getDimension(dimIndex));
  }

  boost::optional<Mantid::Kernel::SpecialCoordinateSystem> output;
  if (!isUnknown) {
    output = coordinateSystem;
  }
  return output;
}

/**
 * Extracts the QFrame coordinate system form the dimension
 * @param dimension: a dimension
 * @returns a Qframe identifier
 */
Mantid::Kernel::SpecialCoordinateSystem
MDFramesToSpecialCoordinateSystem::extractCoordinateSystem(
    Mantid::Geometry::IMDDimension_const_sptr dimension) const {
  auto &frame = dimension->getMDFrame();
  return frame.equivalientSpecialCoordinateSystem();
}

/*
 * Checks if an MDFrame is an UnknownFrame
 * @param dimension: a dimension
 * @returns true if the MDFrame is of UnknownFrame type.
 */
bool MDFramesToSpecialCoordinateSystem::isUnknownFrame(
    Mantid::Geometry::IMDDimension_const_sptr dimension) const {
  Mantid::Geometry::MDFrame_uptr replica(dimension->getMDFrame().clone());
  auto isUnknown = false;
  if (dynamic_cast<Mantid::Geometry::UnknownFrame *>(replica.get())) {
    isUnknown = true;
  }
  return isUnknown;
}
}
}