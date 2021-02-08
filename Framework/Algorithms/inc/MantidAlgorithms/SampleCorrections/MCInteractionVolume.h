// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/IMCInteractionVolume.h"

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class SampleEnvironment;
} // namespace Geometry

namespace Algorithms {
class IBeamProfile;

/**
  Defines a volume where interactions of Tracks and Objects can take place.
  Given an initial Track, end point & wavelengths it calculates the absorption
  correction factor.
*/
class MANTID_ALGORITHMS_DLL MCInteractionVolume : public IMCInteractionVolume {
public:
  enum class ScatteringPointVicinity {
    SAMPLEANDENVIRONMENT,
    SAMPLEONLY,
    ENVIRONMENTONLY
  };
  MCInteractionVolume(const API::Sample &sample,
                      const size_t maxScatterAttempts = 5000,
                      const ScatteringPointVicinity pointsIn =
                          ScatteringPointVicinity::SAMPLEANDENVIRONMENT);

  const Geometry::BoundingBox &getBoundingBox() const override;
  const Geometry::BoundingBox getFullBoundingBox() const override;
  virtual TrackPair calculateBeforeAfterTrack(
      Kernel::PseudoRandomNumberGenerator &rng, const Kernel::V3D &startPos,
      const Kernel::V3D &endPos, MCInteractionStatistics &stats) const override;
  ComponentScatterPoint
  generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const;
  void setActiveRegion(const Geometry::BoundingBox &region) override;

private:
  int getComponentIndex(Kernel::PseudoRandomNumberGenerator &rng) const;
  boost::optional<Kernel::V3D>
  generatePointInObjectByIndex(int componentIndex,
                               Kernel::PseudoRandomNumberGenerator &rng) const;
  const std::shared_ptr<Geometry::IObject> m_sample;
  const Geometry::SampleEnvironment *m_env;
  Geometry::BoundingBox m_activeRegion;
  const size_t m_maxScatterAttempts;
  const ScatteringPointVicinity m_pointsIn;
};

} // namespace Algorithms
} // namespace Mantid
