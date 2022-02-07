// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Geometry {

/** A general N-dimensional plane implicit function.
  This relies on MDPlane to do the heavy lifting. The main thing for
  this class is to be able to specify itself via XML.

  @date 2011-13-12
*/
class DLLExport MDPlaneImplicitFunction : public MDImplicitFunction {
public:
  /// Default constructor.
  MDPlaneImplicitFunction();
  /// Parameter constructor for setting origin.
  MDPlaneImplicitFunction(const size_t nd, const float *normal, const float *point);
  MDPlaneImplicitFunction(const size_t nd, const double *normal, const double *point);

  /// Overriding the addPlane for check
  void addPlane(const MDPlane &plane);

  /// @return the MDPlaneImplicitFunction type name.
  std::string getName() const override;
  /// @return the XML representation of the MDPlaneImplicitFunction
  std::string toXMLString() const override;

private:
  /// Set defaults to origin if not used.
  void checkOrigin();
  /// Create string for coordinate values.
  std::string coordValue(const coord_t *arr) const;

  std::vector<coord_t> origin; ///< The origin point of the implicit plane.
};

} // namespace Geometry
} // namespace Mantid
