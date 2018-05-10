#ifndef MANTID_ALGORITHMS_GRAVITYSANSHELPER_H_
#define MANTID_ALGORITHMS_GRAVITYSANSHELPER_H_

#include "MantidKernel/V3D.h"

namespace Mantid {
namespace API {
class SpectrumInfo;
}
namespace Algorithms {
/** A helper class for calculating neutron's gravitional drop. Only works for
SANS because Mantid has no convention on which was is up or down. For this
to work y must increase with distance from the earth
@author Steve Williams, ISIS Rutherford Appleton Laboratory
@date 26/01/2009

Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class GravitySANSHelper {
public:
  GravitySANSHelper()
      : m_beamLineNorm(-1), m_dropPerAngstrom2(-1), m_cachedDrop(-1) {}
  GravitySANSHelper(const API::SpectrumInfo &spectrumInfo, const size_t index,
                    const double extraLength = 0.0);
  double calcSinTheta(const double wavAngstroms) const;
  double calcComponents(const double wavAngstroms, double &xFrac,
                        double &yFrac) const;

private:
  /// coordinates of the sample
  Kernel::V3D m_samplePos;
  /// the displacement from the source to the sample
  Kernel::V3D m_beamLine;
  /// twice the distance from the source to the sample
  double m_beamLineNorm;

  /// the drop is proportional to the wavelength squared, storing this drop
  /// increases calculation speed a lot
  double m_dropPerAngstrom2;
  /// the location that the neutron would have been detected at if it continued
  /// in a straight line, without gravity
  mutable Kernel::V3D m_cachedLineOfSight;
  /// the drop that was last caclulated, this allows m_cachedDetLoc to be
  /// recalculated each time without its original value being stored
  mutable double m_cachedDrop;

  const Kernel::V3D &getDetLoc(const double wav) const;
  /** Calculates the drop very much faster than running the workspace's
   * gravitationalDrop function assuming
   *  the drop is proportional to the wavelength squared
   *  @param wav :: the wave length in Angstrom
   *  @return the drop in meters
   */
  double gravitationalDrop(const double wav) const {
    return m_dropPerAngstrom2 * wav * wav;
  }

  double gravitationalDrop(const double L2, const double waveLength,
                           const double extraLength) const;
  double calcSinTheta() const;
};
} // namespace Algorithms
} // namespace Mantid
#endif /*#define MANTID_ALGORITHMS_GRAVITYSANSHELPER_H_*/
