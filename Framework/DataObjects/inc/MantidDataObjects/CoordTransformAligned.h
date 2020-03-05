// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_COORDTRANSFORMALIGNED_H_
#define MANTID_DATAOBJECTS_COORDTRANSFORMALIGNED_H_

#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/VectorParameter.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {

/// Unique type declaration for which dimensions are used in the input workspace
DECLARE_VECTOR_PARAMETER(DimensionsToBinFromParam, size_t)

/// Unique type declaration for the offset of coordinates
DECLARE_VECTOR_PARAMETER(OriginOffsetParam, coord_t)

/// Unique type declaration for the step size in transformation
DECLARE_VECTOR_PARAMETER(ScalingParam, coord_t)

/** A restricted version of CoordTransform which transforms
  from one set of dimensions to another, allowing:

   - An offset
   - A reduction in the number of dimensions
   - A scaling

  While a normal Affine matrix would handle this case, this special
  case is used in order to reduce the number of calculation.

  @author Janik Zikovsky
  @date 2011-08-29
*/
class DLLExport CoordTransformAligned : public Mantid::API::CoordTransform {
public:
  CoordTransformAligned(const size_t inD, const size_t outD,
                        const size_t *dimensionToBinFrom, const coord_t *origin,
                        const coord_t *scaling);
  CoordTransformAligned(const size_t inD, const size_t outD,
                        std::vector<size_t> dimensionToBinFrom,
                        std::vector<coord_t> origin,
                        std::vector<coord_t> scaling);
  CoordTransform *clone() const override;

  std::string toXMLString() const override;
  std::string id() const override;
  void apply(const coord_t *inputVector, coord_t *outVector) const override;
  Mantid::Kernel::Matrix<coord_t> makeAffineMatrix() const override;

protected:
  /// For each dimension in the output, index in the input workspace of which
  /// dimension it is
  std::vector<size_t> m_dimensionToBinFrom;
  /// Offset (minimum) position in each of the output dimensions, sized [outD]
  std::vector<coord_t> m_origin;
  /// Scaling from the input to the output dimension, sized [outD]
  std::vector<coord_t> m_scaling;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_COORDTRANSFORMALIGNED_H_ */
