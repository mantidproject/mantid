#ifndef MANTID_ALGORITHMS_GENERALISEDSECONDDIFFERENCE_H_
#define MANTID_ALGORITHMS_GENERALISEDSECONDDIFFERENCE_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Compute the generalised second difference of a spectrum or several spectra
 based on the method
 * described by M.A. Mariscotti., Nuclear Instruments and Methods 50, 309
 (1967).
 * This is used for example by a peak-finding algorithm.
 * Given a spectrum with value yi (0<=i<n), the generalised second difference
 corresponds to
 * the second difference curve, smoothed by averaging each point in the interval
 [-m,+m],
 * and applying this procedure z times.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace.
 </LI>
    <LI> m               - The number of points for averaging, i.e. summing will
 be done in the range [y(i-m),y(i+m)] </LI>
    <LI> z               - The number of iteration steps in the averaging
 procedure </LI>
    <LI> Spectra_min  - Lower bound of the spectra range </LI>
    <LI> Spectra_max  - Upper bound of the spectra range  </LI>
    </UL>

    @author Laurent C Chapon, ISIS Facility Rutherford Appleton Laboratory
    @date 26/12/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport GeneralisedSecondDifference : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override {
    return "GeneralisedSecondDifference";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Computes the generalised second difference of a spectrum or "
           "several spectra.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Arithmetic"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  ///
  void computePrefactors();
  /// Vector that contains the prefactor coefficients Cij in the range
  /// [-zm-1,zm+1]
  std::vector<double> m_Cij;
  /// Vector that contains the prefactor coefficients Cij^2 in the range
  /// [-zm-1,zm+1]
  std::vector<double> m_Cij2;
  /// Contains the value of the property z
  int m_z = 0;
  /// Contains the value of the property m
  int m_m = 0;
  /// Progress reporting
};

} // Namespace Algorithms
} // Namespace Mantid

#endif /* MANTID_ALGORITHMS_GENERALIZEDSECONDDIFFERENCE_H_ */
