#include "MantidGeometry/Crystal/SymmetryElementFactory.h"
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_complex_math.h>
#include <stdexcept>
#include <boost/math/special_functions/round.hpp>

namespace Mantid {
namespace Geometry {

/// Generates an instance of SymmetryElementIdentity.
SymmetryElement_sptr SymmetryElementIdentityGenerator::generateElement(
    const SymmetryOperation &operation) const {
  UNUSED_ARG(operation);

  return boost::make_shared<SymmetryElementIdentity>();
}

/// Checks that the SymmetryOperation has no translation and the matrix is of
/// order 1.
bool SymmetryElementIdentityGenerator::canProcess(
    const SymmetryOperation &operation) const {

  return !operation.hasTranslation() && operation.order() == 1;
}

/// Generates an instance of SymmetryElementTranslation with the vector of the
/// operation as translation vector.
SymmetryElement_sptr SymmetryElementTranslationGenerator::generateElement(
    const SymmetryOperation &operation) const {
  return boost::make_shared<SymmetryElementTranslation>(operation.vector());
}

/// Checks that the order of the matrix is 1 and the operation has a
/// translation.
bool SymmetryElementTranslationGenerator::canProcess(
    const SymmetryOperation &operation) const {
  return operation.order() == 1 && operation.hasTranslation();
}

/// Generates an instance of SymmetryElementInversion with the inversion point
/// equal to the vector of the operation divided by two.
SymmetryElement_sptr SymmetryElementInversionGenerator::generateElement(
    const SymmetryOperation &operation) const {

  return boost::make_shared<SymmetryElementInversion>(operation.vector() / 2);
}

/// Checks that the matrix is identity matrix multiplied with -1.
bool SymmetryElementInversionGenerator::canProcess(
    const SymmetryOperation &operation) const {
  Kernel::IntMatrix inversionMatrix(3, 3, true);
  inversionMatrix *= -1;

  return operation.matrix() == inversionMatrix;
}

/**
 * @brief SymmetryElementWithAxisGenerator::determineTranslation
 *
 * According to ITA, 11.2, the translation component of a symmetry operation
 * can be termined with the following algorithm. First, a matrix \f$W\f$ is
 * calculated using the symmetry operation \f$S\f$ and its powers up to its
 * order \f$k\f$, adding the matrices of the resulting operations:
 *
 * \f[
 *  W = W_1(S^0) + W_2(S^1) + \dots + W_k(S^{k-1})
 * \f]
 *
 * The translation vector is then calculation from the vector \f$w\f$ of the
 * operation:
 *
 * \f[
 *  t = \frac{1}{k}\cdot (W \times w)
 * \f]
 *
 * For operations which do not have translation components, this algorithm
 * returns a 0-vector.
 *
 * @param operation :: Symmetry operation, possibly with translation vector.
 * @return Translation vector.
 */
V3R SymmetryElementWithAxisGenerator::determineTranslation(
    const SymmetryOperation &operation) const {
  Kernel::IntMatrix translationMatrix(3, 3, false);

  for (size_t i = 0; i < operation.order(); ++i) {
    translationMatrix += (operation ^ i).matrix();
  }

  return (translationMatrix * operation.vector()) *
         RationalNumber(1, static_cast<int>(operation.order()));
}

/**
 * Returns a GSL-matrix for the given IntMatrix
 *
 * This free function takes an IntMatrix and returns a GSL-matrix with the data.
 * It allocates the memory using gsl_matrix_alloc and the caller of the function
 * is responsible for freeing the memory again.
 *
 * @param matrix :: Kernel::IntMatrix.
 * @return GSL-matrix containing the same data as the input matrix.
 */
gsl_matrix *getGSLMatrix(const Kernel::IntMatrix &matrix) {
  gsl_matrix *gslMatrix = gsl_matrix_alloc(matrix.numRows(), matrix.numCols());

  for (size_t r = 0; r < matrix.numRows(); ++r) {
    for (size_t c = 0; c < matrix.numCols(); ++c) {
      gsl_matrix_set(gslMatrix, r, c, static_cast<double>(matrix[r][c]));
    }
  }

  return gslMatrix;
}

/**
 * Returns a GSL-indentity matrix.
 *
 * This free function returns a GSL-matrix with the provided dimensions.
 * It allocates the memory using gsl_matrix_alloc and the caller of the function
 * is responsible for freeing the memory again.
 *
 * @param rows :: Number of rows in the matrix.
 * @param cols :: Number of columns in the matrix.
 * @return Identity matrix with dimensions (rows, columns).
 */
gsl_matrix *getGSLIdentityMatrix(size_t rows, size_t cols) {
  gsl_matrix *gslMatrix = gsl_matrix_alloc(rows, cols);

  gsl_matrix_set_identity(gslMatrix);

  return gslMatrix;
}

/**
 * Returns the symmetry axis for the given matrix
 *
 * According to ITA, 11.2 the axis of a symmetry operation can be determined by
 * solving the Eigenvalue problem \f$Wu = u\f$ for rotations or \f$Wu = -u\f$
 * for rotoinversions. This is implemented using the general real non-symmetric
 * eigen-problem solver provided by the GSL.
 *
 * @param matrix :: Matrix of a SymmetryOperation
 * @return Axis of symmetry element.
 */
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
    axis[i] = static_cast<int>(boost::math::round(eigenVector[i] / min));
  }

