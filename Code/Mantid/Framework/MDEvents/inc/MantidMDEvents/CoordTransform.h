#ifndef MANTID_MDEVENTS_COORDTRANSFORM_H_
#define MANTID_MDEVENTS_COORDTRANSFORM_H_
    
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidMDEvents/AffineMatrixParameter.h"
#include "MantidAPI/SingleValueParameter.h"

namespace Mantid
{
namespace MDEvents
{
  /// Unique SingleValueParameter Declaration for InputNDimensions
  DECLARE_SINGLE_VALUE_PARAMETER(InDimParameter, size_t)
  /// Unique SingleValueParaemter Declaration for OutputNDimensions
  DECLARE_SINGLE_VALUE_PARAMETER(OutDimParameter, size_t)

  /** Generic class to transform from M input dimensions to N output dimensions.
   *
   * The types of conversions to account for are:
   * * Simple rotation matrix
   * * Affine Transformation = linear transform such as a rotation + a translation
   * * Projection into lower dimensions, for example taking a 2D slice out of 3D data.
   * 
   * This class could be subclassed in order to handle non-linear transforms (though
   * making the apply() method virtual would disallow inlining = slowdown).
   *
   * @author Janik Zikovsky
   * @date 2011-04-14 10:03:55.944809
   */
  class DLLExport CoordTransform 
  {
  protected:
    /// Input number of dimensions
    InDimParameter inD;

    /// Output number of dimensions
    OutDimParameter outD;

    /** Affine Matrix to perform the transformation. The matrix has inD+1 columns, outD+1 rows.
     * By using an affine, translations and rotations (or other linear transforms) can be
     * combined by simply multiplying the matrices.
     */
     AffineMatrixParameter affineMatrixParameter;

  public:
    CoordTransform(const size_t inD, const size_t outD);
    virtual ~CoordTransform();
    virtual std::string toXMLString() const;
    void addTranslation(const coord_t * translationVector);
    Mantid::Kernel::Matrix<coord_t> getMatrix() const;
    void setMatrix(const Mantid::Kernel::Matrix<coord_t> newMatrix);

    //----------------------------------------------------------------------------------------------
    /** Apply the coordinate transformation
     *
     * @param inputVector :: fixed-size array of input coordinates, of size inD
     * @param outVector :: fixed-size array of output coordinates, of size outD
     */
    virtual void apply(const coord_t * inputVector, coord_t * outVector)
    {
      // For each output dimension
      for (size_t out = 0; out < outD.getValue(); ++out)
      {
        //Cache the row pointer to make the matrix access a bit faster
        coord_t * rawMatrixRow = affineMatrixParameter.getRawMatrix()[out];
        coord_t outVal = 0.0;
        size_t in;
        for (in = 0; in < inD.getValue(); ++in)
          outVal += rawMatrixRow[in] * inputVector[in];

        // The last input coordinate is "1" always (made homogenous coordinate out of the input x,y,etc.)
        outVal += rawMatrixRow[in];
        // Save in the output
        outVector[out] = outVal;
      }
    }

  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_COORDTRANSFORM_H_ */
