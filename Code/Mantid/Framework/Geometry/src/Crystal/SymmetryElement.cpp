#include "MantidGeometry/Crystal/SymmetryElement.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

namespace Mantid {
namespace Geometry {
SymmetryElement::SymmetryElement() : m_hmSymbol() {}

void SymmetryElement::setHMSymbol(const std::string &symbol) {
  m_hmSymbol = symbol;
}

SymmetryElementIdentity::SymmetryElementIdentity() : SymmetryElement() {}

void SymmetryElementIdentity::init(const SymmetryOperation &operation) {

  if (operation.order() != 1) {
    throw std::invalid_argument(
        "SymmetryOperation " + operation.identifier() +
        " cannot be used to construct SymmetryElement 1.");
  }

  setHMSymbol("1");
}

SymmetryElementInversion::SymmetryElementInversion()
    : SymmetryElement(), m_inversionPoint() {}

void SymmetryElementInversion::init(const SymmetryOperation &operation) {
  SymmetryOperation op =
      SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z");

  if (operation.matrix() != op.matrix()) {
    throw std::invalid_argument(
        "SymmetryOperation " + operation.identifier() +
        " cannot be used to initialize SymmetryElement -1.");
  }

  setHMSymbol("-1");
  setInversionPoint(operation.vector() / 2);
}

void SymmetryElementInversion::setInversionPoint(const V3R &inversionPoint) {
  m_inversionPoint = inversionPoint;
}

SymmetryElementWithAxis::SymmetryElementWithAxis() : SymmetryElement() {}

void SymmetryElementWithAxis::setAxis(const V3R &axis) {
  if (axis == V3R(0, 0, 0)) {
    throw std::invalid_argument("Axis cannot be 0.");
  }

  m_axis = axis;
}

V3R SymmetryElementWithAxis::determineTranslation(
    const SymmetryOperation &operation) const {

  Kernel::IntMatrix translationMatrix(3, 3, false);

  for (size_t i = 0; i < operation.order(); ++i) {
    translationMatrix += (operation ^ i).matrix();
  }

  return (translationMatrix * operation.vector()) *
         RationalNumber(1, static_cast<int>(operation.order()));
}

V3R
SymmetryElementWithAxis::determineAxis(const Kernel::IntMatrix &matrix) const {
  UNUSED_ARG(matrix);
  // Solve Eigenvalue problem Wu = sign(det) * u

  return V3R(0, 0, 0);
}

V3R SymmetryElementWithAxis::determineFixPoint(const Kernel::IntMatrix &matrix,
                                               const V3R &vector) const {
  UNUSED_ARG(matrix);
  UNUSED_ARG(vector);

  return V3R(0, 0, 0);
}

SymmetryElementRotation::SymmetryElementRotation()
    : SymmetryElementWithAxis() {}

void SymmetryElementRotation::init(const SymmetryOperation &operation) {
  UNUSED_ARG(operation);
}

SymmetryElementMirror::SymmetryElementMirror() : SymmetryElementWithAxis() {}

void SymmetryElementMirror::init(const SymmetryOperation &operation) {
  UNUSED_ARG(operation);
}

} // namespace Geometry
} // namespace Mantid