  return axis;
}

/// Generates an instance of SymmetryElementRotation with the corresponding
/// symbol, axis, translation vector and rotation sense.
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

/// Checks the trace and determinat of the matrix to determine if the matrix
/// belongs to a rotation.
bool SymmetryElementRotationGenerator::canProcess(
    const SymmetryOperation &operation) const {
  const Kernel::IntMatrix &matrix = operation.matrix();
  int determinant = matrix.determinant();
  int trace = matrix.Trace();

  return (abs(trace) != 3) && !(trace == 1 && determinant == -1);
}

/// Determines the rotation sense according to the description in ITA 11.2.
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

/// Determines the Hermann-Mauguin symbol of the rotation-, rotoinversion- or
/// screw-axis.
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

/// Generates an instance of SymmetryElementMirror with the corresponding
/// symbol, axis and translation vector.
SymmetryElement_sptr SymmetryElementMirrorGenerator::generateElement(
    const SymmetryOperation &operation) const {
  const Kernel::IntMatrix &matrix = operation.matrix();

  V3R axis = determineAxis(matrix);
  V3R translation = determineTranslation(operation);
  std::string symbol = determineSymbol(operation);

  return boost::make_shared<SymmetryElementMirror>(symbol, axis, translation);
}

/// Checks that the trace of the matrix is 1 and the determinant is -1.
bool SymmetryElementMirrorGenerator::canProcess(
    const SymmetryOperation &operation) const {
  const Kernel::IntMatrix &matrix = operation.matrix();

  return matrix.Trace() == 1 && matrix.determinant() == -1;
}

/// Determines the symbol from the translation vector using a map.
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

/**
 * Creates a SymmetryElement from a SymmetryOperation
 *
 * As detailed in the class description, the method checks whether there is
 * already a prototype SymmetryElement for the provided SymmetryOperation. If
 * not, it tries to find an appropriate generator and uses that to create
 * the prototype. Then it returns a clone of the prototype.
 *
 * @param operation :: SymmetryOperation for which to generate the element.
 * @return SymmetryElement for the supplied operation.
 */
SymmetryElement_sptr SymmetryElementFactoryImpl::createSymElement(
    const SymmetryOperation &operation) {
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

/// Checks whether a generator with that class name is already subscribed.
bool SymmetryElementFactoryImpl::isSubscribed(
    const std::string &generatorClassName) const {
  return (std::find(m_generatorNames.begin(), m_generatorNames.end(),
                    generatorClassName) != m_generatorNames.end());
}

/// Subscribes a generator and stores its class name for later checks.
void SymmetryElementFactoryImpl::subscribe(
    const AbstractSymmetryElementGenerator_sptr &generator,
    const std::string &generatorClassName) {
  m_generators.push_back(generator);
  m_generatorNames.insert(generatorClassName);
}

/// Creates a SymmetryElement from an internally stored prototype.
SymmetryElement_sptr SymmetryElementFactoryImpl::createFromPrototype(
    const std::string &identifier) const {
  auto prototypeIterator = m_prototypes.find(identifier);

  if (prototypeIterator != m_prototypes.end()) {
    return (prototypeIterator->second)->clone();
  }

  return SymmetryElement_sptr();
}

/// Returns a generator that can process the supplied symmetry operation or an
/// invalid pointer if no appropriate generator is found.
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

/// Inserts the provided prototype into the factory.
void SymmetryElementFactoryImpl::insertPrototype(
    const std::string &identifier, const SymmetryElement_sptr &prototype) {
  m_prototypes.insert(std::make_pair(identifier, prototype));
}

DECLARE_SYMMETRY_ELEMENT_GENERATOR(SymmetryElementIdentityGenerator);
DECLARE_SYMMETRY_ELEMENT_GENERATOR(SymmetryElementTranslationGenerator);
DECLARE_SYMMETRY_ELEMENT_GENERATOR(SymmetryElementInversionGenerator);
DECLARE_SYMMETRY_ELEMENT_GENERATOR(SymmetryElementRotationGenerator);
DECLARE_SYMMETRY_ELEMENT_GENERATOR(SymmetryElementMirrorGenerator);

} // namespace Geometry
} // namespace Mantid
