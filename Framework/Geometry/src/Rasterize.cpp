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

namespace Mantid {
namespace Geometry {

using Geometry::CSGObject;

void Raster::resize(size_t numVolumeElements) {
  l1.resize(numVolumeElements);
  volume.resize(numVolumeElements);
  position.resize(numVolumeElements);
}

namespace { // anonymous

void getCylinderParameters(const CSGObject &shape, double &radius,
                           double &height) {
  // fundamental checks for any object
  if (!shape.hasValidShape())
    throw std::logic_error("Shape[CSGObject] does not have a valid shape");
  if (shape.shape() != Geometry::detail::ShapeInfo::GeometryShape::CYLINDER)
    throw std::logic_error("Shape[CSGObject] is not a cylinder");

  const auto &shapeInfo = shape.shapeInfo();
  radius = shapeInfo.radius();
  height = shapeInfo.height();
}

} // anonymous

namespace Rasterize {

Raster calculateCylinder(const Kernel::V3D &beamDirection,
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

Raster calculateCylinder(const Kernel::V3D &beamDirection,
                         const CSGObject &shape, const size_t numSlices,
                         const size_t numAnnuli) {
  if (numSlices == 0)
    throw std::runtime_error("Tried to section cylinder into zero slices");
  if (numAnnuli == 0)
    throw std::runtime_error("Tried to section cylinder into zero annuli");

  // get the geometry for the volume elements
  double radius, height;
  getCylinderParameters(shape, radius, height);

  const double sliceThickness{ height / static_cast<double>(numSlices) };
  const double deltaR{ radius / static_cast<double>(numAnnuli) };

  /* The number of volume elements is
   * numslices*(1+2+3+.....+numAnnuli)*6
   * Since the first annulus is separated in 6 segments, the next one in 12 and
   * so on.....
   */
  const size_t numVolumeElements = numSlices * numAnnuli * (numAnnuli + 1) * 3;

  Raster result;
  result.resize(numVolumeElements);
  result.totalvolume = height * M_PI * radius * radius;

  // loop over the elements of the shape and create everything
  size_t counter = 0;
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
      // loop over elements in current annulus
      for (size_t k = 0; k < Ni; ++k) {
        const double phi =
            2. * M_PI * static_cast<double>(k) / static_cast<double>(Ni);
        // Calculate the current position in the sample in Cartesian
        // coordinates.
        // Remember that our cylinder has its axis along the y axis
        result.position[counter](R * sin(phi), z, R * cos(phi));
        assert(shape->isValid(m_elementPositions[counter]));
        // Create track for distance in cylinder before scattering point
        Track incoming(result.position[counter], beamDirection * -1.0);

        shape.interceptSurface(incoming);
        result.l1[counter] = incoming.cbegin()->distFromStart;

        // Also calculate element volumes here
        const double outerR = R + (deltaR / 2.0);
        const double innerR = outerR - deltaR;
        const double elementVolume = M_PI *
                                     (outerR * outerR - innerR * innerR) *
                                     sliceThickness / static_cast<double>(Ni);
        result.volume[counter] = elementVolume;

        counter++;
      } // loop over k
    }   // loop over j
  }     // loop over i

  return result;
}
}
} // namespace Geometry
} // namespace Mantid
