// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_IBEAMPROFILE_H_
#define MANTID_ALGORITHMS_IBEAMPROFILE_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/Objects/BoundingBox.h"
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

  virtual ~IBeamProfile() = default;
  virtual Ray generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const = 0;
  virtual Ray generatePoint(Kernel::PseudoRandomNumberGenerator &rng,
                            const Geometry::BoundingBox &) const = 0;
  virtual Geometry::BoundingBox
  defineActiveRegion(const API::Sample &) const = 0;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_IBEAMPROFILE_H_ */
