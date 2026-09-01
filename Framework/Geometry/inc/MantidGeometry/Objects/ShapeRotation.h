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

/** Reconcile a sample shape with the goniometer rotation of the workspace holding it.
 *
 * A workspace can arrive with its sample shape in either frame. CopySample bakes the destination's
 * goniometer into the shape, so the shape is already in the lab frame, while SetGoniometer on its
 * own leaves the shape untouched in its own frame. Both leave the same goniometer on the run, so
 * the run alone cannot distinguish them - applying R unconditionally rotates an already-rotated
 * shape a second time. Asking the shape what it already carries, through getAppliedRotation,
 * resolves it.
 */

/// The shape as it sits in the lab frame: a clone rotated by whatever part of goniometerR it does
/// not already carry, and reporting goniometerR as its baked rotation. Returns a plain clone when
/// the shape is already in the lab frame, so the common baked case costs nothing extra. Any
/// definition-frame rotation the shape carries is preserved, as is its material.
///
/// Prefer this to rotating a shape by hand. The one case that legitimately wants the matrix instead
/// is a caller that rasterises in the shape's own frame - where an axis-aligned voxel grid is tight
/// - and rotates the result afterwards.
///
/// A shape offering no rotation mechanism at all - MeshObject2D, the flat plate - is by definition
/// already in the frame it is meant to be used in, so it is returned unchanged with a warning.
///
/// @throws std::invalid_argument if the shape is a CSGObject that was assembled from surfaces rather
/// than parsed, so carries no XML to rewrite, and a rotation is actually outstanding. Such a shape
/// can be rotated in principle but offers no way to express it, so it is reported rather than
/// quietly left where it was.
MANTID_GEOMETRY_DLL std::shared_ptr<IObject> getLabFrameShape(const IObject &shape,
                                                              const Kernel::Matrix<double> &goniometerR);

/// The part of goniometerR that the shape does not already carry. Identity when the shape is
/// already in the lab frame, goniometerR when it is in its own frame.
MANTID_GEOMETRY_DLL Kernel::Matrix<double> outstandingGoniometerRotation(const IObject &shape,
                                                                         const Kernel::Matrix<double> &goniometerR);

} // namespace Geometry
} // namespace Mantid
