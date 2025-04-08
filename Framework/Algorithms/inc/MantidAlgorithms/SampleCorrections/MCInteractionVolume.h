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
  enum class ScatteringPointVicinity { SAMPLEANDENVIRONMENT, SAMPLEONLY, ENVIRONMENTONLY };
  static std::shared_ptr<IMCInteractionVolume>
  create(const API::Sample &sample, const size_t maxScatterAttempts = 5000,
         const ScatteringPointVicinity pointsIn = ScatteringPointVicinity::SAMPLEANDENVIRONMENT,
         Geometry::IObject_sptr gaugeVolume = nullptr);
  const Geometry::BoundingBox getFullBoundingBox() const override;
  virtual TrackPair calculateBeforeAfterTrack(Kernel::PseudoRandomNumberGenerator &rng, const Kernel::V3D &startPos,
                                              const Kernel::V3D &endPos, MCInteractionStatistics &stats) const override;
  ComponentScatterPoint generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const override;
  void setActiveRegion(const Geometry::BoundingBox &region) override;
  Geometry::IObject_sptr getGaugeVolume() const override;
  void setGaugeVolume(Geometry::IObject_sptr gaugeVolume) override;

private:
  MCInteractionVolume(const API::Sample &sample, const size_t maxScatterAttempts = 5000,
                      const ScatteringPointVicinity pointsIn = ScatteringPointVicinity::SAMPLEANDENVIRONMENT,
                      Geometry::IObject_sptr gaugeVolume = nullptr);
  int getComponentIndex(Kernel::PseudoRandomNumberGenerator &rng) const;
  std::optional<Kernel::V3D> generatePointInObjectByIndex(int componentIndex,
                                                          Kernel::PseudoRandomNumberGenerator &rng) const;
  const std::shared_ptr<Geometry::IObject> m_sample;
  const Geometry::SampleEnvironment *m_env;
  Geometry::BoundingBox m_activeRegion;
  const size_t m_maxScatterAttempts;
  const ScatteringPointVicinity m_pointsIn;
  Geometry::IObject_sptr m_gaugeVolume;

protected:
  void init() override;
};

} // namespace Algorithms
} // namespace Mantid
