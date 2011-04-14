#include "MantidMDEvents/CoordTransform.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"

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
  CoordTransform::CoordTransform(const size_t inD, const size_t outD)
  : inD(inD), outD(outD), affineMatrix(outD+1, inD+1)
  {
    if (outD > inD)
      throw std::runtime_error("CoordTransform: Cannot have more output dimensions than input dimensions!");
    affineMatrix.identityMatrix();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CoordTransform::~CoordTransform()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Apply the coordinate transformation
   *
   * @param inputVector :: fixed-size array of input coordinates, of size inD
   * @param outVector :: fixed-size array of output coordinates, of size outD
   */
  void CoordTransform::apply(const CoordType * inputVector, CoordType * outVector)
  {
    // For each output dimension
    for (size_t out = 0; out < outD; out++)
    {
      //TODO: some tricks to make the matrix access a bit faster
      CoordType outVal = 0.0;
      size_t in;
      for (in = 0; in < inD; in++)
        outVal += affineMatrix[out][in] * inputVector[in];

      // The last input coordinate is "1" always (made homogenous coordinate out of the input x,y,etc.)
      outVal += affineMatrix[out][in];
      // Save in the output
      outVector[out] = outVal;
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Directly set the affine matrix to use.
   *
   * @param newMatrix :: (outD+1 * inD+1) matrix to set.
   * @throw runtime_error if the matrix dimensions are incompatible.
   */
  void CoordTransform::setMatrix(const Mantid::Geometry::Matrix<CoordType> newMatrix)
  {
    if (newMatrix.numRows() != outD+1)
      throw std::runtime_error("setMatrix(): Number of rows must match!");
    if (newMatrix.numCols() != inD+1)
      throw std::runtime_error("setMatrix(): Number of columns must match!");
  }


  //----------------------------------------------------------------------------------------------
  /** Return the affine matrix in the transform.
   */
  Mantid::Geometry::Matrix<CoordType> & CoordTransform::getMatrix()
  {
    return affineMatrix;
  }

  //----------------------------------------------------------------------------------------------
  /** Add a translation (in the output coordinates) to the transform.
   *
   * @param translationVector :: fixed-size array of the translation vector, of size outD
   */
  void CoordTransform::addTranslation(const CoordType * translationVector)
  {
    Matrix<CoordType> translationMatrix(outD+1, inD+1);
    // Start with identity
    translationMatrix.identityMatrix();
    // Fill the last column with the translation value
    for (size_t i=0; i < outD; i++)
      translationMatrix[i][inD] = translationVector[i];

    // Multiply the affine matrix by the translation affine matrix to combine them
    affineMatrix *= translationMatrix;
  }
  

//  //----------------------------------------------------------------------------------------------
//  /** Set the transformation to be a simple rotation.
//   *
//   * @param translationVector :: fixed-size array of the translation vector, of size inD
//   * @throw runtime_error if inD != outD
//   */
//  TCT
//  void CoordTransform::setRotation(const CoordType * translationVector)
//  {
//    if (inD != outD) throw std::runtime_error("Translation required inD == outD.");
//  }





} // namespace Mantid
} // namespace MDEvents

