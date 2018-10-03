// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_COORDTRANSFORMAFFINE_H_
#define MANTID_DATAOBJECTS_COORDTRANSFORMAFFINE_H_

#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/SingleValueParameter.h"
#include "MantidDataObjects/AffineMatrixParameter.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"

namespace Mantid {
namespace DataObjects {

/** Generic class to transform from M input dimensions to N output dimensions.
 *
 * The types of conversions to account for are:
 * * Simple rotation matrix
 * * Affine Transformation = linear transform such as a rotation + a translation
 * * Projection into lower dimensions, for example taking a 2D slice out of 3D
 *data.
 *
 * This class could be subclassed in order to handle non-linear transforms
 *(though
 * making the apply() method virtual would disallow inlining = slowdown).
 *
 * @author Janik Zikovsky
 * @date 2011-04-14 10:03:55.944809
 */
class DLLExport CoordTransformAffine : public Mantid::API::CoordTransform {
public:
  CoordTransformAffine(const size_t inD, const size_t outD);
  CoordTransform *clone() const override;
  ~CoordTransformAffine() override;
  std::string toXMLString() const override;
  std::string id() const override;
  void addTranslation(const coord_t *translationVector);
  const Mantid::Kernel::Matrix<coord_t> &getMatrix() const;
  Mantid::Kernel::Matrix<coord_t> makeAffineMatrix() const override;
  void setMatrix(const Mantid::Kernel::Matrix<coord_t> &newMatrix);
  void buildOrthogonal(const Mantid::Kernel::VMD &origin,
                       const std::vector<Mantid::Kernel::VMD> &axes,
                       const Mantid::Kernel::VMD &scaling);

  void buildNonOrthogonal(const Mantid::Kernel::VMD &origin,
                          const std::vector<Mantid::Kernel::VMD> &axes,
                          const Mantid::Kernel::VMD &scaling);

  void apply(const coord_t *inputVector, coord_t *outVector) const override;

  static CoordTransformAffine *combineTransformations(CoordTransform *first,
                                                      CoordTransform *second);

protected:
  /** Affine Matrix to perform the transformation. The matrix has inD+1 columns,
   * outD+1 rows.
   * By using an affine, translations and rotations (or other linear transforms)
   * can be
   * combined by simply multiplying the matrices.
   */
  Mantid::Kernel::Matrix<coord_t> m_affineMatrix;

  /// Raw pointer to the same underlying matrix as affineMatrix.
  coord_t **m_rawMatrix;
  /// raw pointer to the memory block, referred by the raw Matrix;
  coord_t *m_rawMemory;

  void copyRawMatrix();
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_COORDTRANSFORMAFFINE_H_ */
