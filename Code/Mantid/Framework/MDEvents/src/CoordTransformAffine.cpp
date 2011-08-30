#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/CoordTransformAffine.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace MDEvents
{



  //----------------------------------------------------------------------------------------------
  /** Constructor.
   * Construct the affine matrix to and initialize to an identity matrix.
   * @param inD :: input number of dimensions, >= 1
   * @param outD :: output number of dimensions, <= inD
   * @throw std::runtime_error if outD > inD
   */
  CoordTransformAffine::CoordTransformAffine(const size_t inD, const size_t outD)
  : CoordTransform(inD, outD),
    affineMatrix(outD+1, inD+1), rawMatrix(NULL)
  {
    affineMatrix.identityMatrix();

    // Allocate the raw matrix
    size_t nx = affineMatrix.numRows();
    size_t ny = affineMatrix.numCols();
    coord_t * tmpX = new coord_t[nx*ny];
    rawMatrix = new coord_t*[nx];
    for (size_t i=0;i<nx;i++)
      rawMatrix[i] = tmpX + (i*ny);
    // Copy into the raw matrix (for speed)
    copyRawMatrix();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CoordTransformAffine::~CoordTransformAffine()
  {
    if (rawMatrix)
    {
      delete [] *rawMatrix;
      delete [] rawMatrix;
    }
    rawMatrix=NULL;
  }


  //----------------------------------------------------------------------------------------------
  /** Copies the affine matrix into a local raw pointer, for speed.
   * Call this after any change to affineMatrix
   */
  void CoordTransformAffine::copyRawMatrix()
  {
    for (size_t x=0; x < affineMatrix.numRows(); ++x)
      for (size_t y=0; y < affineMatrix.numCols(); ++y)
        rawMatrix[x][y] = affineMatrix[x][y];
  }


  //----------------------------------------------------------------------------------------------
  /** Directly set the affine matrix to use.
   *
   * @param newMatrix :: (outD+1 * inD+1) matrix to set.
   * @throw runtime_error if the matrix dimensions are incompatible.
   */
  void CoordTransformAffine::setMatrix(const Mantid::Kernel::Matrix<coord_t> & newMatrix)
  {
    if (newMatrix.numRows() != outD+1)
      throw std::runtime_error("setMatrix(): Number of rows must match!");
    if (newMatrix.numCols() != inD+1)
      throw std::runtime_error("setMatrix(): Number of columns must match!");
    affineMatrix = newMatrix;
    // Copy into the raw matrix (for speed)
    copyRawMatrix();
  }


  //----------------------------------------------------------------------------------------------
  /** Return the affine matrix in the transform.
   */
  const Mantid::Kernel::Matrix<coord_t> & CoordTransformAffine::getMatrix() const
  {
    return affineMatrix;
  }

  //----------------------------------------------------------------------------------------------
  /** Add a translation (in the output coordinates) to the transform.
   *
   * @param translationVector :: fixed-size array of the translation vector, of size outD
   */
  void CoordTransformAffine::addTranslation(const coord_t * translationVector)
  {
    Matrix<coord_t> translationMatrix(outD+1, inD+1);
    // Start with identity
    translationMatrix.identityMatrix();
    // Fill the last column with the translation value
    for (size_t i=0; i < outD; i++)
      translationMatrix[i][inD] = translationVector[i];

    // Multiply the affine matrix by the translation affine matrix to combine them
    affineMatrix *= translationMatrix;

    // Copy into the raw matrix (for speed)
    copyRawMatrix();
  }


  //----------------------------------------------------------------------------------------------
  /** Build a coordinate transformation based on an origin and orthogonal basis vectors.
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
   * @param origin :: origin (in the inDimension), which corresponds to (0,0,...) in outD
   * @param axes :: a list of basis vectors. There must be outD vectors (one for each output dimension)
   *        and each vector must be of length inD (all coordinates in the input dimension).
   *        The vectors must be properly orthogonal: not coplanar or collinear. This is not checked!
   * @param scaling :: a vector of size outD of the scaling to perform in each of the
   *        OUTPUT dimensions. Optionl, defaults to 1 in all dimensions.
   * @throw if inconsistent vector sizes are received, or zero-length
   */
  void CoordTransformAffine::buildOrthogonal(const std::vector<coord_t> & origin, const std::vector<std::vector<coord_t> > & axes,
      const std::vector<coord_t> & scaling)
  {
    if (origin.size() != inD)
      throw std::runtime_error("CoordTransformAffine::buildOrthogonal(): the origin must be in the dimensions of the input workspace (length inD).");
    if (axes.size() != outD)
      throw std::runtime_error("CoordTransformAffine::buildOrthogonal(): you must give as many basis vectors as there are dimensions in the output workspace.");
    std::vector<coord_t> scale = scaling;
    if (scale.size() == 0) scale.resize(outD, 1.0);
    if (scale.size() != outD)
      throw std::runtime_error("CoordTransformAffine::buildOrthogonal(): the size of the scaling vector must be the same as the number of dimensions in the output workspace.");

    // Start with identity
    affineMatrix.identityMatrix();

    std::vector<std::vector<coord_t> > bases;
    for (size_t i=0; i<axes.size(); i++)
    {
      if (VectorHelper::lengthVector(axes[i]) == 0.0)
        throw std::runtime_error("CoordTransformAffine::buildOrthogonal(): one of the basis vector was of zero length.");
      if (axes[i].size() != inD)
        throw std::runtime_error("CoordTransformAffine::buildOrthogonal(): each basis vector must be of length inD (input # of dimensions).");
      // Normalize each axis to unity
      std::vector<coord_t> basis = VectorHelper::normalizeVector(axes[i]);
      // The row of the affine matrix = the unit vector
      for (size_t j=0; j<basis.size(); j++)
        affineMatrix[i][j] = basis[j] * scale[i];

      // Now account for the translation
      coord_t transl = 0;
      for (size_t j=0; j<basis.size(); j++)
        transl += origin[j] * basis[j]; // dot product of origin * basis aka ( X0 . U )
      // The last column of the matrix = the translation movement
      affineMatrix[i][inD] = -transl * scale[i];
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
  void CoordTransformAffine::apply(const coord_t * inputVector, coord_t * outVector) const
  {
    // For each output dimension
    for (size_t out = 0; out < outD; ++out)
    {
      //Cache the row pointer to make the matrix access a bit faster
      coord_t * rawMatrixRow = rawMatrix[out];
      coord_t outVal = 0.0;
      size_t in;
      for (in = 0; in < inD; ++in)
        outVal += rawMatrixRow[in] * inputVector[in];

      // The last input coordinate is "1" always (made homogenous coordinate out of the input x,y,etc.)
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
  std::string CoordTransformAffine::toXMLString() const
  {
     using namespace Poco::XML;

      AutoPtr<Document> pDoc = new Document;
      AutoPtr<Element> coordTransformElement = pDoc->createElement("CoordTransform");
      pDoc->appendChild(coordTransformElement);

      AutoPtr<Element> coordTransformTypeElement = pDoc->createElement("Type");
      coordTransformTypeElement->appendChild(pDoc->createTextNode("CoordTransformAffine"));
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
      affineMatrixParameter.setMatrix(affineMatrix);
      InDimParameter inD_param(inD);
      OutDimParameter outD_param(outD);

      std::string formattedXMLString = boost::str(boost::format(xmlstream.str().c_str())
        % inD_param.toXMLString().c_str() % outD_param.toXMLString().c_str() % affineMatrixParameter.toXMLString().c_str());
      return formattedXMLString;
  }

//  //----------------------------------------------------------------------------------------------
//  /** Set the transformation to be a simple rotation.
//   *
//   * @param translationVector :: fixed-size array of the translation vector, of size inD
//   * @throw runtime_error if inD != outD
//   */
//  TCT
//  void CoordTransformAffine::setRotation(const coord_t * translationVector)
//  {
//    if (inD != outD) throw std::runtime_error("Translation required inD == outD.");
//  }





} // namespace Mantid
} // namespace MDEvents

