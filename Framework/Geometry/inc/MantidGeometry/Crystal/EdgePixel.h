// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Geometry {

/// Function to find peaks near detector edge
MANTID_GEOMETRY_DLL bool edgePixel(const Geometry::Instrument_const_sptr &inst, const std::string &bankName, int col,
                                   int row, int Edge);

} // namespace Geometry
} // namespace Mantid
