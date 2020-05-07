// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/V3D.h"

#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidGeometry/Objects/CSGObject.h"

namespace Mantid {
using Kernel::DeltaEMode;
using Kernel::PseudoRandomNumberGenerator;

namespace Algorithms {

/**
 * Constructor
 * @param beamProfile A reference to the object the beam profile
 * @param sample A reference to the object defining details of the sample
 * @param EMode The energy mode of the instrument
 * @param nevents The number of Monte Carlo events used in the simulation
 * @param maxScatterPtAttempts The maximum number of tries to generate a random
 * @param regenerateTracksForEachLambda Whether to resimulate tracks for each
 * wavelength point or not
 * @param logger Logger from parent algorithm to write logging info
 * point within the object.
 */
MCAbsorptionStrategy::MCAbsorptionStrategy(
    const IBeamProfile &beamProfile, const API::Sample &sample,
    DeltaEMode::Type EMode, const size_t nevents,
    const size_t maxScatterPtAttempts, const bool regenerateTracksForEachLambda,
    Kernel::Logger &logger)
    : m_beamProfile(beamProfile),
      m_scatterVol(MCInteractionVolume(
          sample, beamProfile.defineActiveRegion(sample), logger)),
      m_nevents(nevents), m_maxScatterAttempts(maxScatterPtAttempts),
      m_error(1.0 / std::sqrt(m_nevents)), m_EMode(EMode),
      m_regenerateTracksForEachLambda(regenerateTracksForEachLambda) {}

/**
 * Compute the correction for a final position of the neutron and wavelengths
 * before and after scattering
 * @param rng A reference to a PseudoRandomNumberGenerator
 * @param finalPos Defines the final position of the neutron, assumed to be
 * where it is detected
 * @param lambdas Set of wavelength values from the input workspace
 * @param lambdaFixed Efixed value for a detector ID converted to wavelength, in
 * \f$\\A^-1\f$
 * @param attenuationFactors A vector containing the calculated correction
 * factors
 * @param attFactorErrors A vector containing the calculated correction factor
 * errors
 */
void MCAbsorptionStrategy::calculate(Kernel::PseudoRandomNumberGenerator &rng,
                                     const Kernel::V3D &finalPos,
                                     const std::vector<double> &lambdas,
                                     double lambdaFixed,
                                     std::vector<double> &attenuationFactors,
                                     std::vector<double> &attFactorErrors) {
  const auto scatterBounds = m_scatterVol.getBoundingBox();
  const auto nbins = static_cast<int>(lambdas.size());

  for (size_t i = 0; i < m_nevents; ++i) {
    Geometry::Track beforeScatter;
    Geometry::Track afterScatter;
    for (int j = 0; j < nbins; j++) {
      size_t attempts(0);
      do {
        bool success = false;
        if (m_regenerateTracksForEachLambda || j == 0) {
          const auto neutron = m_beamProfile.generatePoint(rng, scatterBounds);
          success = m_scatterVol.calculateBeforeAfterTrack(
              rng, neutron.startPos, finalPos, beforeScatter, afterScatter);
        } else {
          success = true;
        }
        if (!success) {
          ++attempts;
        } else {
          const double lambdaStep = lambdas[j];
          double lambdaIn(lambdaStep), lambdaOut(lambdaStep);
          if (m_EMode == DeltaEMode::Direct) {
            lambdaIn = lambdaFixed;
          } else if (m_EMode == DeltaEMode::Indirect) {
            lambdaOut = lambdaFixed;
          } else {
            // elastic case already initialized
          }
          const double wgt = m_scatterVol.calculateAbsorption(
              beforeScatter, afterScatter, lambdaIn, lambdaOut);
          attenuationFactors[j] += wgt;

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
  }

  m_scatterVol.generateScatterPointStats();

  std::transform(attenuationFactors.begin(), attenuationFactors.end(),
                 attenuationFactors.begin(),
                 std::bind(std::divides<double>(), std::placeholders::_1,
                           static_cast<double>(m_nevents)));

  std::fill(attFactorErrors.begin(), attFactorErrors.end(), m_error);
}

} // namespace Algorithms
} // namespace Mantid
