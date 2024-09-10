// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <optional>
namespace Mantid {
namespace DataObjects {

/** MDFrameFromMDWorkspace: Each dimension of the MDWorkspace contains an
    MDFrame. The acutal frame which is common to all dimensions is extracted.
*/
class MANTID_DATAOBJECTS_DLL MDFramesToSpecialCoordinateSystem {
public:
  std::optional<Mantid::Kernel::SpecialCoordinateSystem> operator()(const Mantid::API::IMDWorkspace *workspace) const;

private:
  void checkQCompatibility(Mantid::Kernel::SpecialCoordinateSystem specialCoordinateSystem,
                           std::optional<Mantid::Kernel::SpecialCoordinateSystem> qFrameType) const;
  bool isUnknownFrame(const Mantid::Geometry::IMDDimension_const_sptr &dimension) const;
};

} // namespace DataObjects
} // namespace Mantid
