// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Rasterize.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/Track.h"
#include <array>

namespace Mantid::Geometry {

using Geometry::CSGObject;
using Kernel::V3D;

void Raster::reserve(size_t numVolumeElements) {
  l1.reserve(numVolumeElements);
  volume.reserve(numVolumeElements);
  position.reserve(numVolumeElements);
}

namespace { // anonymous

constexpr static V3D X_AXIS{1, 0, 0};
constexpr static V3D Y_AXIS{0, 1, 0};
constexpr static V3D Z_AXIS{0, 0, 1};


// since cylinders are symmetric around the main axis, choose a random
// perpendicular to have as the second axis
V3D createPerpendicular(const V3D &symmetryAxis) {
  const std::array<double, 3> scalars = {{fabs(symmetryAxis.scalar_prod(X_AXIS)),
                                          fabs(symmetryAxis.scalar_prod(Y_AXIS)),
                                          fabs(symmetryAxis.scalar_prod(Z_AXIS))}};
  // check against the cardinal axes
  if (scalars[0] == 0.)
    return symmetryAxis.cross_prod(X_AXIS);
  else if (scalars[1] == 0.)
    return symmetryAxis.cross_prod(Y_AXIS);
  else if (scalars[2] == 0.)
    return symmetryAxis.cross_prod(Z_AXIS);

  // see which one is smallest
  if (scalars[0] < scalars[1] && scalars[0] < scalars[2])
    return symmetryAxis.cross_prod(X_AXIS);
  else if (scalars[1] < scalars[0] && scalars[1] < scalars[2])
    return symmetryAxis.cross_prod(Y_AXIS);
  else
    return symmetryAxis.cross_prod(Z_AXIS);
}

const V3D CalculatePosInCylinder(const double phi, const double R, const double z, const std::array<V3D, 3> coords) {
  const auto &x_prime = coords[0];
  const auto &y_prime = coords[1];
  const auto &z_prime = coords[2];

  double rSinPhi = -R * sin(phi);
  const double rCosPhi = -R * cos(phi);

  // Calculate the current position in the shape in Cartesian coordinates
  const double xcomp = rCosPhi * x_prime[0] + rSinPhi * y_prime[0] + z * z_prime[0];
  const double ycomp = rCosPhi * x_prime[1] + rSinPhi * y_prime[1] + z * z_prime[1];
  const double zcomp = rCosPhi * x_prime[2] + rSinPhi * y_prime[2] + z * z_prime[2];
  return V3D(xcomp, ycomp, zcomp);
}

// This must be updated when new primitives become available
bool hasCustomizedRaster(detail::ShapeInfo::GeometryShape shape) {
  if (shape == detail::ShapeInfo::GeometryShape::CYLINDER)
    return true;
  if (shape == detail::ShapeInfo::GeometryShape::HOLLOWCYLINDER)
    return true;
  // nothing else has been customized
  return false;
}

double calcDistanceInShapeNoCheck(const V3D &beamDirection, const IObject &shape, const V3D &position) {
  // Create track for distance in cylinder before scattering point
  Track incoming(position, -beamDirection);

  if (shape.interceptSurface(incoming) > 0) {
    return incoming.totalDistInsideObject();
  } else {
    return 0;
  }
}

Raster calculateGeneric(const V3D &beamDirection, const IObject &shape, const double cubeSizeInMetre) {
  if (cubeSizeInMetre <= 0.)
    throw std::runtime_error("Tried to section shape into zero size elements");

  const auto bbox = shape.getBoundingBox();
  assert(bbox.xMax() > bbox.xMin());
  assert(bbox.yMax() > bbox.yMin());
  assert(bbox.zMax() > bbox.zMin());

  const double xLength = bbox.xMax() - bbox.xMin();
  const double yLength = bbox.yMax() - bbox.yMin();
  const double zLength = bbox.zMax() - bbox.zMin();

  const auto numXSlices = static_cast<size_t>(xLength / cubeSizeInMetre);
  const auto numYSlices = static_cast<size_t>(yLength / cubeSizeInMetre);
  const auto numZSlices = static_cast<size_t>(zLength / cubeSizeInMetre);
  const double XSliceThickness = xLength / static_cast<double>(numXSlices);
  const double YSliceThickness = yLength / static_cast<double>(numYSlices);
  const double ZSliceThickness = zLength / static_cast<double>(numZSlices);
  const double elementVolume = XSliceThickness * YSliceThickness * ZSliceThickness;

  const size_t numVolumeElements = numXSlices * numYSlices * numZSlices;

  Raster result;
  try {
    result.reserve(numVolumeElements);
  } catch (...) {
    // Typically get here if the number of volume elements is too large
    // Provide a bit more information
    throw std::logic_error("Too many volume elements requested - try increasing the value "
                           "of the ElementSize property.");
  }

  // go through the bounding box generating cubes and seeing if they are
  // inside the shape
  for (size_t i = 0; i < numZSlices; ++i) {
    const double z = (static_cast<double>(i) + 0.5) * ZSliceThickness + bbox.xMin();

    for (size_t j = 0; j < numYSlices; ++j) {
      const double y = (static_cast<double>(j) + 0.5) * YSliceThickness + bbox.yMin();

      for (size_t k = 0; k < numXSlices; ++k) {
        const double x = (static_cast<double>(k) + 0.5) * XSliceThickness + bbox.zMin();
        // Set the current position in the sample in Cartesian coordinates.
        const Kernel::V3D currentPosition = V3D(x, y, z);
        // Check if the current point is within the object. If not, skip.
        if (shape.isValid(currentPosition)) {
          // Create track for distance in sample before scattering point
          Track incoming(currentPosition, -beamDirection);
          // We have an issue where occasionally, even though a point is
          // within the object a track segment to the surface isn't correctly
          // created. In the context of this algorithm I think it's safe to
          // just chuck away the element in this case. This will also throw
          // away points that are inside a gauge volume but outside the sample
          if (shape.interceptSurface(incoming) > 0) {
            result.l1.emplace_back(incoming.totalDistInsideObject());
            result.position.emplace_back(currentPosition);
            result.volume.emplace_back(elementVolume);
          }
        }
      }
    }
  }

  // Record the number of elements we ended up with
  result.totalvolume = static_cast<double>(result.l1.size()) * elementVolume;

  return result;
}

} // namespace

namespace Rasterize {

// -------------------
// collection of calculations that convert to CSGObjects and pass the work on

Raster calculate(const V3D &beamDirection, const IObject &shape, const double cubeSizeInMetre) {
  const auto primitive = shape.shape();
  if (hasCustomizedRaster(primitive)) {
    // convert to the underlying primitive type - this assumes that there are
    // only customizations for CSGObjects
    const auto &shapeInfo = shape.shapeInfo();
    if (primitive == Geometry::detail::ShapeInfo::GeometryShape::CYLINDER) {
      const auto params = shapeInfo.cylinderGeometry();
      const size_t numSlice = std::max<size_t>(1, static_cast<size_t>(params.height / cubeSizeInMetre));
      const size_t numAnnuli = std::max<size_t>(1, static_cast<size_t>(params.radius / cubeSizeInMetre));
      return calculateCylinder(beamDirection, shape, numSlice, numAnnuli);
    } else if (primitive == Geometry::detail::ShapeInfo::GeometryShape::HOLLOWCYLINDER) {
      const auto params = shapeInfo.hollowCylinderGeometry();
      const size_t numSlice = std::max<size_t>(1, static_cast<size_t>(params.height / cubeSizeInMetre));
      const size_t numAnnuli =
          std::max<size_t>(1, static_cast<size_t>((params.radius - params.innerRadius) / cubeSizeInMetre));
      return calculateHollowCylinder(beamDirection, shape, numSlice, numAnnuli);
    } else {
      throw std::runtime_error("Rasterize::calculate should never get to this point");
    }
  } else {
    return calculateGeneric(beamDirection, shape, cubeSizeInMetre);
  }
}

Raster calculateCylinder(const V3D &beamDirection, const IObject &shape, const size_t numSlices,
                         const size_t numAnnuli) {
  if (shape.shape() != detail::ShapeInfo::GeometryShape::CYLINDER)
    throw std::invalid_argument("Given shape is not a cylinder.");

  if (numSlices == 0)
    throw std::runtime_error("Tried to section cylinder into zero slices");
  if (numAnnuli == 0)
    throw std::runtime_error("Tried to section cylinder into zero annuli");

  // convert to the underlying primitive type
  const auto &shapeInfo = shape.shapeInfo();

  // get the geometry for the volume elements
  const auto params = shapeInfo.cylinderGeometry();
  const V3D center = (params.axis * .5 * params.height) + params.centreOfBottomBase;

  const double sliceThickness{params.height / static_cast<double>(numSlices)};
  const double deltaR{params.radius / static_cast<double>(numAnnuli)};

  /* The number of volume elements is
   * numslices*(1+2+3+.....+numAnnuli)*6
   * Since the first annulus is separated in 6 segments, the next one in 12 and
   * so on.....
   */
  const size_t numVolumeElements = numSlices * numAnnuli * (numAnnuli + 1) * 3;

  Raster result;
  result.reserve(numVolumeElements);
  result.totalvolume = params.height * M_PI * params.radius * params.radius;

  // Assume that z' = axis. Then select whatever has the smallest dot product
  // with axis to be the x' direction
  const V3D z_prime = normalize(params.axis);
  const V3D x_prime = createPerpendicular(z_prime);
  const V3D y_prime = z_prime.cross_prod(x_prime);
  const auto coords = std::array<V3D, 3>{{x_prime, y_prime, z_prime}};

  // loop over the elements of the shape and create everything
  // loop over slices
  for (size_t i = 0; i < numSlices; ++i) {
    const double z = (static_cast<double>(i) + 0.5) * sliceThickness - 0.5 * params.height;

    // Number of elements in 1st annulus
    size_t Ni = 0;
    // loop over annuli
    for (size_t j = 0; j < numAnnuli; ++j) {
      Ni += 6;
      const double R = (static_cast<double>(j) * params.radius / static_cast<double>(numAnnuli)) + (0.5 * deltaR);

      // all the volume elements in the ring/slice are the same
      const double outerR = R + (deltaR / 2.0);
      const double innerR = outerR - deltaR;
      const double elementVolume =
          M_PI * (outerR * outerR - innerR * innerR) * sliceThickness / static_cast<double>(Ni);

      // loop over elements in current annulus
      for (size_t k = 0; k < Ni; ++k) {
        const double phi = 2. * M_PI * static_cast<double>(k) / static_cast<double>(Ni);
        const auto position = center + CalculatePosInCylinder(phi, R, z, coords);

        assert(shape.isValid(position));

        result.position.emplace_back(position);
        result.volume.emplace_back(elementVolume);
        // TODO should be customized for cylinder
        result.l1.emplace_back(calcDistanceInShapeNoCheck(beamDirection, shape, position));
      } // loop over k
    } // loop over j
  } // loop over i

  return result;
}

Raster calculateHollowCylinder(const V3D &beamDirection, const IObject &shape, const size_t numSlices,
                               const size_t numAnnuli) {
  if (shape.shape() != detail::ShapeInfo::GeometryShape::HOLLOWCYLINDER)
    throw std::invalid_argument("Given shape is not a hollow cylinder.");

  if (numSlices == 0)
    throw std::runtime_error("Tried to section cylinder into zero slices");
  if (numAnnuli == 0)
    throw std::runtime_error("Tried to section cylinder into zero annuli");

  // convert to the underlying primitive type
  const auto &shapeInfo = shape.shapeInfo();

  // get the geometry for the volume elements
  const auto params = shapeInfo.hollowCylinderGeometry();
  const V3D center = (params.axis * .5 * params.height) + params.centreOfBottomBase;

  const double sliceThickness{params.height / static_cast<double>(numSlices)};
  const double deltaR{(params.radius - params.innerRadius) / static_cast<double>(numAnnuli)};

  /* The number of volume elements is
   * numslices*(1+2+3+.....+numAnnuli)*6
   * Since the first annulus is separated in 6 segments, the next one in 12 and
   * so on.....
   */
  const size_t numVolumeElements = numSlices * numAnnuli * (numAnnuli + 1) * 3;

  Raster result;
  result.reserve(numVolumeElements);
  result.totalvolume = params.height * M_PI * (params.radius * params.radius - params.innerRadius * params.innerRadius);

  // Assume that z' = axis. Then select whatever has the smallest dot product
  // with axis to be the x' direction
  V3D z_prime = params.axis;
  z_prime.normalize();
  const V3D x_prime = createPerpendicular(z_prime);
  const V3D y_prime = z_prime.cross_prod(x_prime);
  const auto coords = std::array<V3D, 3>{{x_prime, y_prime, z_prime}};

  // loop over the elements of the shape and create everything
  // loop over slices
  for (size_t i = 0; i < numSlices; ++i) {
    const double z = (static_cast<double>(i) + 0.5) * sliceThickness - 0.5 * params.height;

    // Number of elements in 1st annulus
    // NOTE:
    // For example, if the hollow cylinder consist of an inner cylinder surface with
    // two annulus, and two annulus for the hollow ring (i.e. total four annulus for
    // the outter cylinder surface). We have
    // Ni = [6,  12,  18, 24]
    //              ^
    //       inner    outter
    const auto nSteps = params.innerRadius / deltaR;
    size_t Ni = static_cast<size_t>(nSteps) * 6;
    // loop over annuli
    for (size_t j = 0; j < numAnnuli; ++j) {
      Ni += 6;
      const double R =
          params.innerRadius +
          (static_cast<double>(j) * (params.radius - params.innerRadius) / static_cast<double>(numAnnuli)) +
          (0.5 * deltaR);

      // all the volume elements in the ring/slice are the same
      const double outerR = R + (deltaR / 2.0);
      const double innerR = outerR - deltaR;
      const double elementVolume =
          M_PI * (outerR * outerR - innerR * innerR) * sliceThickness / static_cast<double>(Ni);

      // loop over elements in current annulus
      for (size_t k = 0; k < Ni; ++k) {
        const double phi = 2. * M_PI * static_cast<double>(k) / static_cast<double>(Ni);

        const auto position = center + CalculatePosInCylinder(phi, R, z, coords);

        assert(shape.isValid(position));

        result.position.emplace_back(position);
        result.volume.emplace_back(elementVolume);
        // TODO should be customized for hollow cylinder
        result.l1.emplace_back(calcDistanceInShapeNoCheck(beamDirection, shape, position));
      } // loop over k
    } // loop over j
  } // loop over i

  return result;
}

} // namespace Rasterize
} // namespace Mantid::Geometry
