// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GravitySANSHelper.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/PhysicalConstants.h"

#include <cmath>

namespace Mantid {
namespace Algorithms {
using Kernel::V3D;

/** sets up the object with workspace data and calculates cached values ready to
 * calculate gravitional effects across a spectrum
 *  @param spectrumInfo :: SpectrumInfo of workspace
 *  @param index :: Workspace index of spectrum to do the calculation for
 *  @param extraLength :: extra length for gravity correction
 */
GravitySANSHelper::GravitySANSHelper(const API::SpectrumInfo &spectrumInfo,
                                     const size_t index,
                                     const double extraLength)
    : m_beamLineNorm(-1), m_dropPerAngstrom2(-1), m_cachedDrop(0) {
  m_samplePos = spectrumInfo.samplePosition();
  m_beamLine = m_samplePos - spectrumInfo.sourcePosition();
  m_beamLineNorm = 2.0 * spectrumInfo.l1();

  // this is the LineOfSight assuming no drop, the drop is added (and
  // subtracted) later in the code when required
  m_cachedLineOfSight = spectrumInfo.position(index) - m_samplePos;
  // the drop is proportional to the wave length squared and using this to do
  // the full calculation only once increases the speed a lot
  m_dropPerAngstrom2 =
      gravitationalDrop(spectrumInfo.l2(index), 1e-10, extraLength);
}
/** Caclulates the sin of the that the neutron left the sample at, before the
 * effect of gravity
 *  @param wavAngstroms :: the neutrons' wave length in Angstoms
 *  @return the sin of theta
 */
double GravitySANSHelper::calcSinTheta(const double wavAngstroms) const {
  getDetLoc(wavAngstroms);
  return calcSinTheta();
}
/** Calculate the sins and cosins of angles as required to calculate Q is 2
 * dimensions
 *  @param[in] wavAngstroms wavelength of the neutron in Angstrom
 *  @param[out] xFrac the component in the x direction
 *  @param[out] yFrac component in the y direction
 *  @return sin of the angle theta to the detector
 */
double GravitySANSHelper::calcComponents(const double wavAngstroms,
                                         double &xFrac, double &yFrac) const {
  const V3D &detPos = getDetLoc(wavAngstroms);

  const double phi = atan2(detPos.Y(), detPos.X());
  xFrac = cos(phi);
  yFrac = sin(phi);
  return calcSinTheta();
}
/** Finds the location of the detector the neutron would have entered if it
 * followed a
 *  straight line path from the sample
 *  @param wav :: wavelength of the neutron in Angstrom
 *  @return a reference to the cached location
 */
const V3D &GravitySANSHelper::getDetLoc(const double wav) const {
  // Calculate the drop
  const double drop = gravitationalDrop(wav);
  // I'm fairly confident that Y is up! Using the previous drop to allow the
  // same V3D to be modified many times
  m_cachedLineOfSight[1] += drop - m_cachedDrop;
  m_cachedDrop = drop;
  // must return a member variable, not a local!
  return m_cachedLineOfSight;
}
/** getDetLoc must have been called before this is used to calculate the sin of
 * the angle
 *  @return the sin of theta
 */
double GravitySANSHelper::calcSinTheta() const {
  // This is 0.5*cos(2theta)
  double halfcosTheta = m_cachedLineOfSight.scalar_prod(m_beamLine) /
                        m_cachedLineOfSight.norm() / m_beamLineNorm;
  // This is sin(theta)
  return sqrt(0.5 - halfcosTheta);
}

/** Calculates the distance a neutron coming from the sample will have deviated
 * from a
 *  straight tragetory before hitting a detector. If calling this function many
 * times
 *  for the same detector you can call this function once, with waveLength=1,
 * and use the fact drop is proportional to wave length squared .This function
 * has no knowledge of which axis is vertical for a given instrument
 *  @param l2 :: distance between sample and detector the neutron entered
 *  @param waveLength :: the neutrons wave length in meters
 *  @param extraLength :: additional length
 *  @return the deviation in meters
 */
double GravitySANSHelper::gravitationalDrop(const double l2,
                                            const double waveLength,
                                            const double extraLength) const {
  using namespace PhysicalConstants;
  /// Pre-factor in gravity calculation: gm^2/2h^2
  static const double gm2_OVER_2h2 =
      g * NeutronMass * NeutronMass / (2.0 * h * h);

  // Perform a path length correction if an Lextra is specified.
  // The correction is Lcorr^2 = (L + Lextra)^2 -(LExtra)^2
  const auto pathLengthWithExtraLength = l2 + extraLength;
  const auto pathLengthSquared =
      std::pow(pathLengthWithExtraLength, 2) - std::pow(extraLength, 2);

  // Want L2 (sample-pixel distance) squared, times the prefactor g^2/h^2
  const double L2 = gm2_OVER_2h2 * pathLengthSquared;

  return waveLength * waveLength * L2;
}
} // namespace Algorithms
} // namespace Mantid
