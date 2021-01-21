// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"

namespace Mantid {
namespace Geometry {

std::string NullImplicitFunction::getName() const { return NullImplicitFunction::functionName(); }

std::string NullImplicitFunction::toXMLString() const { return std::string(); }
} // namespace Geometry
} // namespace Mantid
