// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_NONORTHOGONAL_AXIS_H_
#define MANTID_SLICEVIEWER_NONORTHOGONAL_AXIS_H_

#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <array>

namespace MantidQt {
namespace SliceViewer {

struct NonOrthogonalAxis {
  std::array<Mantid::coord_t, 9> fromHklToXyz;
  size_t dimX;
  size_t dimY;
  size_t dimMissing;
};

} // namespace SliceViewer
} // namespace MantidQt

#endif
