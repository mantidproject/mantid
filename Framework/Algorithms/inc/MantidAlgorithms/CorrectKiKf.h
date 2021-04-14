// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/DllConfig.h"

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
*/

class MANTID_ALGORITHMS_DLL CorrectKiKf : public API::Algorithm {
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
  template <class T> void correctKiKfEventHelper(std::vector<T> &wevector, double efixed, const std::string &emodeStr);
  void getEfixedFromParameterMap(double &Efi, int64_t i, const Mantid::API::SpectrumInfo &spectrumInfo,
                                 const Mantid::Geometry::ParameterMap &pmap);
};

} // namespace Algorithms
} // namespace Mantid
