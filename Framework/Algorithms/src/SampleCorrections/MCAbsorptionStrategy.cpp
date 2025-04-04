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

#include "MantidGeometry/Objects/CSGObject.h"

namespace Mantid {
using Kernel::DeltaEMode;
using Kernel::PseudoRandomNumberGenerator;

namespace Algorithms {

/**
 * Constructor
 * @param interactionVolume A reference to the MCInteractionVolume dependency
 * @param beamProfile A reference to the object the beam profile
 * @param EMode The energy mode of the instrument
 * @param nevents The number of Monte Carlo events used in the simulation
 * @param maxScatterPtAttempts The maximum number of tries to generate a random
 * point within the object
 * @param regenerateTracksForEachLambda Whether to resimulate tracks for each
 * wavelength point or not
 */
MCAbsorptionStrategy::MCAbsorptionStrategy(std::shared_ptr<IMCInteractionVolume> interactionVolume,
                                           const IBeamProfile &beamProfile, DeltaEMode::Type EMode,
                                           const size_t nevents, const size_t maxScatterPtAttempts,
                                           const bool regenerateTracksForEachLambda)
    : m_beamProfile(beamProfile), m_scatterVol(std::move(interactionVolume)), m_nevents(nevents),
      m_maxScatterAttempts(maxScatterPtAttempts), m_EMode(EMode),
      m_regenerateTracksForEachLambda(regenerateTracksForEachLambda) {

  setActiveRegion(m_scatterVol, beamProfile);
}

/**
 * Set the active region on the interaction volume as smaller of the sample
 * bounding box and the beam cross section. Trying to keep the beam details
 * outside the interaction volume class
 * @param interactionVolume The interaction volume object
 * @param beamProfile The beam profile
 * @return The interaction volume object with active region set
 */
void MCAbsorptionStrategy::setActiveRegion(std::shared_ptr<IMCInteractionVolume> interactionVolume,
                                           const IBeamProfile &beamProfile) {
  interactionVolume->setActiveRegion(beamProfile.defineActiveRegion(interactionVolume->getFullBoundingBox()));
}

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
 * @param stats A statistics class to hold the statistics on the generated
 * tracks such as the scattering angle and count of scatter points generated in
 * each sample or environment part
 */
void MCAbsorptionStrategy::calculate(Kernel::PseudoRandomNumberGenerator &rng, const Kernel::V3D &finalPos,
                                     const std::vector<double> &lambdas, const double lambdaFixed,
                                     std::vector<double> &attenuationFactors, std::vector<double> &attFactorErrors,
                                     MCInteractionStatistics &stats) {
  const auto scatterBounds = m_scatterVol->getFullBoundingBox();
  const auto nbins = static_cast<int>(lambdas.size());
  Geometry::IObject_sptr gv = m_scatterVol->getGaugeVolume();

  std::vector<double> wgtMean(attenuationFactors.size()), wgtM2(attenuationFactors.size());

  for (size_t i = 0; i < m_nevents; ++i) {
    std::shared_ptr<Geometry::Track> beforeScatter;
    std::shared_ptr<Geometry::Track> afterScatter;
    for (int j = 0; j < nbins; ++j) {
      size_t attempts(0);
      do {
        bool success = false;
        if (m_regenerateTracksForEachLambda || j == 0) {
          const auto neutron = m_beamProfile.generatePoint(rng, scatterBounds);
          std::tie(success, beforeScatter, afterScatter) =
              m_scatterVol->calculateBeforeAfterTrack(rng, neutron.startPos, finalPos, stats);
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
          const double wgt =
              beforeScatter->calculateAttenuation(lambdaIn) * afterScatter->calculateAttenuation(lambdaOut);
          attenuationFactors[j] += wgt;
          // increment standard deviation using Welford algorithm
          double delta = wgt - wgtMean[j];
          wgtMean[j] += delta / static_cast<double>(i + 1);
          wgtM2[j] += delta * (wgt - wgtMean[j]);
          // calculate sample SD (M2/n-1)
          // will give NaN for m_events=1, but that's correct
          attFactorErrors[j] = sqrt(wgtM2[j] / static_cast<double>(i));

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

  std::transform(attenuationFactors.begin(), attenuationFactors.end(), attenuationFactors.begin(),
                 std::bind(std::divides<double>(), std::placeholders::_1, static_cast<double>(m_nevents)));

  // calculate standard deviation of mean from sample mean
  std::transform(attFactorErrors.begin(), attFactorErrors.end(), attFactorErrors.begin(),
                 [this](double v) -> double { return v / sqrt(static_cast<double>(m_nevents)); });
}

} // namespace Algorithms
} // namespace Mantid
