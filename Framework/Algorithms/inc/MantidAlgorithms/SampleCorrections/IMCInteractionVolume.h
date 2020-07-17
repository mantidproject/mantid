// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionStatistics.h"
#include "MantidGeometry/Objects/BoundingBox.h"

namespace Mantid {
namespace Geometry {
class IObject;
class Track;
} // namespace Geometry

namespace Kernel {
class PseudoRandomNumberGenerator;
class V3D;
} // namespace Kernel
namespace Algorithms {

struct ComponentScatterPoint {
  int componentIndex;
  Kernel::V3D scatterPoint;
};

/**
  Defines a base class for objects describing a volume where interactions of
  Tracks and Objects can take place.
*/
class MANTID_ALGORITHMS_DLL IMCInteractionVolume {
public:
  virtual ~IMCInteractionVolume() = default;
  virtual bool calculateBeforeAfterTrack(
      Kernel::PseudoRandomNumberGenerator &rng, const Kernel::V3D &startPos,
      const Kernel::V3D &endPos, Geometry::Track &beforeScatter,
      Geometry::Track &afterScatter, MCInteractionStatistics &stats) const = 0;
  virtual double calculateAbsorption(const Geometry::Track &beforeScatter,
                                     const Geometry::Track &afterScatter,
                                     double lambdaBefore,
                                     double lambdaAfter) const = 0;
  virtual const Geometry::BoundingBox &getBoundingBox() const = 0;
};

} // namespace Algorithms
} // namespace Mantid
