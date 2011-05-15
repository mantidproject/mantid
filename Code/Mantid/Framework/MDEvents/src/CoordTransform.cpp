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
  : inD(inD), outD(outD), affineMatrix(outD+1, inD+1), rawMatrix(NULL)
  {
    if (outD > inD)
      throw std::runtime_error("CoordTransform: Cannot have more output dimensions than input dimensions!");
    if (outD == 0)
      throw std::runtime_error("CoordTransform: invalid number of output dimensions!");
    if (inD == 0)
      throw std::runtime_error("CoordTransform: invalid number of input dimensions!");
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
  CoordTransform::~CoordTransform()
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
  void CoordTransform::copyRawMatrix()
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
  void CoordTransform::setMatrix(const Mantid::Geometry::Matrix<coord_t> newMatrix)
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
  Mantid::Geometry::Matrix<coord_t> CoordTransform::getMatrix() const
  {
    return affineMatrix;
  }

  //----------------------------------------------------------------------------------------------
  /** Add a translation (in the output coordinates) to the transform.
   *
   * @param translationVector :: fixed-size array of the translation vector, of size outD
   */
  void CoordTransform::addTranslation(const coord_t * translationVector)
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
  

//  //----------------------------------------------------------------------------------------------
//  /** Set the transformation to be a simple rotation.
//   *
//   * @param translationVector :: fixed-size array of the translation vector, of size inD
//   * @throw runtime_error if inD != outD
//   */
//  TCT
//  void CoordTransform::setRotation(const coord_t * translationVector)
//  {
//    if (inD != outD) throw std::runtime_error("Translation required inD == outD.");
//  }





} // namespace Mantid
} // namespace MDEvents

