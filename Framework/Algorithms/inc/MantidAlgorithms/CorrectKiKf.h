#ifndef MANTID_ALGORITHMS_CORRECTKIKF_H_
#define MANTID_ALGORITHMS_CORRECTKIKF_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectrumInfo.h"

namespace Mantid {
namespace Algorithms {
/**	Multiplies the workspace with k_i/k_f
        The initial workspace must have energy transfer units.
  The algorithm will throw an error if the input workspace is an event workspace

        For Direct Geometry instruments the fixed energy is E_i, and the final
  energy E_f=Ei-E.
        For Indirect Geometry, the fixed energy is E_f, the initial energy is
  E_i=E+E_f.
        Each element of the workspace is multiplied by k_i/k_f=SQRT(E_i/E_f)

        Required Properties:
    <UL>
    <LI> InputWorkspace  	- The name of the input workspace. </LI>
    <LI> OutputWorkspace 	- The name of the output workspace. </LI>
          <LI> Emode  			    - The energy mode (1=direct
  geometry, 2=indirect geometry) </LI>
    <LI> Efixed 			    - Value of fixed energy: EI
  (emode=1)
  or
  EF
  (emode=2) (meV) </LI>
    </UL>

  To do: check if instrument type is same as Emode

    @author Andrei Savici
    @date 10/21/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport CorrectKiKf : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CorrectKiKf"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs k_i/k_f multiplication, in order to transform "
           "differential scattering cross section into dynamic structure "
           "factor.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Inelastic\\Corrections;CorrectionFunctions\\SpecialCorrections";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  void execEvent();
  /**
   * Execute CorrectKiKf for event lists
   * @param wevector the list of events to correct
   * @param efixed the value of the fixed energy
   * @param emodeStr the energy mode description
   */
  template <class T>
  void correctKiKfEventHelper(std::vector<T> &wevector, double efixed,
                              const std::string emodeStr);
  void getEfixedFromParameterMap(double &Efi, int64_t i,
                                 const Mantid::API::SpectrumInfo &spectrumInfo,
                                 const Mantid::Geometry::ParameterMap &pmap);
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CORRECTKIKF_H_*/
