// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_MUONALPHACALC_H_
#define MANTID_ALGORITHM_MUONALPHACALC_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**Muon algorithm for calculating the detector efficiency between two groups of
detectors.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> ForwardSpectra - The spectra numbers of the forward group </LI>
<LI> BackwardSpectra - The spectra numbers of the backward group </LI>
<LI> FirstGoodValue - First good value </LI>
<LI> LastGoodValue - Last good value </LI>
<LI> Alpha (output) </LI>
</UL>


@author Anders Markvardsen, ISIS, RAL
@date 21/09/2010
*/
class MANTID_MUON_DLL AlphaCalc : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "AlphaCalc"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Muon algorithm for calculating the detector efficiency between two "
           "groups of detectors.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }
  /// See also
  const std::vector<std::string> seeAlso() const override {
    return {"AsymmetryCalc", "CalculateMuonAsymmetry"};
  };

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MUONALPHACALC_H_*/
