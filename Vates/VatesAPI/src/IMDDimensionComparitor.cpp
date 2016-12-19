#include "MantidVatesAPI/IMDDimensionComparitor.h"

namespace Mantid {
namespace VATES {

IMDDimensionComparitor::IMDDimensionComparitor(
    Mantid::API::IMDWorkspace_sptr workspace)
    : m_workspace(std::move(workspace)) {}

IMDDimensionComparitor::~IMDDimensionComparitor() {}

bool IMDDimensionComparitor::isXDimension(
    Mantid::Geometry::IMDDimension_const_sptr queryDimension) {
  // Compare dimensions on the basis of their ids.
  Mantid::Geometry::IMDDimension_const_sptr actualXDimension =
      m_workspace->getXDimension();
  return queryDimension->getDimensionId() == actualXDimension->getDimensionId();
}

bool IMDDimensionComparitor::isYDimension(
    Mantid::Geometry::IMDDimension_const_sptr queryDimension) {
  Mantid::Geometry::IMDDimension_const_sptr actualYDimension =
      m_workspace->getYDimension();
  if (actualYDimension) {
    // Compare dimensions on the basis of their ids.
    return queryDimension->getDimensionId() ==
           actualYDimension->getDimensionId();
  } else {
    return false; // MDImages may have 1 dimension or more.
  }
}

bool IMDDimensionComparitor::isZDimension(
    Mantid::Geometry::IMDDimension_const_sptr queryDimension) {
  Mantid::Geometry::IMDDimension_const_sptr actualZDimension =
      m_workspace->getZDimension();
  if (actualZDimension) {
    // Compare dimensions on the basis of their ids.
    return queryDimension->getDimensionId() ==
           actualZDimension->getDimensionId();
  } else {
    return false; // MDImages may have 1 dimension or more.
  }
}

bool IMDDimensionComparitor::istDimension(
    Mantid::Geometry::IMDDimension_const_sptr queryDimension) {
  Mantid::Geometry::IMDDimension_const_sptr actualtDimension =
      m_workspace->getTDimension();
  if (actualtDimension) {
    // Compare dimensions on the basis of their ids.
    return queryDimension->getDimensionId() ==
           actualtDimension->getDimensionId();
  } else {
    return false; // MDImages may have 1 dimension or more.
  }
}
}
}
