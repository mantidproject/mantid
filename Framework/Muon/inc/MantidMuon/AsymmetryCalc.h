// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**Takes a muon workspace as input and sums all the spectra into two spectra
which represent
      the two detector groupings. The resultant spectra are used to calculate
(F-aB) / (F+aB) the results of which
      are stored in the output workspace.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> ForwardSpectra - The detector number of the first group </LI>
<LI> BackwardSpectra - The detector number of the second group </LI>
<LI> Alpha - ?? </LI>
</UL>


@author
@date 11/07/2008
*/
class MANTID_MUON_DLL AsymmetryCalc : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "AsymmetryCalc"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates the asymmetry between two groups of detectors for a "
           "muon workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CalculateMuonAsymmetry"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }

protected:
  std::map<std::string, std::string> validateInputs() override;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
