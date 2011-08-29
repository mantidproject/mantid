#ifndef MANTID_MDEVENTS_COORDTRANSFORMAFFINE_H_
#define MANTID_MDEVENTS_COORDTRANSFORMAFFINE_H_

#include "MantidAPI/SingleValueParameter.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/AffineMatrixParameter.h"
#include "MantidMDEvents/CoordTransform.h"

namespace Mantid
{
namespace MDEvents
{

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
  class DLLExport CoordTransformAffine : public CoordTransform
  {
  public:
    CoordTransformAffine(const size_t inD, const size_t outD);
    virtual ~CoordTransformAffine();
    virtual std::string toXMLString() const;
    void addTranslation(const coord_t * translationVector);
    const Mantid::Kernel::Matrix<coord_t> & getMatrix() const;
    void setMatrix(const Mantid::Kernel::Matrix<coord_t> & newMatrix);

    virtual void apply(const coord_t * inputVector, coord_t * outVector) const;

  protected:
    /** Affine Matrix to perform the transformation. The matrix has inD+1 columns, outD+1 rows.
     * By using an affine, translations and rotations (or other linear transforms) can be
     * combined by simply multiplying the matrices.
     */
    Mantid::Kernel::Matrix<coord_t> affineMatrix;

    /// Raw pointer to the same underlying matrix as affineMatrix.
    coord_t ** rawMatrix;

    void copyRawMatrix();
  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_COORDTRANSFORMAFFINE_H_ */
