#include "MantidDataObjects/AffineMatrixParameter.h"

namespace Mantid {
namespace DataObjects {

//----------------------------------------------------------------------------------------------
/** Constructor
*
* @param outD the number of output dimensions
* @param inD the nubmer of input dimensions
*/
AffineMatrixParameter::AffineMatrixParameter(size_t outD, size_t inD)
    : m_affineMatrix(outD + 1, inD + 1) {
  m_isValid = false;
  m_affineMatrix.identityMatrix();
  size_t nx = m_affineMatrix.numRows();
  size_t ny = m_affineMatrix.numCols();
  // big chunk of memory holding the whole matrix
  m_rawMem = new coord_t[nx * ny];
  // array of pointers (one per column)
  m_rawMatrix = new coord_t *[nx];
  for (size_t i = 0; i < nx; i++)
    m_rawMatrix[i] = m_rawMem + (i * ny);
  // Copy into the raw matrix (for speed)
  copyRawMatrix();
}

//----------------------------------------------------------------------------------------------
/// Destructor
AffineMatrixParameter::~AffineMatrixParameter() {
  if (m_rawMatrix) {
    delete[] * m_rawMatrix;
    delete[] m_rawMatrix;
  }
  m_rawMatrix = NULL;
  m_rawMem = NULL;
}

//----------------------------------------------------------------------------------------------
/// Copy elements from affinematrix into raw array.
void AffineMatrixParameter::copyRawMatrix() {
  for (size_t x = 0; x < m_affineMatrix.numRows(); ++x)
    for (size_t y = 0; y < m_affineMatrix.numCols(); ++y)
      m_rawMatrix[x][y] = m_affineMatrix[x][y];
}

//----------------------------------------------------------------------------------------------
/** Gets copy of internal affine matrix.
*
* @return A copy of the underlying affine matrix.
*/
AffineMatrixType AffineMatrixParameter::getAffineMatrix() const {
  return m_affineMatrix;
}

//----------------------------------------------------------------------------------------------
/** Get the matrix in its raw array form.
*
* @return the matrix as an array.
*/
coord_t **AffineMatrixParameter::getRawMatrix() { return m_rawMatrix; }

//----------------------------------------------------------------------------------------------
/** Get the name of the parameter
*
* @return Parameter name.
*/
std::string AffineMatrixParameter::getName() const {
  return AffineMatrixParameter::parameterName();
}

//----------------------------------------------------------------------------------------------
/** Serialize the Affine Matrix Parameter
*
* @return the object as serialized. Xml in a std::string.
*/
std::string AffineMatrixParameter::toXMLString() const {
  std::vector<coord_t> elements = this->m_affineMatrix.getVector();
  const size_t size = elements.size();
  std::string parameterValue;

  for (size_t i = 1; i <= size; i++) {
    std::stringstream sstream;
    sstream << elements[i - 1];
    parameterValue.append(sstream.str());
    sstream.clear();
    if (i % m_affineMatrix.numCols() == 0) {
      if (i != size) {
        parameterValue.append(";");
      }
    } else {
      parameterValue.append(",");
    }
  }

  return parameterXMLTemplate(parameterValue);
}

//----------------------------------------------------------------------------------------------
/** Clone the parameter.
*
* @return Cloned parameter.
*/
AffineMatrixParameter *AffineMatrixParameter::clone() const {
  return new AffineMatrixParameter(m_affineMatrix.numRows() - 1,
                                   m_affineMatrix.numCols() - 1);
}

//----------------------------------------------------------------------------------------------
/** Getter for the valid status.
*
* @return The valid status. i.e. has setMatrix been called.
*/
bool AffineMatrixParameter::isValid() const { return m_isValid; }

//----------------------------------------------------------------------------------------------
/** Assignemnt operator.
* @param other : another affine matrix to assign from.
* @return ref to assigned object
*/
AffineMatrixParameter &AffineMatrixParameter::
operator=(const AffineMatrixParameter &other) {
  if ((other.m_affineMatrix.numCols() != this->m_affineMatrix.numCols()) ||
      (other.m_affineMatrix.numRows() != this->m_affineMatrix.numRows())) {
    throw std::runtime_error("Cannot make assignemnts between "
                             "AffineMatrixParameter when the matrixes are of "
                             "different sizes.");
  }
  if (this != &other) {
    this->m_affineMatrix = other.m_affineMatrix;
    this->m_isValid = other.m_isValid;
    copyRawMatrix();
  }
  return *this;
}

//----------------------------------------------------------------------------------------------
/** Copy constructor
*  @param other : another affine matrix to copy from.
*/
AffineMatrixParameter::AffineMatrixParameter(const AffineMatrixParameter &other)
    : m_affineMatrix(other.m_affineMatrix) {
  m_isValid = other.m_isValid;
  size_t nx = m_affineMatrix.numRows();
  size_t ny = m_affineMatrix.numCols();
  m_rawMem = new coord_t[nx * ny];
  m_rawMatrix = new coord_t *[nx];
  for (size_t i = 0; i < nx; i++)
    m_rawMatrix[i] = m_rawMem + (i * ny);
  copyRawMatrix();
}

//----------------------------------------------------------------------------------------------
/** Setter for the internal affine matrix.
*
* @param newMatrix : new matrix to use.
*/
void AffineMatrixParameter::setMatrix(const AffineMatrixType newMatrix) {
  if (newMatrix.numRows() != this->m_affineMatrix.numRows())
    throw std::runtime_error("setMatrix(): Number of rows must match!");
  if (newMatrix.numCols() != this->m_affineMatrix.numCols())
    throw std::runtime_error("setMatrix(): Number of columns must match!");
  m_affineMatrix = newMatrix;
  // Copy into the raw matrix (for speed)
  copyRawMatrix();
  this->m_isValid = true;
}
}
}
