#include <iostream>

#include "MantidAPI/CoordTransform.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidDataObjects/CoordTransformAligned.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::API::CoordTransform;

namespace Mantid {
namespace DataObjects {

//----------------------------------------------------------------------------------------------
/** Constructor.
 * Construct the affine matrix to and initialize to an identity matrix.
 * @param inD :: input number of dimensions, >= 1
 * @param outD :: output number of dimensions, <= inD
 * @throw std::runtime_error if outD > inD
 */
CoordTransformAffine::CoordTransformAffine(const size_t inD, const size_t outD)
    : CoordTransform(inD, outD), m_affineMatrix(outD + 1, inD + 1),
      m_rawMatrix(NULL), m_rawMemory(NULL) {
  m_affineMatrix.identityMatrix();

  // Allocate the raw matrix
  size_t nx = m_affineMatrix.numRows();
  size_t ny = m_affineMatrix.numCols();
  // vector of pointers
  m_rawMatrix = new coord_t *[nx];
  // memory itself
  m_rawMemory = new coord_t[nx * ny];
  for (size_t i = 0; i < nx; i++)
    m_rawMatrix[i] = m_rawMemory + (i * ny);
  // Copy into the raw matrix (for speed)
  copyRawMatrix();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CoordTransformAffine::~CoordTransformAffine() {
  // delete array of pointers to rows
  delete[] m_rawMatrix;
  m_rawMatrix = NULL;

  // delete large mem block holding the matrix
  delete[] m_rawMemory;
  m_rawMemory = NULL;
}

//----------------------------------------------------------------------------------------------
/** Copies the affine matrix into a local raw pointer, for speed.
 * Call this after any change to affineMatrix
 */
void CoordTransformAffine::copyRawMatrix() {
  for (size_t x = 0; x < m_affineMatrix.numRows(); ++x)
    for (size_t y = 0; y < m_affineMatrix.numCols(); ++y)
      m_rawMatrix[x][y] = m_affineMatrix[x][y];
}

//----------------------------------------------------------------------------------------------
/** Virtual cloner
 * @return a copy of this object  */
CoordTransform *CoordTransformAffine::clone() const {
  CoordTransformAffine *out = new CoordTransformAffine(inD, outD);
  out->setMatrix(this->getMatrix());
  return out;
}

//----------------------------------------------------------------------------------------------
/** Directly set the affine matrix to use.
 *
 * @param newMatrix :: (outD+1 * inD+1) matrix to set.
 * @throw runtime_error if the matrix dimensions are incompatible.
 */
void CoordTransformAffine::setMatrix(
    const Mantid::Kernel::Matrix<coord_t> &newMatrix) {
  if (newMatrix.numRows() != outD + 1)
    throw std::runtime_error("setMatrix(): Number of rows must match!");
  if (newMatrix.numCols() != inD + 1)
    throw std::runtime_error("setMatrix(): Number of columns must match!");
  m_affineMatrix = newMatrix;
  // Copy into the raw matrix (for speed)
  copyRawMatrix();
}

//----------------------------------------------------------------------------------------------
/** Return the affine matrix in the transform.
 */
const Mantid::Kernel::Matrix<coord_t> &CoordTransformAffine::getMatrix() const {
  return m_affineMatrix;
}

/** @return the affine matrix */
Mantid::Kernel::Matrix<coord_t> CoordTransformAffine::makeAffineMatrix() const {
  return m_affineMatrix;
}

//----------------------------------------------------------------------------------------------
/** Add a translation (in the output coordinates) to the transform.
 *
 * @param translationVector :: fixed-size array of the translation vector, of
 *size outD
 */
void CoordTransformAffine::addTranslation(const coord_t *translationVector) {
  Matrix<coord_t> translationMatrix(outD + 1, inD + 1);
  // Start with identity
  translationMatrix.identityMatrix();
  // Fill the last column with the translation value
  for (size_t i = 0; i < outD; i++)
    translationMatrix[i][inD] = translationVector[i];

  // Multiply the affine matrix by the translation affine matrix to combine them
  m_affineMatrix *= translationMatrix;

  // Copy into the raw matrix (for speed)
  copyRawMatrix();
}

//----------------------------------------------------------------------------------------------
/** Build a coordinate transformation based on an origin and orthogonal basis
 *vectors.
 * This can reduce the number of dimensions. For example:
 *
 * - The input position is X=(x,y,z)
 * - The origin is X0=(x0,y0,z0)
 * - The basis vectors are U and V (reducing from 3 to 2D)
 * - The output position u = (X-X0).U = X.U - X0.U = x*Ux + y*Uy + z*Uz + (X0.U)
 * - The output position v = (X-X0).V = X.V - X0.V = x*Vx + y*Vy + z*Vz + (X0.V)
 *
 * And this allows us to create the affine matrix:
 *
 * | Ux  Uy  Uz  X0.U | | x |   | u |
 * | Vx  Vy  Vz  X0.V | | y | = | v |
 * | 0   0   0    1   | | z |   | 1 |
 *                      | 1 |
 *
 * @param origin :: origin (in the inDimension), which corresponds to (0,0,...)
 *in outD
 * @param axes :: a list of basis vectors. There must be outD vectors (one for
 *each output dimension)
 *        and each vector must be of length inD (all coordinates in the input
 *dimension).
 *        The vectors must be properly orthogonal: not coplanar or collinear.
 *This is not checked!
 * @param scaling :: a vector of size outD of the scaling to perform in each of
 *the
 *        OUTPUT dimensions.
 * @throw if inconsistent vector sizes are received, or zero-length
 */
void CoordTransformAffine::buildOrthogonal(
    const Mantid::Kernel::VMD &origin,
    const std::vector<Mantid::Kernel::VMD> &axes,
    const Mantid::Kernel::VMD &scaling) {
  if (origin.size() != inD)
    throw std::runtime_error("CoordTransformAffine::buildOrthogonal(): the "
                             "origin must be in the dimensions of the input "
                             "workspace (length inD).");
  if (axes.size() != outD)
    throw std::runtime_error("CoordTransformAffine::buildOrthogonal(): you "
                             "must give as many basis vectors as there are "
                             "dimensions in the output workspace.");
  if (scaling.size() != outD)
    throw std::runtime_error("CoordTransformAffine::buildOrthogonal(): the "
                             "size of the scaling vector must be the same as "
                             "the number of dimensions in the output "
                             "workspace.");

  // Start with identity
  m_affineMatrix.identityMatrix();

  for (size_t i = 0; i < axes.size(); i++) {
    if (axes[i].length() == 0.0)
      throw std::runtime_error("CoordTransformAffine::buildOrthogonal(): one "
                               "of the basis vector was of zero length.");
    if (axes[i].size() != inD)
      throw std::runtime_error("CoordTransformAffine::buildOrthogonal(): one "
                               "of the basis vectors had the wrong number of "
                               "dimensions (must be inD).");
    // Normalize each axis to unity
    VMD basis = axes[i];
    basis.normalize();
    // The row of the affine matrix = the unit vector
    for (size_t j = 0; j < basis.size(); j++)
      m_affineMatrix[i][j] = static_cast<coord_t>(basis[j] * scaling[i]);

    // Now account for the translation
    coord_t transl = 0;
    for (size_t j = 0; j < basis.size(); j++)
      transl += static_cast<coord_t>(
          origin[j] * basis[j]); // dot product of origin * basis aka ( X0 . U )
    // The last column of the matrix = the translation movement
    m_affineMatrix[i][inD] = -transl * static_cast<coord_t>(scaling[i]);
  }

  // Copy into the raw matrix (for speed)
  copyRawMatrix();
}

//----------------------------------------------------------------------------------------------
/** Apply the coordinate transformation
 *
 * @param inputVector :: fixed-size array of input coordinates, of size inD
 * @param outVector :: fixed-size array of output coordinates, of size outD
 */
void CoordTransformAffine::apply(const coord_t *inputVector,
                                 coord_t *outVector) const {
  // For each output dimension
  for (size_t out = 0; out < outD; ++out) {
    // Cache the row pointer to make the matrix access a bit faster
    coord_t *rawMatrixRow = m_rawMatrix[out];
    coord_t outVal = 0.0;
    size_t in;
    for (in = 0; in < inD; ++in)
      outVal += rawMatrixRow[in] * inputVector[in];

    // The last input coordinate is "1" always (made homogenous coordinate out
    // of the input x,y,etc.)
    outVal += rawMatrixRow[in];
    // Save in the output
    outVector[out] = outVal;
  }
}

//----------------------------------------------------------------------------------------------
/** Serialize the coordinate transform
*
* @return The coordinate transform in its serialized form.
*/
std::string CoordTransformAffine::toXMLString() const {
  using namespace Poco::XML;

  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> coordTransformElement =
      pDoc->createElement("CoordTransform");
  pDoc->appendChild(coordTransformElement);

  AutoPtr<Element> coordTransformTypeElement = pDoc->createElement("Type");
  coordTransformTypeElement->appendChild(
     AutoPtr<Node>(pDoc->createTextNode("CoordTransformAffine")));
  coordTransformElement->appendChild(coordTransformTypeElement);

  AutoPtr<Element> paramListElement = pDoc->createElement("ParameterList");

  AutoPtr<Text> formatText = pDoc->createTextNode("%s%s%s");
  paramListElement->appendChild(formatText);

  coordTransformElement->appendChild(paramListElement);

  std::stringstream xmlstream;

  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  // Convert the members to parameters
  AffineMatrixParameter affineMatrixParameter(inD, outD);
  affineMatrixParameter.setMatrix(m_affineMatrix);
  Mantid::API::InDimParameter inD_param(inD);
  Mantid::API::OutDimParameter outD_param(outD);

  std::string formattedXMLString = boost::str(
      boost::format(xmlstream.str().c_str()) % inD_param.toXMLString().c_str() %
      outD_param.toXMLString().c_str() %
      affineMatrixParameter.toXMLString().c_str());
  return formattedXMLString;
}

/**
 * Coordinate transform id
 * @return the type of coordinate transform
 */
std::string CoordTransformAffine::id() const { return "CoordTransformAffine"; }

//----------------------------------------------------------------------------------------------
/** Combine two transformations into a single affine transformations
 *
 * @param first :: CoordTransformAffine or CoordTransformAligned transform.
 * @param second :: CoordTransformAffine or CoordTransformAligned transform.
 * @return pointer to a new CoordTransformAffine combining both
 * @throw std::runtime_error if one of the inputs is not CoordTransformAffine or
 *CoordTransformAligned
 */
CoordTransformAffine *
CoordTransformAffine::combineTransformations(CoordTransform *first,
                                             CoordTransform *second) {
  if (!first || !second)
    throw std::runtime_error(
        "CoordTransformAffine::combineTransformations(): Null input provided.");
  if (second->getInD() != first->getOutD())
    throw std::runtime_error("CoordTransformAffine::combineTransformations(): "
                             "The # of output dimensions of first must be the "
                             "same as the # of input dimensions of second.");
  // Convert both inputs to affine matrices, if needed
  CoordTransformAffine *firstAff = dynamic_cast<CoordTransformAffine *>(first);
  bool ownFirstAff(false);
  if (!firstAff) {
    CoordTransformAligned *firstAl =
        dynamic_cast<CoordTransformAligned *>(first);
    if (!firstAl)
      throw std::runtime_error(
          "CoordTransformAffine::combineTransformations(): first transform "
          "must be either CoordTransformAffine or CoordTransformAligned.");
    firstAff = new CoordTransformAffine(firstAl->getInD(), firstAl->getOutD());
    firstAff->setMatrix(firstAl->makeAffineMatrix());
    ownFirstAff = true;
  }
  CoordTransformAffine *secondAff =
      dynamic_cast<CoordTransformAffine *>(second);
  bool ownSecondAff(false);
  if (!secondAff) {
    CoordTransformAligned *secondAl =
        dynamic_cast<CoordTransformAligned *>(second);
    if (!secondAl)
      throw std::runtime_error(
          "CoordTransformAffine::combineTransformations(): second transform "
          "must be either CoordTransformAffine or CoordTransformAligned.");
    secondAff =
        new CoordTransformAffine(secondAl->getInD(), secondAl->getOutD());
    secondAff->setMatrix(secondAl->makeAffineMatrix());
    ownSecondAff = true;
  }
  // Initialize the affine matrix
  CoordTransformAffine *out =
      new CoordTransformAffine(firstAff->getInD(), secondAff->getOutD());
  // Multiply the two matrices together
  Matrix<coord_t> outMat = secondAff->getMatrix() * firstAff->getMatrix();
  // Set in the output
  out->setMatrix(outMat);
  // Clean up
  if(ownFirstAff) delete firstAff;
  if(ownSecondAff) delete secondAff;
  return out;
}

} // namespace Mantid
} // namespace DataObjects
