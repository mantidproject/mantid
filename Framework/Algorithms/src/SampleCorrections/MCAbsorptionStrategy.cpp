#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
using Kernel::PseudoRandomNumberGenerator;

namespace Algorithms {

/**
 * Constructor
 * @param beamProfile A reference to the object the beam profile
 * @param sample A reference to the object defining details of the sample
 * @param nevents The number of Monte Carlo events used in the simulation
 */
MCAbsorptionStrategy::MCAbsorptionStrategy(const IBeamProfile &beamProfile,
                                           const API::Sample &sample,
                                           size_t nevents)
    : m_beamProfile(beamProfile), m_scatterVol(MCInteractionVolume(sample)),
      m_nevents(nevents), m_error(1.0 / std::sqrt(m_nevents)) {}

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
  // The simulation consists of sampling the beam profile and then computing the
  // absorption within the interacting volume defined by the sample (and
  // environment. The final correction factor is computed as the simple average
  // of m_nevents of this process.

  double factor(0.0);
  for (size_t i = 0; i < m_nevents; ++i) {
    auto neutron = m_beamProfile.generatePoint(rng);
    factor +=
        m_scatterVol.calculateAbsorption(rng, neutron.startPos, neutron.unitDir,
                                         finalPos, lambdaBefore, lambdaAfter);
  }

  using std::make_tuple;
  return make_tuple(factor / static_cast<double>(m_nevents), m_error);
}

} // namespace Algorithms
} // namespace Mantid
