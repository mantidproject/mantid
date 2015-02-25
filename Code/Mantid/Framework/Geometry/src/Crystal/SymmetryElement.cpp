#include "MantidGeometry/Crystal/SymmetryElement.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
SymmetryElement::SymmetryElement(const std::string &symbol)
    : m_hmSymbol(symbol) {}

void SymmetryElement::setHMSymbol(const std::string &symbol) {
  m_hmSymbol = symbol;
}

SymmetryElementIdentity::SymmetryElementIdentity() : SymmetryElement("1") {}

SymmetryElement_sptr SymmetryElementIdentity::clone() const {
  return boost::make_shared<SymmetryElementIdentity>();
}

SymmetryElementInversion::SymmetryElementInversion(const V3R &inversionPoint)
    : SymmetryElement("-1"), m_inversionPoint(inversionPoint) {}

SymmetryElement_sptr SymmetryElementInversion::clone() const {
  return boost::make_shared<SymmetryElementInversion>(m_inversionPoint);
}

void SymmetryElementInversion::setInversionPoint(const V3R &inversionPoint) {
  m_inversionPoint = inversionPoint;
}

SymmetryElementWithAxis::SymmetryElementWithAxis(const std::string &symbol,
                                                 const V3R &axis,
                                                 const V3R &translation)
    : SymmetryElement(symbol) {
  setAxis(axis);
  setTranslation(translation);
}

void SymmetryElementWithAxis::setAxis(const V3R &axis) {
  if (axis == V3R(0, 0, 0)) {
    throw std::invalid_argument("Axis cannot be 0.");
  }

  m_axis = axis;
}

SymmetryElementRotation::SymmetryElementRotation(
    const std::string &symbol, const V3R &axis, const V3R &translation,
    const SymmetryElementRotation::RotationSense &rotationSense)
    : SymmetryElementWithAxis(symbol, axis, translation),
      m_rotationSense(rotationSense) {}

SymmetryElement_sptr SymmetryElementRotation::clone() const {
  return boost::make_shared<SymmetryElementRotation>(
      m_hmSymbol, m_axis, m_translation, m_rotationSense);
}

SymmetryElementMirror::SymmetryElementMirror(const std::string &symbol,
                                             const V3R &axis,
                                             const V3R &translation)
    : SymmetryElementWithAxis(symbol, axis, translation) {}

SymmetryElement_sptr SymmetryElementMirror::clone() const {
  return boost::make_shared<SymmetryElementMirror>(m_hmSymbol, m_axis,
                                                   m_translation);
}

} // namespace Geometry
} // namespace Mantid
