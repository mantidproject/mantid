// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_MDFRAMESTOSPECIALCOORDINATESYTEM_H_
#define MANTID_DATAOBJECTS_MDFRAMESTOSPECIALCOORDINATESYTEM_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/System.h"
#include "boost/optional.hpp"
namespace Mantid {
namespace DataObjects {

/** MDFrameFromMDWorkspace: Each dimension of the MDWorkspace contains an
    MDFrame. The acutal frame which is common to all dimensions is extracted.
*/
class DLLExport MDFramesToSpecialCoordinateSystem {
public:
  boost::optional<Mantid::Kernel::SpecialCoordinateSystem>
  operator()(const Mantid::API::IMDWorkspace *workspace) const;

private:
  void checkQCompatibility(
      Mantid::Kernel::SpecialCoordinateSystem specialCoordinateSystem,
      boost::optional<Mantid::Kernel::SpecialCoordinateSystem> qFrameType)
      const;
  bool
  isUnknownFrame(Mantid::Geometry::IMDDimension_const_sptr dimension) const;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_MDFRAMESTOSPECIALCOORDINATESYTEM_H_ */
