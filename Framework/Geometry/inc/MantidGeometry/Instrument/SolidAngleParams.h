// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Geometry {

class MANTID_GEOMETRY_DLL SolidAngleParams {
public:
  SolidAngleParams(Kernel::V3D observer, int numberOfCylinderSlices = 10)
      : m_observer(std::move(observer)), m_numberOfCylinderSlices(numberOfCylinderSlices) {}
  inline const Kernel::V3D &observer() const { return m_observer; }
  inline int cylinderSlices() const { return m_numberOfCylinderSlices; }
  inline const SolidAngleParams copyWithNewObserver(Kernel::V3D newObserver) const {
    return SolidAngleParams(std::move(newObserver), m_numberOfCylinderSlices);
  }

private:
  Kernel::V3D m_observer;
  int m_numberOfCylinderSlices;
};

} // namespace Geometry
} // namespace Mantid
