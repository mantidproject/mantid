#ifndef MANTID_ALGORITHMS_MCABSORPTIONSTRATEGY_H_
#define MANTID_ALGORITHMS_MCABSORPTIONSTRATEGY_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include <tuple>

namespace Mantid {
namespace API {
class Sample;
}
namespace Kernel {
class PseudoRandomNumberGenerator;
class V3D;
} // namespace Kernel
namespace Algorithms {
class IBeamProfile;

/**
  Implements the algorithm for calculating the correction factor for
  self-attenuation and single wavelength using a Monte Carlo. A single
  instance has a fixed nominal source position, nominal sample
  position & sample + containers shapes.

  The error on all points is defined to be \f$\frac{1}{\sqrt{N}}\f$, where N is
  the number of events generated.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL MCAbsorptionStrategy {
public:
  MCAbsorptionStrategy(const IBeamProfile &beamProfile,
                       const API::Sample &sample, size_t nevents,
                       size_t maxScatterPtAttempts);
  std::tuple<double, double> calculate(Kernel::PseudoRandomNumberGenerator &rng,
                                       const Kernel::V3D &finalPos,
                                       double lambdaBefore,
                                       double lambdaAfter) const;

private:
  const IBeamProfile &m_beamProfile;
  const MCInteractionVolume m_scatterVol;
  const size_t m_nevents;
  const size_t m_maxScatterAttempts;
  const double m_error;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MCABSORPTIONSTRATEGY_H_ */
