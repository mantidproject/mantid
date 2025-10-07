// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/V3D.h"

namespace Mantid {
namespace DataObjects {

constexpr size_t PEAK_ELLIPSOID_DIMS{3};
typedef std::array<double, PEAK_ELLIPSOID_DIMS> PeakEllipsoidExtent;
typedef std::array<Kernel::V3D, PEAK_ELLIPSOID_DIMS> PeakEllipsoidFrame;
class PeakShapeEllipsoid;

using PeakShapeEllipsoid_sptr = std::shared_ptr<PeakShapeEllipsoid>;
using PeakShapeEllipsoid_const_sptr = std::shared_ptr<const PeakShapeEllipsoid>;

} // namespace DataObjects
} // namespace Mantid
