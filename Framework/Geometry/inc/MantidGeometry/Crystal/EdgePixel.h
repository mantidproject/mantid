// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_EDGEPIXEL_H_
#define MANTID_GEOMETRY_EDGEPIXEL_H_

#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Geometry {

/// Function to find peaks near detector edge
MANTID_GEOMETRY_DLL bool edgePixel(Geometry::Instrument_const_sptr inst,
                                   std::string bankName, int col, int row,
                                   int Edge);

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_EDGEPIXEL_H_ */
