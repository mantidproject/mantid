// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidKernel/V3D.h"

#include <array>

namespace Mantid {
namespace Geometry {
class ReferenceFrame;
}
namespace Algorithms {

/**
  Defines a Rectangular Beam profile explicitly accounting for beam divergence
  allowing for a representative sampling of the neutron distribution
*/
class MANTID_ALGORITHMS_DLL DivergentSlitBeamProfile final : public RectangularBeamProfile {
public:
  DivergentSlitBeamProfile(const Geometry::ReferenceFrame &frame, const Kernel::V3D &center, double width,
                           double height, double horDiv, double verDiv, double slitD);
  double intensityAt(const Kernel::V3D &scatteringPoint) const override;

private:
  const double m_horDiv;
  const double m_verDiv;
  const double m_slitD;
};

} // namespace Algorithms
} // namespace Mantid
