// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionStatistics.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/Logger.h"
#include <boost/optional.hpp>

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class IObject;
class SampleEnvironment;
class Track;
} // namespace Geometry

namespace Kernel {
class PseudoRandomNumberGenerator;
class V3D;
} // namespace Kernel
namespace Algorithms {
class IBeamProfile;

struct ComponentScatterPoint {
  int componentIndex;
  Kernel::V3D scatterPoint;
};

/**
  Defines a volume where interactions of Tracks and Objects can take place.
  Given an initial Track, end point & wavelengths it calculates the absorption
  correction factor.
*/
class MANTID_ALGORITHMS_DLL MCInteractionVolume {
public:
  MCInteractionVolume(const API::Sample &sample,
                      const Geometry::BoundingBox &activeRegion,
                      const size_t maxScatterAttempts = 5000);
  // No creation from temporaries as we store a reference to the object in
  // the sample
  MCInteractionVolume(const API::Sample &&sample,
                      const Geometry::BoundingBox &&activeRegion) = delete;

  const Geometry::BoundingBox &getBoundingBox() const;
  bool calculateBeforeAfterTrack(Kernel::PseudoRandomNumberGenerator &rng,
                                 const Kernel::V3D &startPos,
                                 const Kernel::V3D &endPos,
                                 Geometry::Track &beforeScatter,
                                 Geometry::Track &afterScatter,
                                 MCInteractionStatistics &stats) const;
  double calculateAbsorption(const Geometry::Track &beforeScatter,
                             const Geometry::Track &afterScatter,
                             double lambdaBefore, double lambdaAfter) const;
  ComponentScatterPoint
  generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const;

private:
  int getComponentIndex(Kernel::PseudoRandomNumberGenerator &rng) const;
  boost::optional<Kernel::V3D>
  generatePointInObjectByIndex(int componentIndex,
                               Kernel::PseudoRandomNumberGenerator &rng) const;
  const std::shared_ptr<Geometry::IObject> m_sample;
  const Geometry::SampleEnvironment *m_env;
  const Geometry::BoundingBox m_activeRegion;
  const size_t m_maxScatterAttempts;
};

} // namespace Algorithms
} // namespace Mantid
