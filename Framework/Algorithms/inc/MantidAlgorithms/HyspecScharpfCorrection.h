#ifndef MANTID_ALGORITHMS_HYSPECSCHARPFCORRECTION_H_
#define MANTID_ALGORITHMS_HYSPECSCHARPFCORRECTION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** HyspecScharpfCorrection : Divide by cos(2alpha) where alpha is the angle
  between incident beam and the polarization direction. It assumes scattering
  in the horizontal plane

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL HyspecScharpfCorrection : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void execEvent();
  /**
   * Execute Scharpf correction for event lists
   * @param wevector the list of events to correct
   * @param thPlane the in-plane angle for the detector corresponding to the
   * event list
   */
  template <class T>
  void ScharpfEventHelper(std::vector<T> &wevector, double thPlane);
  /**
   * @brief calculate the Scharph angle correction factor
   * @param kfki kf/ki
   * @param thPlane the in-plane angle of the detector
   * @return factor
   */
  float calculateFactor(const double kfki, const double thPlane);
  /// The user selected (input) workspace
  Mantid::API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output workspace, maybe the same as the input one
  Mantid::API::MatrixWorkspace_sptr m_outputWS;
  /// In plane angle beween polarization and incident beam (in degrees)
  double m_angle;
  /// Lower limit  for abs(cos(2*Scharpf angle)), below which intensities are 0
  double m_precision;
  /// Incident energy
  double m_Ei;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_HYSPECSCHARPFCORRECTION_H_ */
