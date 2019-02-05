// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CylinderAbsorption.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CylinderAbsorption)

using namespace Kernel;
using namespace Geometry;
using namespace API;

CylinderAbsorption::CylinderAbsorption()
    : AbsorptionCorrection(), m_cylHeight(0.0), m_cylRadius(0.0),
      m_numSlices(0), m_numAnnuli(0), m_useSampleShape(false) {}

void CylinderAbsorption::defineProperties() {
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("CylinderSampleHeight", EMPTY_DBL(), mustBePositive,
                  "The height of the cylindrical sample in centimetres");
  declareProperty("CylinderSampleRadius", EMPTY_DBL(), mustBePositive,
                  "The radius of the cylindrical sample in centimetres");

  auto positiveInt = boost::make_shared<BoundedValidator<int>>();
  positiveInt->setLower(1);
  declareProperty(
      "NumberOfSlices", 1, positiveInt,
      "The number of slices into which the cylinder is divided for the\n"
      "calculation");
  declareProperty(
      "NumberOfAnnuli", 1, positiveInt,
      "The number of annuli into which each slice is divided for the\n"
      "calculation");
}

// returns an empty string if anything is wrong
void CylinderAbsorption::getShapeFromSample(
    const Geometry::IObject &sampleShape) {
  if ((!isEmpty(m_cylHeight)) && (!isEmpty(m_cylRadius)))
    return; // nothing to update
  if (!sampleShape.hasValidShape())
    return; // no valid shape
  if (sampleShape.shape() !=
      Geometry::detail::ShapeInfo::GeometryShape::CYLINDER)
    return; // not a cylinder

  // get to the underlying ShapeInfo object
  const auto csgshape = dynamic_cast<const CSGObject *>(&sampleShape);
  if (!csgshape)
    return;
  const auto &shapeObj = csgshape->shapeInfo();

  if (isEmpty(m_cylRadius))
    m_cylRadius = shapeObj.radius();
  if (isEmpty(m_cylHeight))
    m_cylHeight = shapeObj.height();
}

/// Fetch the properties and set the appropriate member variables
void CylinderAbsorption::retrieveProperties() {
  m_cylHeight = getProperty("CylinderSampleHeight"); // in cm
  if (!isEmpty(m_cylHeight))
    m_cylHeight *= 0.01;                             // now in m
  m_cylRadius = getProperty("CylinderSampleRadius"); // in cm
  if (!isEmpty(m_cylRadius))
    m_cylRadius *= 0.01; // now in m

  // this declares that at least part of the built-in sample geometry should be
  // ignored and use the supplied parameters instead
  m_useSampleShape = (isEmpty(m_cylHeight) && isEmpty(m_cylRadius));

  m_numSlices = getProperty("NumberOfSlices");
  m_numAnnuli = getProperty("NumberOfAnnuli");

  // get the missing parameters from the sample shape
  const auto &sampleShape = m_inputWS->sample().getShape();
  getShapeFromSample(sampleShape);

  g_log.information() << "Creating cylinder with radius=" << m_cylRadius
                      << "m, height=" << m_cylHeight << "m\n";
  if (isEmpty(m_cylHeight) || isEmpty(m_cylRadius)) {
    throw std::invalid_argument(
        "Need to specify both height and radius of cylinder");
  }
  if (m_cylHeight <= 0. || m_cylRadius <= 0.)
    throw std::invalid_argument(
        "Failed to specify height and radius of cylinder");
}

std::string CylinderAbsorption::sampleXML() {
  if (m_useSampleShape)
    return std::string();

  // Get the sample position, which is typically the origin but we should be
  // generic
  const V3D samplePos = m_inputWS->getInstrument()->getSample()->getPos();
  // Shift so that cylinder is centered at sample position
  const double cylinderBase = (-0.5 * m_cylHeight) + samplePos.Y();

  std::ostringstream xmlShapeStream;
  xmlShapeStream << "<cylinder id=\"detector-shape\"> "
                 << "<centre-of-bottom-base x=\"" << samplePos.X() << "\" y=\""
                 << cylinderBase << "\" z=\"" << samplePos.Z() << "\" /> "
                 << "(<axis x=\"0\" y=\"1\" z=\"0\" /> )"
                 << "<radius val=\"" << m_cylRadius << "\" /> "
                 << "<height val=\"" << m_cylHeight << "\" /> "
                 << "</cylinder>";

  return xmlShapeStream.str();
}

/// Calculate the distances for L1 and element size for each element in the
/// sample
void CylinderAbsorption::initialiseCachedDistances() {
  if (!m_sampleObject) // should never happen
    throw std::runtime_error("Do not have a sample object defined");

  if (m_sampleObject->shape() !=
      Geometry::detail::ShapeInfo::GeometryShape::CYLINDER)
    throw std::runtime_error("Sample shape is not a cylinder");
  const Geometry::CSGObject *shape =
      dynamic_cast<const Geometry::CSGObject *>(m_sampleObject);
  if (!shape)
    throw std::runtime_error(
        "Failed to convert shape from IObject to CSGObject");
  const auto raster = Geometry::Rasterize::calculateCylinder(
      m_beamDirection, *shape, m_numSlices, m_numAnnuli);
  m_sampleVolume = raster.totalvolume;
  if (raster.l1.size() == 0)
    throw std::runtime_error("Failed to rasterize shape");
  m_numVolumeElements = raster.l1.size();
  m_L1s.assign(raster.l1.begin(), raster.l1.end());
  m_elementPositions.assign(raster.position.begin(), raster.position.end());
  m_elementVolumes.assign(raster.volume.begin(), raster.volume.end());
}

} // namespace Algorithms
} // namespace Mantid
