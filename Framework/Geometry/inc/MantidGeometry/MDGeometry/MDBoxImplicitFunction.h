// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include <boost/tuple/tuple.hpp>
#include <vector>

namespace Mantid {
namespace Geometry {

/** General N-dimensional box implicit function:
 * Defines a cuboid in N dimensions that is aligned with the axes
 * of a MDEventWorkspace.

  @author Janik Zikovsky
  @date 2011-07-21
*/
class DLLExport MDBoxImplicitFunction : public MDImplicitFunction {
public:
  MDBoxImplicitFunction(const Mantid::Kernel::VMD &min, const Mantid::Kernel::VMD &max);

  MDBoxImplicitFunction(const std::vector<coord_t> &min, const std::vector<coord_t> &max);

  double volume() const;

  double fraction(const std::vector<boost::tuple<Mantid::coord_t, Mantid::coord_t>> &boxExtents) const;

private:
  void construct(const Mantid::Kernel::VMD &min, const Mantid::Kernel::VMD &max);

  /// Maximum extents of MDBox
  const Mantid::Kernel::VMD m_max;
  /// Minimum extents of MDBox
  const Mantid::Kernel::VMD m_min;
  /// Box volume
  double m_volume;
};

} // namespace Geometry
} // namespace Mantid
