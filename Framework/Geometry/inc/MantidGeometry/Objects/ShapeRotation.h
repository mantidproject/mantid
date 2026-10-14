// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"

#include <memory>

namespace Mantid {
namespace Geometry {

class IObject;

/* Reconcile a sample shape with the goniometer rotation of the workspace holding it: both ask the
 * shape which frame it is already in rather than rotating blindly. See IObject::getAppliedRotation.
 */

/// The shape as it sits in the lab frame: a clone rotated by whatever part of goniometerR it does
/// not already carry, reporting goniometerR as its bake and keeping its material, id and any
/// definition-frame rotation. Prefer this to rotating a shape by hand.
///
/// A shape with no rotation mechanism at all - MeshObject2D - is by definition already in the frame
/// it is meant to be used in, so it comes back unchanged with a warning.
///
/// @throws std::invalid_argument if a rotation is outstanding on a CSGObject assembled from surfaces
/// rather than parsed, so carrying no XML to rewrite. Such a shape could be rotated but offers no
/// way to express it, so it is reported rather than quietly left where it was.
MANTID_GEOMETRY_DLL std::shared_ptr<IObject> getLabFrameShape(const IObject &shape,
                                                              const Kernel::Matrix<double> &goniometerR);

/// The part of goniometerR that the shape does not already carry. Identity when the shape is
/// already in the lab frame, goniometerR when it is in its own frame. For the caller that wants the
/// matrix rather than the shape - rasterising in the shape's own frame, where the grid is tight,
/// and rotating after.
MANTID_GEOMETRY_DLL Kernel::Matrix<double> outstandingGoniometerRotation(const IObject &shape,
                                                                         const Kernel::Matrix<double> &goniometerR);

} // namespace Geometry
} // namespace Mantid
