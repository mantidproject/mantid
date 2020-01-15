// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MCINTERACTIONVOLUME_H_
#define MANTID_ALGORITHMS_MCINTERACTIONVOLUME_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/Objects/BoundingBox.h"

namespace Mantid {
namespace API {
class Sample;
}
namespace Geometry {
class IObject;
class SampleEnvironment;
} // namespace Geometry

namespace Kernel {
class PseudoRandomNumberGenerator;
class V3D;
} // namespace Kernel
namespace Algorithms {
class IBeamProfile;

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
  double calculateAbsorption(Kernel::PseudoRandomNumberGenerator &rng,
                             const Kernel::V3D &startPos,
                             const Kernel::V3D &endPos, double lambdaBefore,
                             double lambdaAfter) const;
  std::string generateScatterPointStats() const;
  Kernel::V3D generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const;

private:
  mutable int m_sampleScatterPoints = 0;
  mutable std::vector<int> m_envScatterPoints;
  const boost::shared_ptr<Geometry::IObject> m_sample;
  const Geometry::SampleEnvironment *m_env;
  const Geometry::BoundingBox m_activeRegion;
  const size_t m_maxScatterAttempts;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MCINTERACTIONVOLUME_H_ */
