// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/ShapeRotation.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Logger.h"

#include <stdexcept>

namespace Mantid::Geometry {

namespace {
Kernel::Logger g_log("ShapeRotation");

const Kernel::Matrix<double> &identityMatrix() {
  static const Kernel::Matrix<double> identity(3, 3, true);
  return identity;
}
} // namespace

Kernel::Matrix<double> outstandingGoniometerRotation(const IObject &shape, const Kernel::Matrix<double> &goniometerR) {
  const Kernel::Matrix<double> &alreadyApplied = shape.getAppliedRotation();
  if (alreadyApplied == identityMatrix()) {
    return goniometerR;
  }
  g_log.information("The sample shape already carries a rotation, so only the remainder of the "
                    "goniometer rotation is applied");
  // Tprime rather than Invert: these are orthonormal, so the transpose is the inverse exactly, and
  // Invert would both mutate the matrix and return a determinant we have no use for.
  return goniometerR * alreadyApplied.Tprime();
}

std::shared_ptr<IObject> getLabFrameShape(const IObject &shape, const Kernel::Matrix<double> &goniometerR) {
  const auto outstanding = outstandingGoniometerRotation(shape, goniometerR);
  if (outstanding == identityMatrix()) {
    // Already in the lab frame. Returning the clone unrotated is not a shortcut - rotating by an
    // identity built from R and its transpose would perturb the last bits for no gain.
    return std::shared_ptr<IObject>(shape.clone());
  }

  if (const auto *csgShape = dynamic_cast<const CSGObject *>(&shape)) {
    // Rotating a CSG shape means rewriting its definition and rebuilding from that, so a shape
    // assembled surface by surface rather than parsed - ShapeFactory::createSphere and
    // createHexahedralShape both do this - has nothing to rewrite. Rebasing an empty string yields
    // a bare pair of tags that will not parse, and createShape answers that with an empty shape:
    // silently no sample at all, where the caller asked for a rotated one. Say so instead.
    //
    // Unlike the untouched MeshObject2D below, this is not a shape that has no frame to move into.
    // It is a shape that can be rotated and whose rotation we have no way to express, which is a
    // defect rather than a definition, so it is worth an exception rather than a warning.
    if (csgShape->getShapeXML().empty()) {
      throw std::invalid_argument("The sample shape ('" + shape.id() +
                                  "') carries no XML definition, so it cannot be rotated into the lab frame. It was "
                                  "built directly from surfaces rather than parsed from a shape definition. Define "
                                  "the sample with SetSample or CreateSampleShape to make it rotatable.");
    }

    // Rebase the XML so the baked part becomes exactly goniometerR, preserving any rotation of the
    // shape within its own frame, then rebuild. createShape carries over neither the material nor
    // the id.
    const auto xml = ShapeFactory().rebakeGoniometer(goniometerR, csgShape->getShapeXML(), shape.getAppliedRotation());
    auto labShape = ShapeFactory().createShape(xml, false);
    labShape->setMaterial(shape.material());
    labShape->setID(shape.id());
    return labShape;
  }

  auto labShape = std::shared_ptr<IObject>(shape.clone());
  if (auto *meshShape = dynamic_cast<MeshObject *>(labShape.get())) {
    meshShape->bakeGoniometerRotation(outstanding);
    return labShape;
  }

  // A shape with no rotation mechanism at all - MeshObject2D, the flat plate, is the one in the
  // tree. Such a shape offers no way to express a rotation, so by definition it is taken to be
  // defined in the frame it is meant to be used in, and is returned as it stands. Say so, because
  // the caller asked for a rotation and is not getting one.
  g_log.warning("The sample shape is of a type that cannot be rotated ('" + shape.id() +
                "'), so it is taken to be defined in the lab frame already and is used unchanged. "
                "The goniometer rotation on the run has not been applied to it.");
  return labShape;
}

} // namespace Mantid::Geometry
