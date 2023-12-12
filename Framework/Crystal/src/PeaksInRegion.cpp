// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/PeaksInRegion.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PeaksInRegion)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string PeaksInRegion::name() const { return "PeaksInRegion"; }

/// Algorithm's version for identification. @see Algorithm::version
int PeaksInRegion::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PeaksInRegion::category() const { return "Crystal\\Peaks"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PeaksInRegion::init() {
  declareProperty(std::make_unique<PropertyWithValue<bool>>("CheckPeakExtents", false),
                  "Include any peak in the region that has a shape extent "
                  "extending into that region.");

  this->initBaseProperties();

  auto mandatoryExtents = std::make_shared<Mantid::Kernel::MandatoryValidator<std::vector<double>>>();

  std::vector<double> extents(2, 0);
  extents[0] = -50;
  extents[1] = +50;
  declareProperty(std::make_unique<ArrayProperty<double>>("Extents", std::move(extents), std::move(mandatoryExtents)),
                  "A comma separated list of min, max for each dimension,\n"
                  "specifying the extents of each dimension. Optional, default +-50 in "
                  "each dimension.");

  setPropertySettings("PeakRadius", std::make_unique<EnabledWhenProperty>("CheckPeakExtents", IS_NOT_DEFAULT));
}

void PeaksInRegion::validateExtentsInput() const {
  const size_t facesN = this->numberOfFaces();
  std::stringstream outbuff;
  if (m_extents.size() != facesN) {
    throw std::invalid_argument("Six commma separated entries for the extents expected");
  }
  if (m_extents[0] > m_extents[1]) {
    outbuff << "xmin > xmax " << m_extents[0] << " > " << m_extents[1];
    throw std::invalid_argument(outbuff.str());
  }
  if (m_extents[2] > m_extents[3]) {
    outbuff << "ymin > ymax " << m_extents[2] << " > " << m_extents[3];
    throw std::invalid_argument(outbuff.str());
  }
  if (m_extents[4] > m_extents[5]) {
    outbuff << "zmin > zmax " << m_extents[2] << " > " << m_extents[3];
    throw std::invalid_argument(outbuff.str());
  }
}

bool PeaksInRegion::pointOutsideAnyExtents(const V3D &testPoint) const {
  return testPoint[0] < m_extents[0] || testPoint[0] > m_extents[1] || testPoint[1] < m_extents[2] ||
         testPoint[1] > m_extents[3] || testPoint[2] < m_extents[4] || testPoint[2] > m_extents[5];
}

bool PeaksInRegion::pointInsideAllExtents(const V3D &testPoint, const Mantid::Kernel::V3D & /*peakCenter*/) const {
  return testPoint[0] >= m_extents[0] && testPoint[0] <= m_extents[1] && testPoint[1] >= m_extents[2] &&
         testPoint[1] <= m_extents[3] && testPoint[2] >= m_extents[4] && testPoint[2] <= m_extents[5];
}

void PeaksInRegion::checkTouchPoint(const V3D &touchPoint, const V3D &normal, const V3D &faceVertex) const {
  if (normal.scalar_prod(touchPoint - faceVertex) != 0) {
    throw std::runtime_error("Debugging. Calculation is wrong. touch point should always be on the "
                             "plane!"); // Remove this line later. Check that geometry is setup
                                        // properly.
  }
}

/**
Implementation of pure virtual method on PeaksIntersection.
@return Number of faces that the box has (6 - always)
*/
int PeaksInRegion::numberOfFaces() const { return 6; }

/**
Create the faces associated with this shape.
@return newly created faces
*/
VecVecV3D PeaksInRegion::createFaces() const {
  const int minXIndex = 0;
  const int maxXIndex = 1;
  const int minYIndex = 2;
  const int maxYIndex = 3;
  const int minZIndex = 4;
  const int maxZIndex = 5;

  // Clockwise ordering of points around the extents box
  /*

  on front face. Positive z extends into plane.

  p2|---|p3
    |   |
  p1|---|p4
  */
  V3D point1(m_extents[minXIndex], m_extents[minYIndex], m_extents[minZIndex]);
  V3D point2(m_extents[minXIndex], m_extents[maxYIndex], m_extents[minZIndex]);
  V3D point3(m_extents[maxXIndex], m_extents[maxYIndex], m_extents[minZIndex]);
  V3D point4(m_extents[maxXIndex], m_extents[minYIndex], m_extents[minZIndex]);
  V3D point5(m_extents[minXIndex], m_extents[minYIndex], m_extents[maxZIndex]);
  V3D point6(m_extents[minXIndex], m_extents[maxYIndex], m_extents[maxZIndex]);
  V3D point7(m_extents[maxXIndex], m_extents[maxYIndex], m_extents[maxZIndex]);
  V3D point8(m_extents[maxXIndex], m_extents[minYIndex], m_extents[maxZIndex]);

  const int facesN = this->numberOfFaces();
  VecVecV3D faces(facesN);
  int faceIndex = 0;
  faces[faceIndex++] = {point1, point5, point6}; // These define a
                                                 // face normal to x
                                                 // at xmin.

  faces[faceIndex++] = {point4, point7, point8}; // These define a
                                                 // face normal to x
                                                 // at xmax.

  faces[faceIndex++] = {point1, point4, point8}; // These define a
                                                 // face normal to y
                                                 // at ymin.

  faces[faceIndex++] = {point2, point3, point7}; // These define a
                                                 // face normal to y
                                                 // at ymax.

  faces[faceIndex++] = {point1, point2, point3}; // These define a
                                                 // face normal to z
                                                 // at zmin.

  faces[faceIndex++] = {point5, point6, point7}; // These define a
                                                 // face normal to z
                                                 // at zmax.
  return faces;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PeaksInRegion::exec() {
  m_extents = this->getProperty("Extents");
  const bool checkPeakExtents = this->getProperty("CheckPeakExtents");

  validateExtentsInput();

  executePeaksIntersection(checkPeakExtents);
}

} // namespace Mantid::Crystal
