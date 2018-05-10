#include "MantidAlgorithms/TOFSANSResolutionByPixelCalculator.h"
#include <cmath>
namespace Mantid {
namespace Algorithms {

/**
 * Calculates the wavelength-independent prefactor for the resolutin calculation
 * @param r1: the aperture of the source
 * @param r2: the aperture of the sample
 * @param deltaR: the radius uncertainty
 * @param lCollim: the collimation length
 * @param l2: the L2 distance
 * @returns the prefactor
 */
double TOFSANSResolutionByPixelCalculator::getWavelengthIndependentFactor(
    double r1, double r2, double deltaR, double lCollim, double l2) const {
  return (4.0 * M_PI * M_PI / 12.0) *
         (3.0 * r1 * r1 / (lCollim * lCollim) +
          3.0 * r2 * r2 * (lCollim + l2) * (lCollim + l2) /
              (lCollim * lCollim * l2 * l2) +
          (deltaR * deltaR) / (l2 * l2));
}

/*
 * Calculates the sigmaQ value
 * @param moderatorValue: The value of the moderator for a certain wavelength
 * @param wavlengthIndependentFactor: the geometry-dependent prefactor
 * @param q: the momentum transfer
 * @param wavelength: the neutron's wavelength
 * @param deltaWavelength: the wavelength spread for that wavelength
 * @param l1: the l1 length
 * @param l2: the L2 length
 * @returns the sigma q value
 */

double TOFSANSResolutionByPixelCalculator::getSigmaQValue(
    double moderatorValue, double wavlengthIndependentFactor, double q,
    double wavelength, double deltaWavelength, double l1, double l2) const {
  // Calculate the moderator uncertainty
  const double sigModerator = moderatorValue * 3.9560 / (1000.0 * (l1 + l2));

  // Calculate the wavelength uncertainty
  const double sigWavelengthSquared =
      deltaWavelength * deltaWavelength / 12.0 + sigModerator * sigModerator;

  // Calculate the q uncertainty
  const double qbyWavelengthSquared = q * q / (wavelength * wavelength);

  return sqrt(wavlengthIndependentFactor / (wavelength * wavelength) +
              (sigWavelengthSquared * qbyWavelengthSquared));
}
} // namespace Algorithms
} // namespace Mantid
