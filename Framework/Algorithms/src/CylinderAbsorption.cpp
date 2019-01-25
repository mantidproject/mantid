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
      m_numSlices(0), m_sliceThickness(0), m_numAnnuli(0), m_deltaR(0),
      m_useSampleShape(false) {}

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
void
CylinderAbsorption::getShapeFromSample(const Geometry::IObject &sampleShape) {
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

  m_numSlices = getProperty("NumberOfSlices");
  m_sliceThickness = m_cylHeight / m_numSlices;
  m_numAnnuli = getProperty("NumberOfAnnuli");
  m_deltaR = m_cylRadius / m_numAnnuli;

  /* The number of volume elements is
   * numslices*(1+2+3+.....+numAnnuli)*6
   * Since the first annulus is separated in 6 segments, the next one in 12 and
   * so on.....
   */
  m_numVolumeElements = m_numSlices * m_numAnnuli * (m_numAnnuli + 1) * 3;

  if (m_numVolumeElements == 0) {
    g_log.error() << "Input properties lead to no defined volume elements.\n";
    throw std::runtime_error("No volume elements defined.");
  }

  m_sampleVolume = m_cylHeight * M_PI * m_cylRadius * m_cylRadius;

  if (m_sampleVolume == 0.0) {
    g_log.error() << "Defined sample has zero volume.\n";
    throw std::runtime_error("Sample with zero volume defined.");
  }
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
  m_L1s.resize(m_numVolumeElements);
  m_elementVolumes.resize(m_numVolumeElements);
  m_elementPositions.resize(m_numVolumeElements);

  int counter = 0;
  // loop over slices
  for (int i = 0; i < m_numSlices; ++i) {
    const double z = (i + 0.5) * m_sliceThickness - 0.5 * m_cylHeight;

    // Number of elements in 1st annulus
    int Ni = 0;
    // loop over annuli
    for (int j = 0; j < m_numAnnuli; ++j) {
      Ni += 6;
      const double R = (j * m_cylRadius / m_numAnnuli) + (m_deltaR / 2.0);
      // loop over elements in current annulus
      for (int k = 0; k < Ni; ++k) {
        const double phi = 2 * M_PI * k / Ni;
        // Calculate the current position in the sample in Cartesian
        // coordinates.
        // Remember that our cylinder has its axis along the y axis
        m_elementPositions[counter](R * sin(phi), z, R * cos(phi));
        assert(m_sampleObject->isValid(m_elementPositions[counter]));
        // Create track for distance in cylinder before scattering point
        Track incoming(m_elementPositions[counter], m_beamDirection * -1.0);

        m_sampleObject->interceptSurface(incoming);
        m_L1s[counter] = incoming.cbegin()->distFromStart;

        // Also calculate element volumes here
        const double outerR = R + (m_deltaR / 2.0);
        const double innerR = outerR - m_deltaR;
        const double elementVolume =
            M_PI * (outerR * outerR - innerR * innerR) * m_sliceThickness / Ni;
        m_elementVolumes[counter] = elementVolume;

        counter++;
      }
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
