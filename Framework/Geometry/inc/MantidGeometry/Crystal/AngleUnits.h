// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_ANGLEUNITS_H_
#define MANTID_GEOMETRY_ANGLEUNITS_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/PhysicalConstants.h"
#include <cmath>

namespace Mantid {
namespace Geometry {
/**
  Defines units/enum for Crystal work
 */

/// Degrees to radians conversion factor
constexpr double deg2rad = M_PI / 180.;

/// Radians to degrees conversion factor
constexpr double rad2deg = 180. / M_PI;

/// Flag for angle units used in UnitCell class
enum AngleUnits { angDegrees, angRadians };
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_ANGLEUNITS_H_ */
