#include "MantidGeometry/Crystal/SymmetryElement.h"
#include "MantidGeometry/Crystal/SymmetryOperationFactory.h"

#include <gsl/gsl_eigen.h>
#include <gsl/gsl_complex_math.h>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>

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

  gsl_eigen_genv_workspace *eigenWs = gsl_eigen_genv_alloc(matrix.numRows());

  gsl_vector_complex *alpha = gsl_vector_complex_alloc(3);
  gsl_vector *beta = gsl_vector_alloc(3);
  gsl_matrix_complex *eigenVectors = gsl_matrix_complex_alloc(3, 3);

  gsl_eigen_genv(eigenMatrix, identityMatrix, alpha, beta, eigenVectors,
                 eigenWs);

  double determinant = matrix.determinant();

  std::vector<double> eigenVector(3, 0.0);

  for (size_t i = 0; i < matrix.numCols(); ++i) {
    double eigenValue = GSL_REAL(gsl_complex_div_real(
        gsl_vector_complex_get(alpha, i), gsl_vector_get(beta, i)));

    if (fabs(eigenValue - determinant) < 1e-9) {
      for (size_t j = 0; j < matrix.numRows(); ++j) {
        double element = GSL_REAL(gsl_matrix_complex_get(eigenVectors, j, i));

        eigenVector[j] = element;
      }
    }
  }

  gsl_matrix_free(eigenMatrix);
  gsl_matrix_free(identityMatrix);
  gsl_eigen_genv_free(eigenWs);
  gsl_vector_complex_free(alpha);
  gsl_vector_free(beta);
  gsl_matrix_complex_free(eigenVectors);

  double min = 1.0;
  for (size_t i = 0; i < eigenVector.size(); ++i) {
    double absoluteValue = fabs(eigenVector[i]);
    if (absoluteValue != 0.0 &&
        (eigenVector[i] < min && (absoluteValue - fabs(min)) < 1e-9)) {
      min = eigenVector[i];
    }
  }

  V3R axis;
  for (size_t i = 0; i < eigenVector.size(); ++i) {
    axis[i] = static_cast<int>(round(eigenVector[i] / min));
  }

  return axis;
}

SymmetryElementRotation::SymmetryElementRotation()
    : SymmetryElementWithAxis() {}

void SymmetryElementRotation::init(const SymmetryOperation &operation) {
  const Kernel::IntMatrix &matrix = operation.matrix();

  int determinant = matrix.determinant();
  int trace = matrix.Trace();

  if (isNotRotation(determinant, trace)) {
    throw std::invalid_argument(
        "SymmetryOperation " + operation.identifier() +
        " cannot be used to construct SymmetryElementRotation.");
  }

  setAxis(determineAxis(matrix));
  setTranslation(determineTranslation(operation));
  setHMSymbol(determineSymbol(operation));
  setRotationSense(determineRotationSense(operation, getAxis()));
}

SymmetryElementRotation::RotationSense
SymmetryElementRotation::determineRotationSense(
    const SymmetryOperation &operation, const V3R &rotationAxis) const {

  Kernel::V3D pointOnAxis1 = rotationAxis;
  Kernel::V3D pointOnAxis2 = rotationAxis * 2;
  Kernel::V3D pointOffAxis = rotationAxis + Kernel::V3D(2.1, 5.05, -1.1);
  Kernel::V3D generatedPoint = operation * pointOffAxis;

  Kernel::DblMatrix matrix(3, 3, false);
  matrix.setColumn(0, pointOnAxis2 - pointOnAxis1);
  matrix.setColumn(1, pointOffAxis - pointOnAxis1);
  matrix.setColumn(2, generatedPoint - pointOnAxis1);

  double determinant = matrix.determinant() * operation.matrix().determinant();

  if (determinant < 0) {
    return Negative;
  } else {
    return Positive;
  }
}

bool SymmetryElementRotation::isNotRotation(int determinant, int trace) const {
  // It's an inversion or identity
  if (abs(trace) == 3) {
    return true;
  }

  // It's a mirror
  if (trace == 1 && determinant == -1) {
    return true;
  }

  return false;
}

std::string SymmetryElementRotation::determineSymbol(
    const SymmetryOperation &operation) const {

  const Kernel::IntMatrix &matrix = operation.matrix();

  int trace = matrix.Trace();
  int determinant = matrix.determinant();

  if (trace == 0 && determinant == -1) {
    return "-3";
  }

  std::string symbol;

  if (determinant < 0) {
    symbol += "-";
  }

  symbol += boost::lexical_cast<std::string>(operation.order());

  int translation =
      static_cast<int>(static_cast<double>(operation.order()) *
                       Kernel::V3D(determineTranslation(operation)).norm());

  if (translation != 0) {
    symbol += boost::lexical_cast<std::string>(translation);
  }

  return symbol;
}

std::map<V3R, std::string> SymmetryElementMirror::g_glideSymbolMap =
    boost::assign::map_list_of(V3R(0, 0, 0), "m")(V3R(1, 0, 0) / 2,
                                                  "a")(V3R(0, 1, 0) / 2, "b")(
        V3R(0, 0, 1) / 2, "c")(V3R(1, 1, 0) / 2, "n")(V3R(1, 0, 1) / 2, "n")(
        V3R(0, 1, 1) / 2, "n")(V3R(1, 1, 1) / 2, "n")(V3R(1, 1, 0) / 4, "d")(
        V3R(1, 0, 1) / 4, "d")(V3R(0, 1, 1) / 4, "d")(V3R(1, 1, 1) / 4, "d");

SymmetryElementMirror::SymmetryElementMirror() : SymmetryElementWithAxis() {}

void SymmetryElementMirror::init(const SymmetryOperation &operation) {
  const Kernel::IntMatrix &matrix = operation.matrix();

  if (isNotMirror(matrix.determinant(), matrix.Trace())) {
    throw std::invalid_argument(
        "SymmetryOperation " + operation.identifier() +
        " cannot be used to construct SymmetryElementMirror.");
  }

  setAxis(determineAxis(matrix));
  setTranslation(determineTranslation(operation));
  setHMSymbol(determineSymbol(operation));
}

bool SymmetryElementMirror::isNotMirror(int determinant, int trace) const {
  return !(determinant == -1 && trace == 1);
}

std::string SymmetryElementMirror::determineSymbol(
    const SymmetryOperation &operation) const {

  V3R rawTranslation = determineTranslation(operation);

  V3R translation;
  for (size_t i = 0; i < 3; ++i) {
    translation[i] = rawTranslation[i] > RationalNumber(1, 2)
                         ? rawTranslation[i] - 1
                         : rawTranslation[i];
  }

  std::string symbol = g_glideSymbolMap[translation.getPositiveVector()];

  /* Some space groups have "unconventional glides" for which there is no
   * proper symbol, so the general symbol "g" is used for these cases.
   * Examples can be found in No. 227 (Fd-3m).
   */
  if (symbol == "") {
    return "g";
  }

  return symbol;
}

} // namespace Geometry
} // namespace Mantid
