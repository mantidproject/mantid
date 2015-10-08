#include "MantidDataObjects/MDFramesToSpecialCoordinateSystem.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include <boost/pointer_cast.hpp>

namespace Mantid {
namespace DataObjects {

MDFramesToSpecialCoordinateSystem::MDFramesToSpecialCoordinateSystem() {}

MDFramesToSpecialCoordinateSystem::~MDFramesToSpecialCoordinateSystem() {}

/**
 * Return a common
 */
Mantid::Kernel::SpecialCoordinateSystem MDFramesToSpecialCoordinateSystem::
operator()(Mantid::API::IMDWorkspace_const_sptr workspace) const {
  // Make sure that the workspaces are either an MDHisto or MDEvent workspaces
  if (!boost::dynamic_pointer_cast<const Mantid::API::IMDEventWorkspace>(workspace) &&
      !boost::dynamic_pointer_cast<const Mantid::API::IMDHistoWorkspace>(workspace)) {
    throw std::invalid_argument("Error in MDFrameFromWorkspace: Can only "
                                "extract MDFrame from MDEvent and MDHisto "
                                "workspaces");
  }

  // Extract expected special coordinate system
  auto coordinteSystem = extractCoordinateSystem(workspace->getDimension(0));
  for (size_t dimIndex = 0; dimIndex < workspace->getNumDims(); ++dimIndex) {
    auto current = extractCoordinateSystem(workspace->getDimension(dimIndex));
    if (coordinteSystem == current) {
      coordinteSystem = current;
    } else {
      throw std::invalid_argument("Error in MDFrameFromWorkspace: Coordinate "
                                  "system in the different dimensions don't "
                                  "match.");
    }
  }
  return coordinteSystem;
}

Mantid::Kernel::SpecialCoordinateSystem
MDFramesToSpecialCoordinateSystem::extractCoordinateSystem(
    Mantid::Geometry::IMDDimension_const_sptr dimension) const {
  auto &frame = dimension->getMDFrame();
  return frame.equivalientSpecialCoordinateSystem();
}
}
}