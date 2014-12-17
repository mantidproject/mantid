#include "MantidMDEvents/AffineMatrixParameter.h"

namespace Mantid {
namespace MDEvents {

//----------------------------------------------------------------------------------------------
/** Constructor
*
* @param outD the number of output dimensions
* @param inD the nubmer of input dimensions
*/
AffineMatrixParameter::AffineMatrixParameter(size_t outD, size_t inD)
    : affineMatrix(outD + 1, inD + 1) {
  m_isValid = false;
  affineMatrix.identityMatrix();
  size_t nx = affineMatrix.numRows();
  size_t ny = affineMatrix.numCols();
  coord_t *tmpX = new coord_t[nx * ny];
  rawMatrix = new coord_t *[nx];
  for (size_t i = 0; i < nx; i++)
    rawMatrix[i] = tmpX + (i * ny);
  // Copy into the raw matrix (for speed)
  copyRawMatrix();
}

//----------------------------------------------------------------------------------------------
/// Destructor
AffineMatrixParameter::~AffineMatrixParameter() {
  if (rawMatrix) {
    delete[] * rawMatrix;
    delete[] rawMatrix;
  }
  rawMatrix = NULL;
}

//----------------------------------------------------------------------------------------------
/// Copy elements from affinematrix into raw array.
void AffineMatrixParameter::copyRawMatrix() {
  for (size_t x = 0; x < affineMatrix.numRows(); ++x)
    for (size_t y = 0; y < affineMatrix.numCols(); ++y)
      rawMatrix[x][y] = affineMatrix[x][y];
}

//----------------------------------------------------------------------------------------------
/** Gets copy of internal affine matrix.
*
* @return A copy of the underlying affine matrix.
*/
AffineMatrixType AffineMatrixParameter::getAffineMatrix() const {
  return affineMatrix;
}

//----------------------------------------------------------------------------------------------
/** Get the matrix in its raw array form.
*
* @return the matrix as an array.
*/
coord_t **AffineMatrixParameter::getRawMatrix() { return rawMatrix; }

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
  std::vector<coord_t> elements = this->affineMatrix.getVector();
  const size_t size = elements.size();
  std::string parameterValue;

  for (size_t i = 1; i <= size; i++) {
    std::stringstream sstream;
    sstream << elements[i - 1];
    parameterValue.append(sstream.str());
    sstream.clear();
    if (i % affineMatrix.numCols() == 0) {
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
  return new AffineMatrixParameter(affineMatrix.numRows() - 1,
                                   affineMatrix.numCols() - 1);
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
  if ((other.affineMatrix.numCols() != this->affineMatrix.numCols()) ||
      (other.affineMatrix.numRows() != this->affineMatrix.numRows())) {
    throw std::runtime_error("Cannot make assignemnts between "
                             "AffineMatrixParameter when the matrixes are of "
                             "different sizes.");
  }
  if (this != &other) {
    this->affineMatrix = other.affineMatrix;
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
    : affineMatrix(other.affineMatrix) {
  m_isValid = other.m_isValid;
  size_t nx = affineMatrix.numRows();
  size_t ny = affineMatrix.numCols();
  coord_t *tmpX = new coord_t[nx * ny];
  rawMatrix = new coord_t *[nx];
  for (size_t i = 0; i < nx; i++)
    rawMatrix[i] = tmpX + (i * ny);
  copyRawMatrix();
}

//----------------------------------------------------------------------------------------------
/** Setter for the internal affine matrix.
*
* @param newMatrix : new matrix to use.
*/
void AffineMatrixParameter::setMatrix(const AffineMatrixType newMatrix) {
  if (newMatrix.numRows() != this->affineMatrix.numRows())
    throw std::runtime_error("setMatrix(): Number of rows must match!");
  if (newMatrix.numCols() != this->affineMatrix.numCols())
    throw std::runtime_error("setMatrix(): Number of columns must match!");
  affineMatrix = newMatrix;
  // Copy into the raw matrix (for speed)
  copyRawMatrix();
  this->m_isValid = true;
}
}
}
