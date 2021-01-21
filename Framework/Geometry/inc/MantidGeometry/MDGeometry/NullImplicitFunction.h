// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include <gsl/gsl_blas.h>
#include <vector>

namespace Mantid {
namespace Geometry {
/**
This class represents a Null Implicit function. This type prevents the
requirement
for Null checks and handles where ImplicitFunctions are used.

@author Owen Arnold, Tessella plc
@date 28/01/2011
*/

class DLLExport NullImplicitFunction : public Mantid::Geometry::MDImplicitFunction {
public:
  std::string getName() const override;
  std::string toXMLString() const override;
  //----------------------MDImplicit function methods ------------
  bool isPointContained(const coord_t *) override { return true; }
  bool isPointContained(const std::vector<coord_t> &) override { return true; }
  // Unhide base class methods (avoids Intel compiler warning)
  using MDImplicitFunction::isPointContained;
  //---------------------------------------------------------------
  static std::string functionName() { return "NullImplicitFunction"; }
};
} // namespace Geometry
} // namespace Mantid
