// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/VectorParameter.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {
/// Unique CoordCenterVectorParam type declaration for ndimensional coordinate
/// centers
DECLARE_VECTOR_PARAMETER(CoordCenterVectorParam, coord_t)

/// Unique DimensionsUsedVectorParam type declaration for boolean masks over
/// dimensions
DECLARE_VECTOR_PARAMETER(DimensionsUsedVectorParam, bool)

/** A non-linear coordinate transform that takes
 * a point from nd dimensions and converts it to a
 * single dimension: the SQUARE of the distance between the MDLeanEvent
 * and a given point in up to nd dimensions.
 *
 * The number of output dimensions is 1 (the square of the distance to the
 *point).
 *
 * The square is used instead of the plain distance since square root is a slow
 *calculation.
 *
 * @author Janik Zikovsky
 * @date 2011-04-25 14:48:33.517020
 */
class DLLExport CoordTransformDistance : public Mantid::API::CoordTransform {
public:
  CoordTransformDistance(const size_t inD, const coord_t *center, const bool *dimensionsUsed, const size_t outD = 1,
                         const std::vector<Kernel::V3D> &eigenvects = std::vector<Kernel::V3D>(0),
                         const std::vector<double> &eigenvals = std::vector<double>(0, 0.0));

  CoordTransform *clone() const override;
  std::string toXMLString() const override;
  std::string id() const override;

  void apply(const coord_t *inputVector, coord_t *outVector) const override;

  /// Return the center coordinate array
  const std::vector<coord_t> &getCenter() { return m_center; }

  /// Return the dimensions used bool array
  const std::vector<bool> &getDimensionsUsed() { return m_dimensionsUsed; }

protected:
  /// Coordinates at the center
  std::vector<coord_t> m_center;

  /// Parmeter where True is set for those dimensions that are considered when
  /// calculating distance
  std::vector<bool> m_dimensionsUsed;

  // Eigenvectors and radii for nd ellipsoid
  std::vector<Kernel::V3D> m_eigenvects;
  std::vector<double> m_eigenvals;
  double m_maxEigenval;
};
} // namespace DataObjects
} // namespace Mantid
