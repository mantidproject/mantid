#include "MantidGeometry/Crystal/SymmetryElement.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

#include <boost/make_shared.hpp>
#include <stdexcept>

namespace Mantid {
namespace Geometry {

/// Constructor with symbol argument.
SymmetryElement::SymmetryElement(const std::string &symbol)
    : m_hmSymbol(symbol) {}

SymmetryElementIdentity::SymmetryElementIdentity() : SymmetryElement("1") {}

/// Returns a clone of the identity element.
SymmetryElement_sptr SymmetryElementIdentity::clone() const {
  return boost::make_shared<SymmetryElementIdentity>();
}

/// Constructor with inversion point, default is (0,0,0).
SymmetryElementInversion::SymmetryElementInversion(const V3R &inversionPoint)
    : SymmetryElement("-1"), m_inversionPoint(inversionPoint) {}

/// Returns a clone of the inversion element.
SymmetryElement_sptr SymmetryElementInversion::clone() const {
  return boost::make_shared<SymmetryElementInversion>(m_inversionPoint);
}

/// Constructor for SymmetryElementWithAxis.
SymmetryElementWithAxis::SymmetryElementWithAxis(const std::string &symbol,
                                                 const V3R &axis,
                                                 const V3R &translation)
    : SymmetryElement(symbol), m_translation(translation) {
  setAxis(axis);
}

/// Sets the axis, throws std::invalid_argument if the axis is (0,0,0).
void SymmetryElementWithAxis::setAxis(const V3R &axis) {
  if (axis == V3R(0, 0, 0)) {
    throw std::invalid_argument("Axis cannot be (0,0,0).");
  }

  m_axis = axis;
}

/// Constructor for rotation-,rotoinversion- and screw-axes.
SymmetryElementRotation::SymmetryElementRotation(
    const std::string &symbol, const V3R &axis, const V3R &translation,
    const SymmetryElementRotation::RotationSense &rotationSense)
    : SymmetryElementWithAxis(symbol, axis, translation),
      m_rotationSense(rotationSense) {}

/// Returns a clone of the symmetry element.
SymmetryElement_sptr SymmetryElementRotation::clone() const {
  return boost::make_shared<SymmetryElementRotation>(
      m_hmSymbol, m_axis, m_translation, m_rotationSense);
}

/// Constructor for mirror planes.
SymmetryElementMirror::SymmetryElementMirror(const std::string &symbol,
                                             const V3R &axis,
                                             const V3R &translation)
    : SymmetryElementWithAxis(symbol, axis, translation) {}

/// Returns a clone of the mirror plane.
SymmetryElement_sptr SymmetryElementMirror::clone() const {
  return boost::make_shared<SymmetryElementMirror>(m_hmSymbol, m_axis,
                                                   m_translation);
}

/// Constructor for translation element, requires translation vector.
SymmetryElementTranslation::SymmetryElementTranslation(const V3R &translation)
    : SymmetryElement("t"), m_translation(translation) {}

/// Returns a clone of the translation.
SymmetryElement_sptr SymmetryElementTranslation::clone() const {
  return boost::make_shared<SymmetryElementTranslation>(m_translation);
}

} // namespace Geometry
} // namespace Mantid
