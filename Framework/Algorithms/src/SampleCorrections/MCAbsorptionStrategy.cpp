#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/V3D.h"

#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidGeometry/Objects/Object.h"

namespace Mantid {
using Kernel::PseudoRandomNumberGenerator;

namespace Algorithms {

/**
 * Constructor
 * @param beamProfile A reference to the object the beam profile
 * @param sample A reference to the object defining details of the sample
 * @param nevents The number of Monte Carlo events used in the simulation
 * @param maxScatterPtAttempts The maximum number of tries to generate a random
 * point within the object.
 */
MCAbsorptionStrategy::MCAbsorptionStrategy(const IBeamProfile &beamProfile,
                                           const API::Sample &sample,
                                           size_t nevents,
                                           size_t maxScatterPtAttempts)
    : m_beamProfile(beamProfile),
      m_scatterVol(
          MCInteractionVolume(sample, beamProfile.defineActiveRegion(sample))),
      m_nevents(nevents), m_maxScatterAttempts(maxScatterPtAttempts),
      m_error(1.0 / std::sqrt(m_nevents)) {}

/**
 * Compute the correction for a final position of the neutron and wavelengths
 * before and after scattering
 * @param rng A reference to a PseudoRandomNumberGenerator
 * @param finalPos Defines the final position of the neutron, assumed to be
 * where it is detected
 * @param lambdaBefore Wavelength, in \f$\\A^-1\f$, before scattering
 * @param lambdaAfter Wavelength, in \f$\\A^-1\f$, after scattering
 * @return A tuple of the <correction factor, associated error>.
 */
std::tuple<double, double>
MCAbsorptionStrategy::calculate(Kernel::PseudoRandomNumberGenerator &rng,
                                const Kernel::V3D &finalPos,
                                double lambdaBefore, double lambdaAfter) const {
  const auto scatterBounds = m_scatterVol.getBoundingBox();
  double factor(0.0);
  for (size_t i = 0; i < m_nevents; ++i) {
    size_t attempts(0);
    do {
      const auto neutron = m_beamProfile.generatePoint(rng, scatterBounds);

      const double wgt = m_scatterVol.calculateAbsorption(
          rng, neutron.startPos, finalPos, lambdaBefore, lambdaAfter);
      if (wgt < 0.0) {
        ++attempts;
      } else {
        factor += wgt;
        break;
      }
      if (attempts == m_maxScatterAttempts) {
        throw std::runtime_error("Unable to generate valid track through "
                                 "sample interaction volume after " +
                                 std::to_string(m_maxScatterAttempts) +
                                 " attempts. Try increasing the maximum "
                                 "threshold or if this does not help then "
                                 "please check the defined shape.");
      }
    } while (true);
  }
  using std::make_tuple;
  return make_tuple(factor / static_cast<double>(m_nevents), m_error);
}

} // namespace Algorithms
} // namespace Mantid
