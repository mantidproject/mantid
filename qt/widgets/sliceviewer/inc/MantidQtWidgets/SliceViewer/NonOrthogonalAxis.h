// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <array>
#include <cstddef>

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
