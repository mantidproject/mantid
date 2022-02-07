// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//--------------------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------------------
#pragma once

#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include <MantidKernel/V3D.h>

namespace Mantid::Geometry::MDAlgorithms {
class MDBoxMaskFunction : public Mantid::Geometry::MDImplicitFunction {
private:
  Mantid::Kernel::V3D m_pos;
  double m_radiusSquared;

public:
  // constructor
  MDBoxMaskFunction(const Mantid::Kernel::V3D &pos, const double &radiusSquared) {
    m_pos = pos;
    m_radiusSquared = radiusSquared;
  }
  using MDImplicitFunction::isPointContained; // Avoids Intel compiler
                                              // warning.
  bool isPointContained(const coord_t *coords) override {
    double sum = 0;
    for (size_t i = 0; i < 3; i++) {
      sum += pow(coords[i] - m_pos[i], 2);
    }
    if (sum < m_radiusSquared) {
      return true;
    } else {
      return false;
    }
  }
};
} // namespace Mantid::Geometry::MDAlgorithms