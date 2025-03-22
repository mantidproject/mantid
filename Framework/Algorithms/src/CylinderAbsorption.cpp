// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CylinderAbsorption.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CylinderAbsorption)

using namespace Kernel;
using namespace Geometry;
using namespace API;

CylinderAbsorption::CylinderAbsorption()
    : AbsorptionCorrection(), m_cylHeight(0.0), m_cylRadius(0.0), m_numSlices(0), m_numAnnuli(0),
      m_useSampleShape(false) {}

std::map<std::string, std::string> CylinderAbsorption::validateInputs() {
  std::map<std::string, std::string> issues;
  std::vector<double> prop = getProperty("CylinderAxis");
  if (prop.size() != 3) {
    issues["CylinderAxis"] = "CylinderAxis must be a list with 3 elements.";
  }
  return issues;
}

void CylinderAbsorption::defineProperties() {
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("CylinderSampleHeight", EMPTY_DBL(), mustBePositive,
                  "The height of the cylindrical sample in centimetres");
  declareProperty("CylinderSampleRadius", EMPTY_DBL(), mustBePositive,
                  "The radius of the cylindrical sample in centimetres");
  declareProperty(std::make_unique<ArrayProperty<double>>("CylinderAxis", "0.0, 1.0, 0.0"),
                  "A 3D vector specifying the cylindrical sample's orientation");

  auto positiveInt = std::make_shared<BoundedValidator<int>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfSlices", 1, positiveInt,
                  "The number of slices into which the cylinder is divided for the\n"
                  "calculation");
  declareProperty("NumberOfAnnuli", 1, positiveInt,
                  "The number of annuli into which each slice is divided for the\n"
                  "calculation");
}

// returns an empty string if anything is wrong
void CylinderAbsorption::getShapeFromSample(const Geometry::IObject &sampleShape, bool updateHeight,
                                            bool updateRadius) {
  if (!(updateHeight || updateRadius))
    return; // nothing to update
  if (!sampleShape.hasValidShape())
    return; // no valid shape
  if (sampleShape.shape() != Geometry::detail::ShapeInfo::GeometryShape::CYLINDER)
    return; // not a cylinder

  // get to the underlying ShapeInfo object
  const auto csgshape = dynamic_cast<const CSGObject *>(&sampleShape);
  if (!csgshape)
    return;
  const auto &shapeObj = csgshape->shapeInfo();

  if (updateRadius)
    m_cylRadius = shapeObj.radius();
  if (updateHeight)
    m_cylHeight = shapeObj.height();
}

/// Fetch the properties and set the appropriate member variables
void CylinderAbsorption::retrieveProperties() {
  m_numSlices = getProperty("NumberOfSlices");
  m_numAnnuli = getProperty("NumberOfAnnuli");
  std::vector<double> axisXYZ = getProperty("CylinderAxis");
  m_cylAxis = V3D(axisXYZ[0], axisXYZ[1], axisXYZ[2]);

  bool userSuppliedHeight = false;
  bool userSuppliedRadius = false;

  m_cylHeight = getProperty("CylinderSampleHeight"); // in cm
  if (!isEmpty(m_cylHeight)) {
    m_cylHeight *= 0.01; // now in m
    userSuppliedHeight = true;
  }
  m_cylRadius = getProperty("CylinderSampleRadius"); // in cm
  if (!isEmpty(m_cylRadius)) {
    m_cylRadius *= 0.01; // now in m
    userSuppliedRadius = true;
  }

  // this declares that at least part of the built-in sample geometry should be
  // ignored and use the supplied parameters instead
  m_useSampleShape = !(userSuppliedHeight || userSuppliedRadius);

  // if the user supplied both, then just ignore the built-in shape
  if (userSuppliedHeight && userSuppliedRadius) {
    g_log.information("Chosing user supplied sample geometry in CylinderAbsorption");
    return;
  }

  // get the missing parameters from the sample shape
  const auto &sampleShape = m_inputWS->sample().getShape();
  getShapeFromSample(sampleShape, !userSuppliedHeight, !userSuppliedRadius);

  bool heightOk = m_cylRadius >= 0. && (!isEmpty(m_cylHeight));
  bool radiusOk = (m_cylRadius >= 0.) && (!isEmpty(m_cylRadius));
  if (heightOk && radiusOk) {
    g_log.information() << "Creating cylinder with radius=" << m_cylRadius << "m, height=" << m_cylHeight << "m\n";
  } else if (!heightOk) {
    if (radiusOk) {
      throw std::invalid_argument("Failed to specify height of cylinder");
    } else { // radiusOk == false
      throw std::invalid_argument("Failed to specify height and radius of cylinder");
    }
  } else { // radiusOk == false
    throw std::invalid_argument("Failed to specify radius of cylinder");
  }
}

std::string CylinderAbsorption::sampleXML() {
  if (m_useSampleShape)
    return std::string();

  // Get the sample position, which is typically the origin but we should be
  // generic
  const V3D samplePos = m_inputWS->getInstrument()->getSample()->getPos();
  // Shift so that cylinder is centered at sample position
  const V3D cylBase = m_cylAxis * (-0.5 * m_cylHeight) + samplePos;

  // The default behavior is to have the sample along the y-axis. If something
  // else is desired, it will have to be done through SetSample.
  std::ostringstream xmlShapeStream;
  xmlShapeStream << "<cylinder id=\"detector-shape\"> "
                 << "<centre-of-bottom-base x=\"" << cylBase.X() << "\" y=\"" << cylBase.Y() << "\" z=\"" << cylBase.Z()
                 << "\" /> "
                 << "<axis x=\"" << m_cylAxis.X() << "\" y=\"" << m_cylAxis.Y() << "\" z=\"" << m_cylAxis.Z()
                 << "\" /> "
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

  if (m_sampleObject->shape() != Geometry::detail::ShapeInfo::GeometryShape::CYLINDER)
    throw std::runtime_error("Sample shape is not a cylinder");
  const auto *shape = dynamic_cast<const Geometry::CSGObject *>(m_sampleObject);
  if (!shape)
    throw std::runtime_error("Failed to convert shape from IObject to CSGObject");
  auto raster = Geometry::Rasterize::calculateCylinder(m_beamDirection, *shape, *shape, m_numSlices, m_numAnnuli);
  m_sampleVolume = raster.totalvolume;
  if (raster.l1.size() == 0)
    throw std::runtime_error("Failed to rasterize shape");
  // move over the information
  m_numVolumeElements = raster.l1.size();
  m_L1s = std::move(raster.l1);
  m_elementPositions = std::move(raster.position);
  m_elementVolumes = std::move(raster.volume);
}

} // namespace Mantid::Algorithms
