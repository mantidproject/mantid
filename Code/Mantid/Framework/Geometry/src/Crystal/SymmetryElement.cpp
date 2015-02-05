#include "MantidGeometry/Crystal/SymmetryElement.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

#include <gsl/gsl_eigen.h>

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

gsl_matrix *getGSLMatrix(const Kernel::IntMatrix &matrix) {
  gsl_matrix *gslMatrix = gsl_matrix_alloc(matrix.numRows(), matrix.numCols());

  for (size_t r = 0; r < matrix.numRows(); ++r) {
    for (size_t c = 0; c < matrix.numCols(); ++c) {
      gsl_matrix_set(gslMatrix, r, c, static_cast<double>(matrix[r][c]));
    }
  }

  return gslMatrix;
}

gsl_matrix *getGSLIdentityMatrix(size_t rows, size_t cols) {
  gsl_matrix *gslMatrix = gsl_matrix_alloc(rows, cols);

  gsl_matrix_set_identity(gslMatrix);

  return gslMatrix;
}

V3R
SymmetryElementWithAxis::determineAxis(const Kernel::IntMatrix &matrix) const {
  gsl_matrix *eigenMatrix = getGSLMatrix(matrix);
  gsl_matrix *identityMatrix =
      getGSLIdentityMatrix(matrix.numRows(), matrix.numCols());

  gsl_eigen_gen_workspace *eigenWs = gsl_eigen_gen_alloc(matrix.numRows());

  gsl_matrix_free(eigenMatrix);
  gsl_matrix_free(identityMatrix);
  gsl_eigen_gen_free(eigenWs);

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
