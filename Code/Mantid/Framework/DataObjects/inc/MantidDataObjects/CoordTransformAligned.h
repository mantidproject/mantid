#ifndef MANTID_DATAOBJECTS_COORDTRANSFORMALIGNED_H_
#define MANTID_DATAOBJECTS_COORDTRANSFORMALIGNED_H_

#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/VectorParameter.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {

/// Unique type declaration for which dimensions are used in the input workspace
DECLARE_VECTOR_PARAMETER(DimensionsToBinFromParam, size_t);

/// Unique type declaration for the offset of coordinates
DECLARE_VECTOR_PARAMETER(OriginOffsetParam, coord_t);

/// Unique type declaration for the step size in transformation
DECLARE_VECTOR_PARAMETER(ScalingParam, coord_t);

/** A restricted version of CoordTransform which transforms
  from one set of dimensions to another, allowing:

   - An offset
   - A reduction in the number of dimensions
   - A scaling

  While a normal Affine matrix would handle this case, this special
  case is used in order to reduce the number of calculation.

  @author Janik Zikovsky
  @date 2011-08-29

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CoordTransformAligned : public Mantid::API::CoordTransform {
public:
  CoordTransformAligned(const size_t inD, const size_t outD,
                        const size_t *dimensionToBinFrom, const coord_t *origin,
                        const coord_t *scaling);
  CoordTransformAligned(const size_t inD, const size_t outD,
                        const std::vector<size_t> dimensionToBinFrom,
                        const std::vector<coord_t> origin,
                        const std::vector<coord_t> scaling);
  virtual CoordTransform *clone() const;
  virtual ~CoordTransformAligned();

  std::string toXMLString() const;
  std::string id() const;
  void apply(const coord_t *inputVector, coord_t *outVector) const;
  Mantid::Kernel::Matrix<coord_t> makeAffineMatrix() const;

protected:
  /// For each dimension in the output, index in the input workspace of which
  /// dimension it is
  size_t *m_dimensionToBinFrom;
  /// Offset (minimum) position in each of the output dimensions, sized [outD]
  coord_t *m_origin;
  /// Scaling from the input to the output dimension, sized [outD]
  coord_t *m_scaling;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_COORDTRANSFORMALIGNED_H_ */
