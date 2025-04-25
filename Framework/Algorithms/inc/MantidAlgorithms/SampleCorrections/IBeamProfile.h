// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace API {
class Sample;
}
namespace Kernel {
class PseudoRandomNumberGenerator;
}
namespace Algorithms {

/**

  Base class for all classes defining a beam profile.
*/
class MANTID_ALGORITHMS_DLL IBeamProfile {
public:
  struct Ray {
    Kernel::V3D startPos;
    Kernel::V3D unitDir;
  };

  IBeamProfile(const Kernel::V3D center);
  IBeamProfile() = default;
  virtual ~IBeamProfile() = default;
  virtual Ray generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const = 0;
  virtual Ray generatePoint(Kernel::PseudoRandomNumberGenerator &rng, const Geometry::BoundingBox &) const = 0;
  virtual Geometry::BoundingBox defineActiveRegion(const Geometry::BoundingBox &) const = 0;
  Geometry::IObject_sptr getIntersectionWithSample(const Geometry::IObject &sample) const;

protected:
  Kernel::V3D m_beamCenter;
};

} // namespace Algorithms
} // namespace Mantid
