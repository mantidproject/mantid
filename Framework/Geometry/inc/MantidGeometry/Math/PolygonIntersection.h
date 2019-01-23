// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_POLYGONINTERSECTION_H_
#define MANTID_GEOMETRY_POLYGONINTERSECTION_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"

namespace Mantid {
namespace Geometry {
//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
class ConvexPolygon;

/// Compute the instersection of two convex polygons.
bool MANTID_GEOMETRY_DLL intersection(const ConvexPolygon &P,
                                      const ConvexPolygon &Q,
                                      ConvexPolygon &out);

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_POLYGONINTERSECTION_H_ */
