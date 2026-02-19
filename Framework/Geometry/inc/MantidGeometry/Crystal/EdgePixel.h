// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include <string>

namespace Mantid {
namespace Geometry {

// forward declare
class ComponentInfo;

/// Function to find peaks near detector edge
MANTID_GEOMETRY_DLL bool edgePixel(ComponentInfo const &info, const std::string &bankName, int col, int row, int Edge);

} // namespace Geometry
} // namespace Mantid
