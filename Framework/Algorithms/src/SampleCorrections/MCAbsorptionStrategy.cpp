// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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
 * @param nlambda The number of steps to use across the wavelength range
 * @param maxScatterPtAttempts The maximum number of tries to generate a random
 * @param useSparseInstrument Whether a sparse instrument representation is
 * being used
 * @param interpolateOpt Method to be used to interpolate between wavelength
 * points
 * @param regenerateTracksForEachLambda Whether to resimulate tracks for each
 * wavelength point or not
 * @param logger Logger from parent algorithm to write logging info
 * point within the object.
 */
MCAbsorptionStrategy::MCAbsorptionStrategy(
    const IBeamProfile &beamProfile, const API::Sample &sample,
    DeltaEMode::Type EMode, const size_t nevents, const int nlambda,
    const size_t maxScatterPtAttempts, const bool useSparseInstrument,
    const InterpolationOption &interpolateOpt,
    const bool regenerateTracksForEachLambda, Kernel::Logger &logger)
    : m_beamProfile(beamProfile),
      m_scatterVol(MCInteractionVolume(
          sample, beamProfile.defineActiveRegion(sample), logger)),
      m_nevents(nevents), m_maxScatterAttempts(maxScatterPtAttempts),
      m_error(1.0 / std::sqrt(m_nevents)), m_EMode(EMode), m_nlambda(nlambda),
      m_useSparseInstrument(useSparseInstrument),
      m_interpolateOpt(interpolateOpt),
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
 * @param attenuationFactorsSpectrum A spectrum containing the correction
 * factors and associated errors
 */
void MCAbsorptionStrategy::calculate(
    Kernel::PseudoRandomNumberGenerator &rng, const Kernel::V3D &finalPos,
    Mantid::HistogramData::Points lambdas, double lambdaFixed,
    Mantid::API::ISpectrum &attenuationFactorsSpectrum) {
  const auto scatterBounds = m_scatterVol.getBoundingBox();
  const auto nbins = static_cast<int>(lambdas.size());
  const int lambdaStepSize = nbins / m_nlambda;
  auto &attenuationFactors = attenuationFactorsSpectrum.mutableY();

  for (size_t i = 0; i < m_nevents; ++i) {
    Geometry::Track beforeScatter;
    Geometry::Track afterScatter;
    for (int j = 0; j < nbins; j += lambdaStepSize) {
      size_t attempts(0);
      do {
        const auto neutron = m_beamProfile.generatePoint(rng, scatterBounds);

        bool success = false;
        if (m_regenerateTracksForEachLambda || j == 0) {
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

          // Ensure we have the last point for the interpolation
          if (lambdaStepSize > 1 && j + lambdaStepSize >= nbins &&
              j + 1 != nbins) {
            j = nbins - lambdaStepSize - 1;
          }

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

  attenuationFactors = attenuationFactors / static_cast<double>(m_nevents);

  // Interpolate through points not simulated. Simulation WS only has
  // reduced X values if using sparse instrument so no interpolation required
  auto attenuationFactorsHist = attenuationFactorsSpectrum.histogram();

  if (!m_useSparseInstrument && lambdaStepSize > 1) {
    if (lambdaStepSize < nbins) {
      m_interpolateOpt.applyInplace(attenuationFactorsHist, lambdaStepSize);
    } else {
      std::fill(attenuationFactorsHist.mutableY().begin() + 1,
                attenuationFactorsHist.mutableY().end(),
                attenuationFactorsHist.y()[0]);
    }
  }

  std::fill(attenuationFactorsHist.mutableE().begin(),
            attenuationFactorsHist.mutableE().end(), m_error);
  // ISpectrum::histogram() returns a value rather than reference so need to
  // reapply
  attenuationFactorsSpectrum.setHistogram(attenuationFactorsHist);
}

} // namespace Algorithms
} // namespace Mantid
