#include "MantidGeometry/Crystal/SymmetryElementFactory.h"
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_complex_math.h>

namespace Mantid {
namespace Geometry {

SymmetryElement_sptr SymmetryElementIdentityGenerator::generateElement(
    const SymmetryOperation &operation) const {
  UNUSED_ARG(operation);

  return boost::make_shared<SymmetryElementIdentity>();
}

bool SymmetryElementIdentityGenerator::canProcess(
    const SymmetryOperation &operation) const {

  return !operation.hasTranslation() && operation.order() == 1;
}

SymmetryElement_sptr SymmetryElementInversionGenerator::generateElement(
    const SymmetryOperation &operation) const {

  return boost::make_shared<SymmetryElementInversion>(operation.vector() / 2);
}

bool SymmetryElementInversionGenerator::canProcess(
    const SymmetryOperation &operation) const {
  Kernel::IntMatrix inversionMatrix(3, 3, true);
  inversionMatrix *= -1.0;

  return operation.matrix() == inversionMatrix;
}

V3R SymmetryElementWithAxisGenerator::determineTranslation(
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

V3R SymmetryElementWithAxisGenerator::determineAxis(
    const Kernel::IntMatrix &matrix) const {
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

SymmetryElement_sptr SymmetryElementRotationGenerator::generateElement(
    const SymmetryOperation &operation) const {
  const Kernel::IntMatrix &matrix = operation.matrix();

  V3R axis = determineAxis(matrix);
  V3R translation = determineTranslation(operation);
  SymmetryElementRotation::RotationSense rotationSense =
      determineRotationSense(operation, axis);
  std::string symbol = determineSymbol(operation);

  return boost::make_shared<SymmetryElementRotation>(symbol, axis, translation,
                                                     rotationSense);
}

bool SymmetryElementRotationGenerator::canProcess(
    const SymmetryOperation &operation) const {
  const Kernel::IntMatrix &matrix = operation.matrix();
  int determinant = matrix.determinant();
  int trace = matrix.Trace();

  return (abs(trace) != 3) && !(trace == 1 && determinant == -1);
}

SymmetryElementRotation::RotationSense
SymmetryElementRotationGenerator::determineRotationSense(
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
    return SymmetryElementRotation::Negative;
  } else {
    return SymmetryElementRotation::Positive;
  }
}

std::string SymmetryElementRotationGenerator::determineSymbol(
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

std::map<V3R, std::string> SymmetryElementMirrorGenerator::g_glideSymbolMap =
    boost::assign::map_list_of(V3R(0, 0, 0), "m")(V3R(1, 0, 0) / 2,
                                                  "a")(V3R(0, 1, 0) / 2, "b")(
        V3R(0, 0, 1) / 2, "c")(V3R(1, 1, 0) / 2, "n")(V3R(1, 0, 1) / 2, "n")(
        V3R(0, 1, 1) / 2, "n")(V3R(1, 1, 1) / 2, "n")(V3R(1, 1, 0) / 4, "d")(
        V3R(1, 0, 1) / 4, "d")(V3R(0, 1, 1) / 4, "d")(V3R(1, 1, 1) / 4, "d");

SymmetryElement_sptr SymmetryElementMirrorGenerator::generateElement(
    const SymmetryOperation &operation) const {
  const Kernel::IntMatrix &matrix = operation.matrix();

  V3R axis = determineAxis(matrix);
  V3R translation = determineTranslation(operation);
  std::string symbol = determineSymbol(operation);

  return boost::make_shared<SymmetryElementMirror>(symbol, axis, translation);
}

bool SymmetryElementMirrorGenerator::canProcess(
    const SymmetryOperation &operation) const {
  const Kernel::IntMatrix &matrix = operation.matrix();

  return matrix.Trace() == 1 && matrix.determinant() == -1;
}

std::string SymmetryElementMirrorGenerator::determineSymbol(
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

SymmetryElement_sptr
SymmetryElementFactoryImpl::createSymElem(const SymmetryOperation &operation) {
  std::string operationIdentifier = operation.identifier();

  SymmetryElement_sptr element = createFromPrototype(operationIdentifier);

  if (element) {
    return element;
  }

  AbstractSymmetryElementGenerator_sptr generator = getGenerator(operation);

  if (!generator) {
    throw std::runtime_error("Could not process symmetry operation '" +
                             operationIdentifier + "'.");
  }

  insertPrototype(operationIdentifier, generator->generateElement(operation));

  return createFromPrototype(operationIdentifier);
}

bool SymmetryElementFactoryImpl::isSubscribed(
    const std::string &generatorClassName) const {
  return (std::find(m_generatorNames.begin(), m_generatorNames.end(),
                    generatorClassName) != m_generatorNames.end());
}

void SymmetryElementFactoryImpl::subscribe(
    const AbstractSymmetryElementGenerator_sptr &generator,
    const std::string &generatorClassName) {
  m_generators.push_back(generator);
  m_generatorNames.insert(generatorClassName);
}

SymmetryElement_sptr SymmetryElementFactoryImpl::createFromPrototype(
    const std::string &identifier) const {
  auto prototypeIterator = m_prototypes.find(identifier);

  if (prototypeIterator != m_prototypes.end()) {
    return (prototypeIterator->second)->clone();
  }

  return SymmetryElement_sptr();
}

AbstractSymmetryElementGenerator_sptr SymmetryElementFactoryImpl::getGenerator(
    const SymmetryOperation &operation) const {
  for (auto generator = m_generators.begin(); generator != m_generators.end();
       ++generator) {
    if ((*generator)->canProcess(operation)) {
      return *generator;
    }
  }

  return AbstractSymmetryElementGenerator_sptr();
}

void SymmetryElementFactoryImpl::insertPrototype(
    const std::string &identifier, const SymmetryElement_sptr &prototype) {
  m_prototypes.insert(std::make_pair(identifier, prototype));
}

DECLARE_SYMMETRY_ELEMENT_GENERATOR(SymmetryElementIdentityGenerator);
DECLARE_SYMMETRY_ELEMENT_GENERATOR(SymmetryElementInversionGenerator);
DECLARE_SYMMETRY_ELEMENT_GENERATOR(SymmetryElementRotationGenerator);
DECLARE_SYMMETRY_ELEMENT_GENERATOR(SymmetryElementMirrorGenerator);

} // namespace Geometry
} // namespace Mantid
