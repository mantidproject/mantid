// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Rasterize.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/Track.h"
#include <iostream> // REMOVE

namespace Mantid {
namespace Geometry {

using Geometry::CSGObject;
using Kernel::V3D;

void Raster::reserve(size_t numVolumeElements) {
  l1.reserve(numVolumeElements);
  volume.reserve(numVolumeElements);
  position.reserve(numVolumeElements);
}

namespace { // anonymous

const V3D X_AXIS{ 1, 0, 0 };
const V3D Y_AXIS{ 0, 1, 0 };
const V3D Z_AXIS{ 0, 0, 1 };

void getCylinderParameters(const CSGObject &shape, double &radius,
                           double &height, V3D &centerBottomBase, V3D &axis) {
  // fundamental checks for any object
  if (!shape.hasValidShape())
    throw std::logic_error("Shape[CSGObject] does not have a valid shape");
  if (shape.shape() != Geometry::detail::ShapeInfo::GeometryShape::CYLINDER)
    throw std::logic_error("Shape[CSGObject] is not a cylinder");

  const auto &shapeInfo = shape.shapeInfo();
  radius = shapeInfo.radius();
  height = shapeInfo.height();

  std::vector<V3D> points = shapeInfo.points();
  centerBottomBase = points[0];
  axis = points[1];
}

V3D createPerpendicular(const V3D &z_axis) {
  const std::vector<double> scalars = { fabs(z_axis.scalar_prod(X_AXIS)),
                                        fabs(z_axis.scalar_prod(Y_AXIS)),
                                        fabs(z_axis.scalar_prod(Z_AXIS)) };
  // check against the cardinal axes
  if (scalars[0] == 0.)
    return z_axis.cross_prod(X_AXIS);
  else if (scalars[1] == 0.)
    return z_axis.cross_prod(Y_AXIS);
  else if (scalars[2] == 0.)
    return z_axis.cross_prod(Z_AXIS);

  // see which one is smallest
  if (scalars[0] < scalars[1] && scalars[0] < scalars[2])
    return z_axis.cross_prod(X_AXIS);
  else if (scalars[1] < scalars[0] && scalars[1] < scalars[2])
    return z_axis.cross_prod(Y_AXIS);
  else
    return z_axis.cross_prod(Z_AXIS);
}

} // namespace

namespace Rasterize {

// --------- collection of calculations that convert to CSGObjects and pass the
// work on

Raster calculate(const V3D &beamDirection,
                 const boost::shared_ptr<IObject> shape,
                 const double cubeSizeInMetre) {
  // convert to the underlying CSGObject
  const auto csgshape =
      boost::dynamic_pointer_cast<const Geometry::CSGObject>(shape);
  if (!csgshape)
    throw std::logic_error("Failed to convert IObject to CSGObject");
  if (!(csgshape->hasValidShape()))
    throw std::logic_error("Shape[IObject] does not have a valid shape");

  return calculate(beamDirection, *csgshape, cubeSizeInMetre);
}

Raster calculateCylinder(const V3D &beamDirection,
                         const boost::shared_ptr<Geometry::IObject> shape,
                         const size_t numSlices, const size_t numAnnuli) {
  // convert to the underlying CSGObject
  const auto csgshape =
      boost::dynamic_pointer_cast<const Geometry::CSGObject>(shape);
  if (!csgshape)
    throw std::logic_error("Failed to convert IObject to CSGObject");
  if (!(csgshape->hasValidShape()))
    throw std::logic_error("Shape[IObject] does not have a valid shape");

  return calculateCylinder(beamDirection, *csgshape, numSlices, numAnnuli);
}

// --------- actual calculations

/*
 * This method attempts to convert the object to one of the primitive types. If
 * that is not found it uses the arbitrary shape code.
 */
Raster calculate(const V3D &beamDirection, const CSGObject &shape,
                 const double cubeSizeInMetre) {
  if (cubeSizeInMetre <= 0.)
    throw std::runtime_error("Tried to section shape into zero size elements");

  if (shape.shape() == Geometry::detail::ShapeInfo::GeometryShape::CYLINDER) {
    double radius, height;
    V3D center, axis; // these values are ignored
    getCylinderParameters(shape, radius, height, center, axis);
    const size_t numSlice =
        std::max<size_t>(1, static_cast<size_t>(height / cubeSizeInMetre));
    const size_t numAnnuli =
        std::max<size_t>(1, static_cast<size_t>(radius / cubeSizeInMetre));
    return calculateCylinder(beamDirection, shape, numSlice, numAnnuli);
  } else { // arbitrary shape code
    const auto bbox = shape.getBoundingBox();
    assert(bbox.xMax() > bbox.xMin());
    assert(bbox.yMax() > bbox.yMin());
    assert(bbox.zMax() > bbox.zMin());

    const double xLength = bbox.xMax() - bbox.xMin();
    const double yLength = bbox.yMax() - bbox.yMin();
    const double zLength = bbox.zMax() - bbox.zMin();

    const V3D center{ bbox.xMin() + 0.5 * xLength, bbox.yMin() + 0.5 * yLength,
                      bbox.zMin() + 0.5 * zLength };

    const size_t numXSlices = static_cast<size_t>(xLength / cubeSizeInMetre);
    const size_t numYSlices = static_cast<size_t>(yLength / cubeSizeInMetre);
    const size_t numZSlices = static_cast<size_t>(zLength / cubeSizeInMetre);
    const double XSliceThickness = xLength / static_cast<double>(numXSlices);
    const double YSliceThickness = yLength / static_cast<double>(numYSlices);
    const double ZSliceThickness = zLength / static_cast<double>(numZSlices);
    const double elementVolume =
        XSliceThickness * YSliceThickness * ZSliceThickness;

    const size_t numVolumeElements = numXSlices * numYSlices * numZSlices;

    Raster result;
    try {
      result.reserve(numVolumeElements);
    }
    catch (...) {
      // Typically get here if the number of volume elements is too large
      // Provide a bit more information
      throw std::logic_error(
          "Too many volume elements requested - try increasing the value "
          "of the ElementSize property.");
    }

    // go through the bounding box generating cubes and seeing if they are
    // inside the shape
    for (size_t i = 0; i < numZSlices; ++i) {
      const double z =
          (static_cast<double>(i) + 0.5) * ZSliceThickness + bbox.xMin();

      for (size_t j = 0; j < numYSlices; ++j) {
        const double y =
            (static_cast<double>(j) + 0.5) * YSliceThickness + bbox.yMin();

        for (size_t k = 0; k < numXSlices; ++k) {
          const double x =
              (static_cast<double>(k) + 0.5) * XSliceThickness + bbox.zMin();
          // Set the current position in the sample in Cartesian coordinates.
          const Kernel::V3D currentPosition = V3D(x, y, z);
          // Check if the current point is within the object. If not, skip.
          if (shape.isValid(currentPosition)) {
            // Create track for distance in sample before scattering point
            Track incoming(currentPosition, beamDirection * -1.0);
            // We have an issue where occasionally, even though a point is
            // within
            // the object a track segment to the surface isn't correctly
            // created.
            // In the context of this algorithm I think it's safe to just chuck
            // away the element in this case.
            // This will also throw away points that are inside a gauge volume
            // but
            // outside the sample
            if (shape.interceptSurface(incoming) > 0) {
              result.l1.push_back(incoming.cbegin()->distFromStart);
              result.position.push_back(currentPosition);
              result.volume.push_back(elementVolume);
            }
          }
        }
      }
    }

    // Record the number of elements we ended up with
    result.totalvolume = static_cast<double>(result.l1.size()) * elementVolume;

    return result;
  }
}

Raster calculateCylinder(const V3D &beamDirection, const CSGObject &shape,
                         const size_t numSlices, const size_t numAnnuli) {
  if (numSlices == 0)
    throw std::runtime_error("Tried to section cylinder into zero slices");
  if (numAnnuli == 0)
    throw std::runtime_error("Tried to section cylinder into zero annuli");

  // get the geometry for the volume elements
  double radius, height;
  V3D centerBottomBase, z_prime;
  getCylinderParameters(shape, radius, height, centerBottomBase, z_prime);
  V3D center = (z_prime * .5 * height) + centerBottomBase;
  z_prime.normalize();

  const double sliceThickness{height / static_cast<double>(numSlices)};
  const double deltaR{radius / static_cast<double>(numAnnuli)};

  /* The number of volume elements is
   * numslices*(1+2+3+.....+numAnnuli)*6
   * Since the first annulus is separated in 6 segments, the next one in 12 and
   * so on.....
   */
  const size_t numVolumeElements = numSlices * numAnnuli * (numAnnuli + 1) * 3;

  Raster result;
  result.reserve(numVolumeElements);
  result.totalvolume = height * M_PI * radius * radius;

  // Assume that z' = axis. Then select whatever has the smallest dot product
  // with axis to be the x' direction
  V3D x_prime = createPerpendicular(z_prime);
  V3D y_prime = z_prime.cross_prod(x_prime);

  // loop over the elements of the shape and create everything
  // loop over slices
  for (size_t i = 0; i < numSlices; ++i) {
    const double z =
        (static_cast<double>(i) + 0.5) * sliceThickness - 0.5 * height;

    // Number of elements in 1st annulus
    size_t Ni = 0;
    // loop over annuli
    for (size_t j = 0; j < numAnnuli; ++j) {
      Ni += 6;
      const double R =
          (static_cast<double>(j) * radius / static_cast<double>(numAnnuli)) +
          (0.5 * deltaR);

      // all the volume elements in the ring/slice are the same
      const double outerR = R + (deltaR / 2.0);
      const double innerR = outerR - deltaR;
      const double elementVolume = M_PI * (outerR * outerR - innerR * innerR) *
                                   sliceThickness / static_cast<double>(Ni);

      // loop over elements in current annulus
      for (size_t k = 0; k < Ni; ++k) {
        const double phi =
            2. * M_PI * static_cast<double>(k) / static_cast<double>(Ni);
        const double rSinPhi = R * sin(phi);
        const double rCosPhi = R * cos(phi);

        // Calculate the current position in the shape in Cartesian coordinates
        const double xcomp =
            rSinPhi * x_prime[0] + rCosPhi * y_prime[0] + z * z_prime[0];
        const double ycomp =
            rSinPhi * x_prime[1] + rCosPhi * y_prime[1] + z * z_prime[1];
        const double zcomp =
            rSinPhi * x_prime[2] + rCosPhi * y_prime[2] + z * z_prime[2];
        const auto position = center + V3D(xcomp, ycomp, zcomp);

        result.position.push_back(position);
        assert(shape->isValid(position));
        // Create track for distance in cylinder before scattering point
        Track incoming(position, beamDirection * -1.0);

        shape.interceptSurface(incoming);
        result.l1.push_back(incoming.cbegin()->distFromStart);

        result.volume.push_back(elementVolume);
      } // loop over k
    }   // loop over j
  }     // loop over i

  return result;
}
} // namespace Rasterize
} // namespace Geometry
} // namespace Mantid
